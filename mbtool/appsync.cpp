/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "appsync.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include "util/finally.h"
#include "util/logging.h"
#include "util/selinux.h"
#include "util/socket.h"

#define ANDROID_SOCKET_ENV_PREFIX       "ANDROID_SOCKET_"
#define ANDROID_SOCKET_DIR              "/dev/socket"

#define INSTALLD_PATH                   "/system/bin/installd"
#define SOCKET_PATH                     "installd"

// Original init.rc definition for the socket is:
//    socket installd stream 600 system system
#define INSTALLD_SOCKET_PATH            ANDROID_SOCKET_DIR "/installd.real"
#define INSTALLD_SOCKET_UID             1000 // AID_SYSTEM
#define INSTALLD_SOCKET_GID             1000 // AID_SYSTEM
#define INSTALLD_SOCKET_PERMS           0600
#define INSTALLD_SOCKET_CONTEXT         "u:object_r:installd_socket:s0"

#define COMMAND_BUF_SIZE                1024


namespace mb
{

//static int launch_installd()
//{
//    const char *argv[] = { INSTALLD_PATH, nullptr };
//    if (execve(argv[0], const_cast<char * const *>(argv), environ) < 0) {
//        LOGE("Failed to launch installd: {}", strerror(errno));
//        return -1;
//    }
//}

/*!
 * \brief Get installd socket fd created by init from ANDROID_SOCKET_installd
 *
 * \return The fd if the environment variable was set. Otherwise, -1
 */
static int get_socket_from_env(const char *name)
{
    char key[64] = ANDROID_SOCKET_ENV_PREFIX;
    strlcat(key, name, sizeof(key));

    const char *value = getenv(key);
    if (!value) {
        return -1;
    }

    errno = 0;
    int fd = strtol(value, nullptr, 10);
    if (errno) {
        return -1;
    }

    return fd;
}

/*!
 * \brief Set ANDROID_SOCKET_installd to the new installd socket fd
 */
static void put_socket_to_env(const char *name, int fd)
{
    char key[64] = ANDROID_SOCKET_ENV_PREFIX;
    char value[64];

    strlcat(key, name, sizeof(key));
    snprintf(value, sizeof(value), "%d", fd);

    setenv(key, value, 1);
}

/*!
 * \brief Create a new socket for installd to listen on
 */
static int create_new_socket()
{
    struct sockaddr_un addr;
    int fd;

    fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) {
        LOGE("Failed to create socket: {}", strerror(errno));
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s",
             INSTALLD_SOCKET_PATH);

    if (unlink(addr.sun_path) < 0 && errno != ENOENT) {
        LOGE("Failed to unlink old socket: {}", strerror(errno));
        close(fd);
        return -1;
    }

    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        LOGE("Failed to bind socket: {}", strerror(errno));
        unlink(addr.sun_path);
        close(fd);
        return -1;
    }

    chown(addr.sun_path, INSTALLD_SOCKET_UID, INSTALLD_SOCKET_GID);
    chmod(addr.sun_path, INSTALLD_SOCKET_PERMS);
    util::selinux_set_context(addr.sun_path, INSTALLD_SOCKET_CONTEXT);

    // Make sure close-on-exec is cleared to the fd remains open across execve()
    fcntl(fd, F_SETFD, 0);

    return fd;
}

/*
 * Socket messages are prefixed with 16-bit unsigned value (little-endian)
 * indicating the number of bytes that follow. The data should be treated as
 * a string and a null terminator must be added to the end.
 */

/*!
 * \brief Receive a message from a socket
 */
static bool receive_message(int fd, char *buf, size_t size)
{
    unsigned short count;

    if (!util::socket_read_uint16(fd, &count)) {
        LOGE("Failed to read command size");
        return false;
    }

    if (count < 1 || count >= size) {
        LOGE("Invalid size {:d}", count);
        return false;
    }

    if (util::socket_read(fd, buf, count) != count) {
        LOGE("Failed to read command");
        return false;
    }

    buf[count] = 0;

    return true;
}

/*!
 * \brief Send a message to a socket
 */
static bool send_message(int fd, const char *command)
{
    unsigned short count = strlen(command);

    if (!util::socket_write_uint16(fd, count)) {
        LOGE("Failed to write command size");
        return false;
    }

    if (util::socket_write(fd, command, count) != count) {
        LOGE("Failed to write command");
        return false;
    }

    return true;
}

/*!
 * \brief Connect to the installd socket at INSTALLD_SOCKET_PATH
 *
 * Because installd is spawned right before this function is called, this
 * function will make 5 attempts to connect to the socket. If an attempt fails,
 * it will wait 1 second before trying again.
 *
 * \return fd if the connection succeeds. Otherwise, -1
 */
