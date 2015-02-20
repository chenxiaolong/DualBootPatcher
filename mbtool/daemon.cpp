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

#include "daemon.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <unistd.h>

#include "actions.h"
#include "roms.h"
#include "validcerts.h"
#include "util/finally.h"
#include "util/logging.h"
#include "util/properties.h"
#include "util/socket.h"

#define RESPONSE_ALLOW "ALLOW"                  // Credentials allowed
#define RESPONSE_DENY "DENY"                    // Credentials denied
#define RESPONSE_OK "OK"                        // Generic accepted response
#define RESPONSE_SUCCESS "SUCCESS"              // Generic success response
#define RESPONSE_FAIL "FAIL"                    // Generic failure response
#define RESPONSE_UNSUPPORTED "UNSUPPORTED"      // Generic unsupported response

#define V1_COMMAND_LIST_ROMS "LIST_ROMS"        // [Version 1] List installed ROMs
#define V1_COMMAND_CHOOSE_ROM "CHOOSE_ROM"      // [Version 1] Switch ROM
#define V1_COMMAND_SET_KERNEL "SET_KERNEL"      // [Version 1] Set kernel
#define V1_COMMAND_REBOOT "REBOOT"              // [Version 1] Reboot
#define V1_COMMAND_OPEN "OPEN"                  // [Version 1] Open file


