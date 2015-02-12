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
#include <sys/un.h>
#include <unistd.h>

#include "actions.h"
#include "roms.h"
#include "util/finally.h"
#include "util/logging.h"
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
                && util::socket_write_int32(fd, r->use_raw_paths)
                && util::socket_write_string(fd, "ROM_END");

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

    if (util::socket_read_string(fd, &boot_blockdev)) {
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
                || command == V1_COMMAND_REBOOT) {
            // Acknowledge command
            if (!util::socket_write_string(fd, RESPONSE_OK)) {
                return false;
            }
        } else {
            // Invalid command; allow further commands
            if (util::socket_write_string(fd, RESPONSE_UNSUPPORTED)) {
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
        }

        if (!ret) {
            return false;
        }
    }

    return true;
}

#if 0
static int verify_credentials(uid_t uid)
{
    struct packages pkgs;
    mb_packages_init(&pkgs);
    mb_packages_load_xml(&pkgs, "/data/local/tmp/packages.xml");



    mb_packages_cleanup(&pkgs);

    return 0;
}
#endif

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

    LOGD("Client PID: %d", cred.pid);
    LOGD("Client UID: %d", cred.uid);
    LOGD("Client GID: %d", cred.gid);

    // TODO: Verify credentials
    if (!util::socket_write_string(fd, RESPONSE_ALLOW)) {
        LOGE("Failed to send credentials allowed message");
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

    LOGD("Socket ready, waiting for connections");

    int client_fd;
    while ((client_fd = accept(fd, NULL, NULL)) >= 0) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            LOGE("Failed to fork: %s", strerror(errno));
        } else if (child_pid == 0) {
            bool ret = client_connection(client_fd);
            close(client_fd);
            exit(ret ? 0 : 1);
        }
        close(client_fd);
    }

    if (client_fd < 0) {
        LOGE("Failed to accept connection on socket: %s", strerror(errno));
        return false;
    }

    return true;
}

static void daemon_usage(int error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: daemon [OPTION]...\n\n"
            "Options:\n"
            "  -h, --help    Display this help message\n");
}

int daemon_main(int argc, char *argv[])
{
    int opt;

    static struct option long_options[] = {
        {"help",   no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "ls:t:h", long_options, &long_index)) != -1) {
        switch (opt) {
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

    return run_daemon() ? EXIT_SUCCESS : EXIT_FAILURE;
}

}