static int connect_to_installd()
{
    struct sockaddr_un addr;

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_LOCAL;
    snprintf(addr.sun_path, sizeof(addr.sun_path), "%s",
             INSTALLD_SOCKET_PATH);

    int fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) {
        LOGE("Failed to create socket: {}", strerror(errno));
        return -1;
    }

    int attempt;
    for (attempt = 0; attempt < 5; ++attempt) {
        LOGV("Connecting to installd [Attempt {}/{}]", attempt + 1, 5);
        if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
            sleep(1);
        } else {
            break;
        }
    }

    if (attempt == 5) {
        LOGD("Failed to connect to installd after 5 attempts");
        close(fd);
        return -1;
    }

    LOGD("Connected to installd");

    return fd;
}

/*!
 * \brief Run installd
 *
 * This will fork and run /system/bin/installd.
 *
 * \return installd's pid if it was successfully launched. Otherwise -1
 */
static pid_t spawn_installd()
{
    // Spawn installd
    pid_t pid = fork();
    if (pid == 0) {
        const char *argv[] = { INSTALLD_PATH, nullptr };
        execve(argv[0], const_cast<char * const *>(argv), environ);
        LOGE("Failed to launch installd: {}", strerror(errno));
        _exit(EXIT_FAILURE);
    } else if (pid < 0) {
        LOGD("Failed to fork: {}", strerror(errno));
        return -1;
    }
    return pid;
}

static std::vector<std::string> parse_args(const char *cmdline)
{
    std::vector<char> buf(cmdline, cmdline + strlen(cmdline) + 1);
    std::vector<std::string> args;

    char *temp;
    char *token;

    token = strtok_r(buf.data(), " ", &temp);
    while (token) {
        args.push_back(token);

        token = strtok_r(nullptr, " ", &temp);
    }

    return args;
}

static std::string args_to_string(const std::vector<std::string> &args)
{
    std::string output("[");
    for (size_t i = 0; i < args.size(); ++i) {
        if (i != 0) {
            output += ", ";
        }
        output += args[i];
    }
    output += "]";
    return output;
}

/**
 * \brief Main function for capturing and relaying the daemon commands
 *
 * This function will not return under normal conditions. It runs through the
 * following loop infinitely:
 *
 * 1. Accept a connection on the original installd socket
 * 2. Run installd
 * 3. Connect to the installd
 * 4. Capture and proxy commands
 *
 * If installd crashes or connection between mbtool and installd breaks in some
 * way, it will simply loop again. If this function fails to accept a connection
 * on the original socket, then it will return false.
 *
 * \return False if accepting the socket connection fails. Otherwise, does not
 *         return
 */
