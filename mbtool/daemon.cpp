/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "daemon.h"

#include <algorithm>

#include <fcntl.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <proc/readproc.h>

#include "mblog/logging.h"
#include "mblog/stdio_logger.h"
#include "mbutil/autoclose/file.h"
#include "mbutil/directory.h"
#include "mbutil/finally.h"
#include "mbutil/properties.h"
#include "mbutil/selinux.h"
#include "mbutil/socket.h"

#include "daemon_v3.h"
#include "multiboot.h"
#include "packages.h"
#include "sepolpatch.h"
#include "validcerts.h"
#include "version.h"

#define RESPONSE_ALLOW "ALLOW"                  // Credentials allowed
#define RESPONSE_DENY "DENY"                    // Credentials denied
#define RESPONSE_OK "OK"                        // Generic accepted response
#define RESPONSE_UNSUPPORTED "UNSUPPORTED"      // Generic unsupported response


namespace mb
{

static int pipe_fds[2];
static bool send_ok_to_pipe = false;

static autoclose::file log_fp(nullptr, std::fclose);

static bool verify_credentials(uid_t uid)
{
    // Rely on the OS for signature checking and simply compare strings in
    // packages.xml. The only way that file changes is if the package is
    // removed and reinstalled, in which case, Android will kill the client and
    // the connection will terminate. Or, the client already has root access, in
    // which case, there's not much we can do to prevent damage.

    Packages pkgs;
    if (!pkgs.load_xml(PACKAGES_XML)) {
        LOGE("Failed to load " PACKAGES_XML);
        return false;
    }

    std::shared_ptr<Package> pkg = pkgs.find_by_uid(uid);
    if (!pkg) {
        LOGE("Failed to find package for UID %u", uid);
        return false;
    }

    pkg->dump();
    LOGD("%s has %zu signatures", pkg->name.c_str(), pkg->sig_indexes.size());

    for (const std::string &index : pkg->sig_indexes) {
        if (pkgs.sigs.find(index) == pkgs.sigs.end()) {
            LOGW("Signature index %s has no key", index.c_str());
            continue;
        }

        const std::string &key = pkgs.sigs[index];
        if (std::find(valid_certs.begin(), valid_certs.end(), key)
                != valid_certs.end()) {
            LOGV("%s matches whitelisted signatures", pkg->name.c_str());
            return true;
        }
    }

    LOGE("%s does not match whitelisted signatures", pkg->name.c_str());
    return false;
}

static bool client_connection(int fd)
{
    bool ret = true;
    auto fail = util::finally([&] {
        if (!ret) {
            LOGE("Killing connection");
        }
    });

    LOGD("Accepted connection from %d", fd);

    struct ucred cred;
    socklen_t cred_len = sizeof(struct ucred);

    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &cred_len) < 0) {
        LOGE("Failed to get socket credentials: %s", strerror(errno));
        return ret = false;
    }

    LOGD("Client PID: %u", cred.pid);
    LOGD("Client UID: %u", cred.uid);
    LOGD("Client GID: %u", cred.gid);

    if (verify_credentials(cred.uid)) {
        if (!util::socket_write_string(fd, RESPONSE_ALLOW)) {
            LOGE("Failed to send credentials allowed message");
            return ret = false;
        }
    } else {
        if (!util::socket_write_string(fd, RESPONSE_DENY)) {
            LOGE("Failed to send credentials denied message");
        }
        return ret = false;
    }

    int32_t version;
    if (!util::socket_read_int32(fd, &version)) {
        LOGE("Failed to get interface version");
        return ret = false;
    }

    if (version == 2) {
        LOGE("Protocol version 2 is no longer supported");
        util::socket_write_string(fd, RESPONSE_UNSUPPORTED);
        return ret = false;
    } else if (version == 3) {
        if (!util::socket_write_string(fd, RESPONSE_OK)) {
            return false;
        }

        if (!connection_version_3(fd)) {
            LOGE("[Version 3] Communication error");
        }
        return true;
    } else {
        LOGE("Unsupported interface version: %d", version);
        util::socket_write_string(fd, RESPONSE_UNSUPPORTED);
        return ret = false;
    }

    return true;
}