namespace mb
{

static bool v1_list_roms(int fd)
{
    std::vector<std::shared_ptr<Rom>> roms;
    mb_roms_add_installed(&roms);

    if (!util::socket_write_int32(fd, roms.size())) {
        return false;
    }

    for (auto r : roms) {
        std::string build_prop;
        if (r->use_raw_paths) {
            build_prop += "/raw";
        }
        build_prop += r->system_path;
        build_prop += "/build.prop";

        std::unordered_map<std::string, std::string> properties;
        util::file_get_all_properties(build_prop, &properties);

        bool success = util::socket_write_string(fd, "ROM_BEGIN")
                && util::socket_write_string(fd, "ID")
                && util::socket_write_string(fd, r->id)
                && util::socket_write_string(fd, "SYSTEM_PATH")
                && util::socket_write_string(fd, r->system_path)
                && util::socket_write_string(fd, "CACHE_PATH")
                && util::socket_write_string(fd, r->cache_path)
                && util::socket_write_string(fd, "DATA_PATH")
                && util::socket_write_string(fd, r->data_path)
                && util::socket_write_string(fd, "USE_RAW_PATHS")
                && util::socket_write_int32(fd, r->use_raw_paths);

        if (success && properties.find("ro.build.version.release")
                != properties.end()) {
            const std::string &version = properties["ro.build.version.release"];
            success = util::socket_write_string(fd, "VERSION")
                    && util::socket_write_string(fd, version);
        }
        if (success && properties.find("ro.build.display.id")
                != properties.end()) {
            const std::string &build = properties["ro.build.display.id"];
            success = util::socket_write_string(fd, "BUILD")
                    && util::socket_write_string(fd, build);
        }
        if (success) {
            success = util::socket_write_string(fd, "ROM_END");
        }

        if (!success) {
            return false;
        }
    }

    if (!util::socket_write_string(fd, RESPONSE_OK)) {
        return false;
    }

    return true;
}

static bool v1_choose_rom(int fd)
{
    std::string id;
    std::string boot_blockdev;

    if (!util::socket_read_string(fd, &id)) {
        return false;
    }

    if (!util::socket_read_string(fd, &boot_blockdev)) {
        return false;
    }

    bool ret;

    if (!action_choose_rom(id, boot_blockdev)) {
        ret = util::socket_write_string(fd, RESPONSE_FAIL);
    } else {
        ret = util::socket_write_string(fd, RESPONSE_SUCCESS);
    }

    if (!ret) {
        return false;
    }

    return true;
}

static bool v1_set_kernel(int fd)
{
    std::string id;
    std::string boot_blockdev;

    if (!util::socket_read_string(fd, &id)) {
        return false;
    }

    if (!util::socket_read_string(fd, &boot_blockdev)) {
        return false;
    }

    bool ret;

    if (!action_set_kernel(id, boot_blockdev)) {
        ret = util::socket_write_string(fd, RESPONSE_FAIL);
    } else {
        ret = util::socket_write_string(fd, RESPONSE_SUCCESS);
    }

    if (!ret) {
        return false;
    }

    return true;
}

static bool v1_reboot(int fd)
{
    std::string reboot_arg;

    if (!util::socket_read_string(fd, &reboot_arg)) {
        return false;
    }

    if (!action_reboot(reboot_arg)) {
        if (!util::socket_write_string(fd, RESPONSE_FAIL)) {
            return false;
        }
    }

    // Not reached
    return true;
}

static bool v1_open(int fd)
{
    std::string path;
    std::vector<std::string> modes;

    if (!util::socket_read_string(fd, &path)) {
        return false;
    }

    if (!util::socket_read_string_array(fd, &modes)) {
        return false;
    }

    int flags = 0;

    for (const std::string &m : modes) {
        if (m == "APPEND") {
            flags |= O_APPEND;
        } else if (m == "CREAT") {
            flags |= O_CREAT;
        } else if (m == "EXCL") {
            flags |= O_EXCL;
        } else if (m == "RDWR") {
            flags |= O_RDWR;
        } else if (m == "TRUNC") {
            flags |= O_TRUNC;
        } else if (m == "WRONLY") {
            flags |= O_WRONLY;
        }
    }

    int ffd = open(path.c_str(), flags, 0666);
    if (ffd < 0) {
        if (!util::socket_write_string(fd, RESPONSE_FAIL)) {
            return false;
        }
    }

    auto close_ffd = util::finally([&]{
        close(ffd);
    });

    if (!util::socket_write_string(fd, RESPONSE_SUCCESS)) {
        return false;
    }

    if (!util::socket_send_fds(fd, { ffd })) {
        return false;
    }

    return true;
}

static bool connection_version_1(int fd)
{
    std::string command;

    if (!util::socket_write_string(fd, RESPONSE_OK)) {
        return false;
    }

    while (1) {
        if (!util::socket_read_string(fd, &command)) {
            return false;
        }

        if (command == V1_COMMAND_LIST_ROMS
                || command == V1_COMMAND_CHOOSE_ROM
                || command == V1_COMMAND_SET_KERNEL
                || command == V1_COMMAND_REBOOT
                || command == V1_COMMAND_OPEN) {
            // Acknowledge command
            if (!util::socket_write_string(fd, RESPONSE_OK)) {
                return false;
            }
        } else {
            // Invalid command; allow further commands
            if (!util::socket_write_string(fd, RESPONSE_UNSUPPORTED)) {
                return false;
            }
        }

        // NOTE: A negative return value indicates a connection error, not a
        //       command failure!
        bool ret = true;

        if (command == V1_COMMAND_LIST_ROMS) {
            ret = v1_list_roms(fd);
        } else if (command == V1_COMMAND_CHOOSE_ROM) {
            ret = v1_choose_rom(fd);
        } else if (command == V1_COMMAND_SET_KERNEL) {
            ret = v1_set_kernel(fd);
        } else if (command == V1_COMMAND_REBOOT) {
            ret = v1_reboot(fd);
        } else if (command == V1_COMMAND_OPEN) {
            ret = v1_open(fd);
        }

        if (!ret) {
            return false;
        }
    }

    return true;
}

static bool verify_credentials(uid_t uid)
{
    // Rely on the OS for signature checking and simply compare strings in
    // packages.xml. The only way that file changes is if the package is
    // removed and reinstalled, in which case, Android will kill the client and
    // the connection will terminate. Or, the client already has root access, in
    // which case, there's not much we can do to prevent damage.

    Packages pkgs;
    if (!pkgs.load_xml("/data/system/packages.xml")) {
        LOGE("Failed to load /data/system/packages.xml");
        return false;
    }

    std::shared_ptr<Package> pkg = pkgs.find_by_uid(uid);
    if (!pkg) {
        LOGE("Failed to find package for UID {:d}", uid);
        return false;
    }

    pkg->dump();
    LOGD("{} has {:d} signatures", pkg->name, pkg->sig_indexes.size());

    for (const std::string &index : pkg->sig_indexes) {
        if (pkgs.sigs.find(index) == pkgs.sigs.end()) {
            LOGW("Signature index {} has no key", index);
            continue;
        }

        const std::string &key = pkgs.sigs[index];
        if (std::find(valid_certs.begin(), valid_certs.end(), key)
                != valid_certs.end()) {
            LOGV("{} matches whitelisted signatures", pkg->name);
            return true;
        }
    }

    LOGE("{} does not match whitelisted signatures", pkg->name);
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

    LOGD("Accepted connection from {:d}", fd);

    struct ucred cred;
    socklen_t cred_len = sizeof(struct ucred);

    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &cred_len) < 0) {
        LOGE("Failed to get socket credentials: {}", strerror(errno));
        return ret = false;
    }

    LOGD("Client PID: {:d}", cred.pid);
    LOGD("Client UID: {:d}", cred.uid);
    LOGD("Client GID: {:d}", cred.gid);

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

    if (version == 1) {
        if (!connection_version_1(fd)) {
            LOGE("[Version 1] Communication error");
        }
        return true;
    } else {
        LOGE("Unsupported interface version: {:d}", version);
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
        LOGE("Failed to create socket: {}", strerror(errno));
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
        LOGE("Failed to bind socket: {}", strerror(errno));
        LOGE("Is another instance running?");
        return false;
    }

    if (listen(fd, 3) < 0) {
        LOGE("Failed to listen on socket: {}", strerror(errno));
        return false;
    }

    LOGD("Socket ready, waiting for connections");

    int client_fd;
    while ((client_fd = accept(fd, nullptr, nullptr)) >= 0) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            LOGE("Failed to fork: {}", strerror(errno));
        } else if (child_pid == 0) {
            bool ret = client_connection(client_fd);
            close(client_fd);
            exit(ret ? 0 : 1);
        }
        close(client_fd);
    }

    if (client_fd < 0) {
        LOGE("Failed to accept connection on socket: {}", strerror(errno));
        return false;
    }

    return true;
}

static bool run_daemon_fork(void)
{
    pid_t first, second;
    int status;
    if ((first = fork()) >= 0) {
        if (first == 0) {
            if ((second = fork()) >= 0) {
                if (second == 0) {
                    run_daemon();
                }
                exit(EXIT_SUCCESS);
            } else {
                LOGE("Failed to fork");
                exit(EXIT_FAILURE);
            }
        } else {
            first = waitpid(first, &status, 0);
        }
    } else {
        LOGE("Failed to fork");
    }

    return first == -1 ? false : true;
}

static void daemon_usage(int error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: daemon [OPTION]...\n\n"
            "Options:\n"
            "  -d, --daemonize  Fork to background\n"
            "  -h, --help       Display this help message\n");
}

int daemon_main(int argc, char *argv[])
{
    int opt;
    bool fork_flag = false;

    static struct option long_options[] = {
        {"daemonize", no_argument, 0, 'd'},
        {"help",      no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "dh", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'd':
            fork_flag = true;
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

    if (fork_flag) {
        return run_daemon_fork() ? EXIT_SUCCESS : EXIT_FAILURE;
    } else {
        return run_daemon() ? EXIT_SUCCESS : EXIT_FAILURE;
    }
}

}