static bool proxy_process(int fd)
{
    while (true) {
        int client_fd = accept(fd, nullptr, nullptr);
        if (client_fd < 0) {
            LOGE("Failed to accept client connection: {}", strerror(errno));
            return false;
        }

        auto close_client_fd = util::finally([&]{
            LOGD("Closing client connection");
            close(client_fd);
        });

        LOGD("Accepted new client connection");

        LOGD("Launching installd");
        pid_t pid = spawn_installd();
        if (pid < 0) {
            continue;
        }

        // Kill installd when we exit
        auto kill_installd = util::finally([&]{
            kill(pid, SIGINT);

            int status;

            do {
                if (waitpid(pid, &status, 0) < 0) {
                    LOGE("Failed to waitpid(): {}", strerror(errno));
                    break;
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        });

        // Connect to installd
        int installd_fd = connect_to_installd();
        if (installd_fd < 0) {
            continue;
        }

        auto close_installd_fd = util::finally([&]{
            LOGD("Closing installd connection");
            close(installd_fd);
        });

        // Use the same buffer size as installd
        char buf[COMMAND_BUF_SIZE];

        while (true) {
            if (!receive_message(client_fd, buf, sizeof(buf))) {
                break;
            }

            std::vector<std::string> args = parse_args(buf);

            bool log_result = true;

            if (args.empty()) {
                LOGE("Invalid command (empty message)");
            } else {
                const std::string &cmd = args[0];

                if (cmd == "ping"
                        || cmd == "freecache") {
                    LOGD("Received unimportant command: [{}, ...]", cmd);
                } else if (cmd == "getsize") {
                    // Get size is so annoying we don't want it to show... EVER!
                    log_result = false;
                } else if (cmd == "install"
                        || cmd == "dexopt"
                        || cmd == "movedex"
                        || cmd == "rmdex"
                        || cmd == "remove"
                        || cmd == "rename"
                        || cmd == "fixuid"
                        || cmd == "rmcache"
                        || cmd == "rmcodecache"
                        || cmd == "rmuserdata"
                        || cmd == "movefiles"
                        || cmd == "linklib"
                        || cmd == "mkuserdata"
                        || cmd == "mkuserconfig"
                        || cmd == "rmuser"
                        || cmd == "idmap"
                        || cmd == "restorecondata"
                        || cmd == "patchoat") {
                    LOGD("Received command: {}", args_to_string(args));
                } else {
                    LOGW("Unrecognized command: {}", args_to_string(args));
                }
            }

            if (!send_message(installd_fd, buf)) {
                break;
            }

            if (!receive_message(installd_fd, buf, sizeof(buf))) {
                break;
            }

            args = parse_args(buf);

            if (log_result) {
                LOGD("Sending reply: {}", args_to_string(args));
            }

            if (!send_message(client_fd, buf)) {
                break;
            }
        }
    }

    // Not reached
    return true;
}

/*!
 * \brief Set up the environment for proxying the installd socket
 */
static bool hijack_socket()
{
    LOGD("Starting appsync");

    int orig_fd = get_socket_from_env(SOCKET_PATH);
    if (orig_fd < 0) {
        LOGE("Failed to get socket fd from environment");
        return false;
    }

    fcntl(orig_fd, F_SETFD, FD_CLOEXEC);

    // Listen on the original socket
    if (listen(orig_fd, 5) < 0) {
        LOGE("Failed to listen on socket: {}", strerror(errno));
        return false;
    }

    // Create a new socket for installd
    int new_fd = create_new_socket();
    if (new_fd < 0) {
        LOGE("Failed to create new socket for installd");
        return false;
    }

    auto close_new_fd = util::finally([&]{
        close(new_fd);
    });

    // Put the new fd in the environment
    put_socket_to_env(SOCKET_PATH, new_fd);

    LOGD("Ready! Waiting for connections");

    // Start processing commands!
    if (!proxy_process(orig_fd)) {
        return false;
    }

    return true;
}

/*!
 * \brief Patch SEPolicy to allow installd to connect to our fd
 */
static bool patch_sepolicy()
{
    policydb_t pdb;

    if (policydb_init(&pdb) < 0) {
        LOGE("Failed to initialize policydb");
        return false;
    }

    if (!util::selinux_read_policy(MB_SELINUX_POLICY_FILE, &pdb)) {
        LOGE("Failed to read SELinux policy file: {}", MB_SELINUX_POLICY_FILE);
        policydb_destroy(&pdb);
        return false;
    }

    LOGD("Policy version: {}", pdb.policyvers);

    util::selinux_add_rule(&pdb, "installd", "init", "unix_stream_socket", "accept");
    util::selinux_add_rule(&pdb, "installd", "init", "unix_stream_socket", "listen");
    util::selinux_add_rule(&pdb, "installd", "init", "unix_stream_socket", "read");
    util::selinux_add_rule(&pdb, "installd", "init", "unix_stream_socket", "write");

    if (!util::selinux_write_policy(MB_SELINUX_LOAD_FILE, &pdb)) {
        LOGE("Failed to write SELinux policy file: {}", MB_SELINUX_LOAD_FILE);
        policydb_destroy(&pdb);
        return false;
    }

    policydb_destroy(&pdb);

    return true;
}

static void appsync_usage(bool error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: appsync [OPTION]...\n\n"
            "Options:\n"
            "  -h, --help    Display this help message\n"
           "\n"
           "This tool implements app (apk and data) sharing by hijacking the connection\n"
           "between the Android Java framework and the installd daemon. It captures the\n"
           "incoming messages and performs the needed operations. Messages between the\n"
           "framework and the installd daemon are not altered in any way, though this may\n"
           "change in the future.\n"
           "\n"
           "To use this tool, the installd definition in /init.rc should be modified to\n"
           "point to mbtool. For example, the following original definition:\n"
           "\n"
           "    service installd /system/bin/installd\n"
           "        class main\n"
           "        socket installd stream 600 system system\n"
           "\n"
           "would be changed to the following:\n"
           "\n"
           "    service installd /appsync\n"
           "        class main\n"
           "        socket installd stream 600 system system\n"
           "\n"
           "It is not necessary to keep the installd definition because appsync will run\n"
           "/system/bin/installd automatically.\n");
}

int appsync_main(int argc, char *argv[])
{
    int opt;

    static struct option long_options[] = {
        {"help",   no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            appsync_usage(false);
            return EXIT_SUCCESS;

        default:
            appsync_usage(true);
            return EXIT_FAILURE;
        }
    }

    // There should be no other arguments
    if (argc - optind != 0) {
        appsync_usage(true);
        return EXIT_FAILURE;
    }


    typedef std::unique_ptr<std::FILE, int (*)(std::FILE *)> file_ptr;

    file_ptr fp(fopen("/sdcard/appsync.log", "wb"), fclose);
    if (!fp) {
        fprintf(stderr, "Failed to open log: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // mbtool logging
    util::log_set_logger(std::make_shared<util::StdioLogger>(fp.get()));


    // Allow installd to read and write to our socket
    struct stat sb;
    if (stat("/sys/fs/selinux", &sb) < 0) {
        LOGV("SELinux not supported. No need to modify policy");
    } else {
        LOGV("Patching SELinux policy to allow installd connection");
        if (!patch_sepolicy()) {
            LOGE("Failed to patch current SELinux policy");
        }
    }


    return hijack_socket() ? EXIT_SUCCESS : EXIT_FAILURE;
}

}