static bool run_daemon(void)
{
    int fd;
    struct sockaddr_un addr;

    fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) {
        LOGE("Failed to create socket: %s", strerror(errno));
        return false;
    }

    auto close_fd = util::finally([&] {
        close(fd);
    });

    char abs_name[] = "\0mbtool.daemon";
    size_t abs_name_len = sizeof(abs_name) - 1;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    memcpy(addr.sun_path, abs_name, abs_name_len);

    // Calculate correct length so the trailing junk is not included in the
    // abstract socket name
    socklen_t addr_len = offsetof(struct sockaddr_un, sun_path) + abs_name_len;

    if (bind(fd, (struct sockaddr *) &addr, addr_len) < 0) {
        LOGE("Failed to bind socket: %s", strerror(errno));
        LOGE("Is another instance running?");
        return false;
    }

    if (listen(fd, 3) < 0) {
        LOGE("Failed to listen on socket: %s", strerror(errno));
        return false;
    }

    // Let parent process know that we're ready if we're forking the daemon
    if (send_ok_to_pipe) {
        ssize_t n = write(pipe_fds[1], "", 1);
        close(pipe_fds[1]);
        if (n < 0) {
            LOGE("Failed to send OK to parent process");
            return false;
        }
    }

    // Eat zombies!
    // SIG_IGN reaps zombie processes (it's not just a dummy function)
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGCHLD, &sa, 0) < 0) {
        LOGE("Failed to set SIGCHLD handler: %s", strerror(errno));
        return false;
    }

    LOGD("Socket ready, waiting for connections");

    int client_fd;
    while ((client_fd = accept(fd, nullptr, nullptr)) >= 0) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            LOGE("Failed to fork: %s", strerror(errno));
        } else if (child_pid == 0) {
            // Restore default SIGCHLD handler
            struct sigaction sa;
            sa.sa_handler = SIG_DFL;
            sigemptyset(&sa.sa_mask);
            sa.sa_flags = 0;
            if (sigaction(SIGCHLD, &sa, 0) < 0) {
                LOGE("Failed to set default SIGCHLD handler: %s",
                     strerror(errno));
                _exit(127);
            }

            bool ret = client_connection(client_fd);
            close(client_fd);
            _exit(ret ? EXIT_SUCCESS : EXIT_FAILURE);
        }
        close(client_fd);
    }

    if (client_fd < 0) {
        LOGE("Failed to accept connection on socket: %s", strerror(errno));
        return false;
    }

    return true;
}

static bool redirect_stdio_to_dev_null()
{
    bool ret = true;

    int fd = open("/dev/null", O_RDWR);
    if (fd < 0) {
        LOGE("Failed to open /dev/null: %s", strerror(errno));
        return false;
    }
    if (dup2(fd, STDIN_FILENO) < 0) {
        LOGE("Failed to reopen stdin: %s", strerror(errno));
        ret = false;
    }
    if (dup2(fd, STDOUT_FILENO) < 0) {
        LOGE("Failed to reopen stdout: %s", strerror(errno));
        ret = false;
    }
    if (dup2(fd, STDERR_FILENO) < 0) {
        LOGE("Failed to reopen stderr: %s", strerror(errno));
        ret = false;
    }
    if (fd != STDIN_FILENO && fd != STDOUT_FILENO && fd != STDERR_FILENO) {
        close(fd);
    }

    return ret;
}

static bool daemon_init()
{
    if (chdir("/") < 0) {
        LOGE("Failed to change cwd to /: %s", strerror(errno));
        return false;
    }

    umask(0);

    if (!redirect_stdio_to_dev_null()) {
        return false;
    }

    // Set up logging
    if (!util::mkdir_parent(MULTIBOOT_LOG_DAEMON, 0775) && errno != EEXIST) {
        LOGE("Failed to create parent directory of %s: %s",
             MULTIBOOT_LOG_DAEMON, strerror(errno));
        return false;
    }

    log_fp = autoclose::fopen(MULTIBOOT_LOG_DAEMON, "w");
    if (!log_fp) {
        LOGE("Failed to open log file %s: %s",
             MULTIBOOT_LOG_DAEMON, strerror(errno));
        return false;
    }

    fix_multiboot_permissions();

    // mbtool logging
    log::log_set_logger(std::make_shared<log::StdioLogger>(log_fp.get(), true));

    LOGD("Initialized daemon");

    return true;
}

__attribute__((noreturn))
static void run_daemon_fork(void)
{
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Failed to fork: %s\n", strerror(errno));
        _exit(EXIT_FAILURE);
    } else if (pid > 0) {
        int status;
        do {
            if (waitpid(pid, &status, 0) < 0) {
                fprintf(stderr, "Failed to waitpid(): %s\n", strerror(errno));
                _exit(EXIT_FAILURE);
            }
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));

        _exit(EXIT_SUCCESS);
    }

    // Become session and process group leader
    if (setsid() < 0) {
        fprintf(stderr, "Failed to become session leader: %s\n",
                strerror(errno));
        _exit(EXIT_FAILURE);
    }

    // Ignore hangup signal
    signal(SIGHUP, SIG_IGN);

    // Create pipe for the daemon to tell us it is listening for connections
    send_ok_to_pipe = true;
    if (pipe(pipe_fds) < 0) {
        fprintf(stderr, "Failed to create pipe: %s\n", strerror(errno));
        _exit(EXIT_FAILURE);
    }

    pid = fork();
    if (pid < 0) {
        fprintf(stderr, "Failed to fork: %s\n", strerror(errno));
        _exit(EXIT_FAILURE);
    } else if (pid > 0) {
        // Close write end of pipe
        close(pipe_fds[1]);

        fprintf(stderr, "Waiting for daemon to start...");

        // Wait for OK from child
        char dummy;
        ssize_t n = read(pipe_fds[0], &dummy, 1);
        // Close read end of pipe
        close(pipe_fds[0]);

        if (n < 0) {
            fprintf(stderr, " Failed: %s\n", strerror(errno));
            _exit(EXIT_FAILURE);
        } else if (n != 1) {
            fprintf(stderr, " Failed\n");
            _exit(EXIT_FAILURE);
        }

        fprintf(stderr, " OK\n");
        _exit(EXIT_SUCCESS);
    }

    // Close read end of the pipe
    close(pipe_fds[0]);

    _exit((daemon_init() && run_daemon()) ? EXIT_SUCCESS : EXIT_FAILURE);
}

static bool patch_sepolicy_daemon()
{
    policydb_t pdb;

    if (policydb_init(&pdb) < 0) {
        LOGE("Failed to initialize policydb");
        return false;
    }

    if (!util::selinux_read_policy(SELINUX_POLICY_FILE, &pdb)) {
        LOGE("Failed to read SELinux policy file: %s", SELINUX_POLICY_FILE);
        policydb_destroy(&pdb);
        return false;
    }

    LOGD("Policy version: %u", pdb.policyvers);

    util::selinux_add_rule(&pdb, "untrusted_app", "init",
                           "unix_stream_socket", "connectto");

    if (!util::selinux_write_policy(SELINUX_LOAD_FILE, &pdb)) {
        LOGE("Failed to write SELinux policy file: %s", SELINUX_LOAD_FILE);
        policydb_destroy(&pdb);
        return false;
    }

    policydb_destroy(&pdb);

    return true;
}

static void daemon_usage(bool error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: daemon [OPTION]...\n\n"
            "Options:\n"
            "  -d, --daemonize  Fork to background\n"
            "  -r, --replace    Kill existing daemon (if any) before starting\n"
            "  -h, --help       Display this help message\n");
}

int daemon_main(int argc, char *argv[])
{
    int opt;
    bool fork_flag = false;
    bool replace_flag = false;

    static struct option long_options[] = {
        {"daemonize", no_argument, 0, 'd'},
        {"replace",   no_argument, 0, 'r'},
        {"help",      no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "drh", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'd':
            fork_flag = true;
            break;

        case 'r':
            replace_flag = true;
            break;

        case 'h':
            daemon_usage(0);
            return EXIT_SUCCESS;

        default:
            daemon_usage(1);
            return EXIT_FAILURE;
        }
    }

    // There should be no other arguments
    if (argc - optind != 0) {
        daemon_usage(1);
        return EXIT_FAILURE;
    }

    // Patch SELinux policy to make init permissive
    patch_loaded_sepolicy();

    // Allow untrusted_app to connect to our daemon
    patch_sepolicy_daemon();

    // Set version property if we're the system mbtool (i.e. launched by init)
    // Possible to override this with another program by double forking, letting
    // 2nd child reparent to init, and then calling execve("/mbtool", ...), but
    // meh ...
    if (getppid() == 1) {
        if (!util::set_property("ro.multiboot.version", get_mbtool_version())) {
            LOGW("Failed to set 'ro.multiboot.version' to '%s'\n",
                 get_mbtool_version());
        }
    }

    if (replace_flag) {
        PROCTAB *proc = openproc(PROC_FILLCOM | PROC_FILLSTAT);
        if (proc) {
            pid_t curpid = getpid();

            while (proc_t *info = readproc(proc, nullptr)) {
                if (strcmp(info->cmd, "mbtool") == 0          // This is mbtool
                        && info->cmdline                      // And we can see the command line
                        && info->cmdline[1]                   // And argc > 1
                        && strstr(info->cmdline[1], "daemon") // And it's a daemon process
                        && info->tid != curpid) {             // And we're not killing ourself
                    // Kill the daemon process
                    LOGV("Killing PID %d\n", info->tid);
                    kill(info->tid, SIGTERM);
                }

                freeproc(info);
            }

            closeproc(proc);
        }

        // Give processes a chance to exit
        usleep(500000);
    }

    if (fork_flag) {
        run_daemon_fork();
    } else {
        return (daemon_init() && run_daemon()) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}

}
