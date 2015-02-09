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

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "actions.h"
#include "roms.h"
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


static int v1_list_roms(int fd)
{
    struct roms roms;
    mb_roms_init(&roms);
    mb_roms_get_installed(&roms);

    if (mb_socket_write_int32(fd, roms.len) < 0) {
        goto error;
    }

    for (unsigned int i = 0; i < roms.len; ++i) {
        struct rom *r = roms.list[i];

        int success = mb_socket_write_string(fd, "ROM_BEGIN") == 0;
        if (success && r->id) {
            success = mb_socket_write_string(fd, "ID") == 0
                    && mb_socket_write_string(fd, r->id) == 0;
        }
        if (success && r->system_path) {
            success = mb_socket_write_string(fd, "SYSTEM_PATH") == 0
                    && mb_socket_write_string(fd, r->system_path) == 0;
        }
        if (success && r->cache_path) {
            success = mb_socket_write_string(fd, "CACHE_PATH") == 0
                    && mb_socket_write_string(fd, r->cache_path) == 0;
        }
        if (success && r->data_path) {
            success = mb_socket_write_string(fd, "DATA_PATH") == 0
                    && mb_socket_write_string(fd, r->data_path) == 0;
        }
        if (success) {
            success = mb_socket_write_string(fd, "USE_RAW_PATHS") == 0
                    && mb_socket_write_int32(fd, r->use_raw_paths) == 0;
        }
        if (success) {
            success = mb_socket_write_string(fd, "ROM_END") == 0;
        }

        if (!success) {
            goto error;
        }
    }

    if (mb_socket_write_string(fd, RESPONSE_OK) < 0) {
        goto error;
    }

    mb_roms_cleanup(&roms);
    return 0;

error:
    mb_roms_cleanup(&roms);
    return -1;
}

static int v1_choose_rom(int fd)
{
    char *id = NULL;
    char *boot_blockdev = NULL;

    if (mb_socket_read_string(fd, &id) < 0) {
        goto error;
    }

    if (mb_socket_read_string(fd, &boot_blockdev) < 0) {
        goto error;
    }

    int ret;

    if (mb_action_choose_rom(id, boot_blockdev) < 0) {
        ret = mb_socket_write_string(fd, RESPONSE_FAIL);
    } else {
        ret = mb_socket_write_string(fd, RESPONSE_SUCCESS);
    }

    if (ret < 0) {
        goto error;
    }

    free(id);
    free(boot_blockdev);
    return 0;

error:
    free(id);
    free(boot_blockdev);
    return -1;
}

static int v1_set_kernel(int fd)
{
    char *id = NULL;
    char *boot_blockdev = NULL;

    if (mb_socket_read_string(fd, &id) < 0) {
        goto error;
    }

    if (mb_socket_read_string(fd, &boot_blockdev) < 0) {
        goto error;
    }

    int ret;

    if (mb_action_set_kernel(id, boot_blockdev) < 0) {
        ret = mb_socket_write_string(fd, RESPONSE_FAIL);
    } else {
        ret = mb_socket_write_string(fd, RESPONSE_SUCCESS);
    }

    if (ret < 0) {
        goto error;
    }

    free(id);
    free(boot_blockdev);
    return 0;

error:
    free(id);
    free(boot_blockdev);
    return -1;
}

static int v1_reboot(int fd)
{
    char *reboot_arg = NULL;

    if (mb_socket_read_string(fd, &reboot_arg) < 0) {
        goto error;
    }

    if (mb_action_reboot(reboot_arg) < 0) {
        if (mb_socket_write_string(fd, RESPONSE_FAIL) < 0) {
            goto error;
        }
    }

    // Not reached

    free(reboot_arg);
    return 0;

error:
    free(reboot_arg);
    return -1;
}

static int connection_version_1(int fd)
{
    if (mb_socket_write_string(fd, RESPONSE_OK) < 0) {
        goto error;
    }

    char *command = NULL;

    while (1) {
        if (mb_socket_read_string(fd, &command) < 0) {
            goto error;
        }

        if (strcmp(command, V1_COMMAND_LIST_ROMS) == 0
                || strcmp(command, V1_COMMAND_CHOOSE_ROM) == 0
                || strcmp(command, V1_COMMAND_SET_KERNEL) == 0
                || strcmp(command, V1_COMMAND_REBOOT) == 0) {
            // Acknowledge command
            if (mb_socket_write_string(fd, RESPONSE_OK) < 0) {
                goto error;
            }
        } else {
            // Invalid command; allow further commands
            if (mb_socket_write_string(fd, RESPONSE_UNSUPPORTED) < 0) {
                goto error;
            }
        }

        // NOTE: A negative return value indicates a connection error, not a
        //       command failure!
        int ret = 0;

        if (strcmp(command, V1_COMMAND_LIST_ROMS) == 0) {
            ret = v1_list_roms(fd);
        } else if (strcmp(command, V1_COMMAND_CHOOSE_ROM) == 0) {
            ret = v1_choose_rom(fd);
        } else if (strcmp(command, V1_COMMAND_SET_KERNEL) == 0) {
            ret = v1_set_kernel(fd);
        } else if (strcmp(command, V1_COMMAND_REBOOT) == 0) {
            ret = v1_reboot(fd);
        }

        if (ret < 0) {
            goto error;
        }

        free(command);
    }

    return 0;

error:
    LOGE("[Version 1] Communication error");
    free(command);
    return -1;
}

static int client_connection(int fd)
{
    LOGD("Accepted connection from %d", fd);

    struct ucred cred;
    socklen_t cred_len = sizeof(struct ucred);

    if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &cred_len) < 0) {
        LOGE("Failed to get socket credentials: %s", strerror(errno));
        goto error;
    }

    LOGD("Client PID: %d", cred.pid);
    LOGD("Client UID: %d", cred.uid);
    LOGD("Client GID: %d", cred.gid);

    // TODO: Verify credentials
    if (mb_socket_write_string(fd, RESPONSE_ALLOW) < 0) {
        LOGE("Failed to send credentials allowed message");
        goto error;
    }

    int32_t version;
    if (mb_socket_read_int32(fd, &version) < 0) {
        LOGE("Failed to get interface version");
        goto error;
    }

    if (version == 1) {
        return connection_version_1(fd);
    } else {
        LOGE("Unsupported interface version: %d", version);
        mb_socket_write_string(fd, RESPONSE_UNSUPPORTED);
        goto error;
    }

    return 0;

error:
    LOGE("Killing connection");
    return -1;
}

static int run_daemon(void)
{
    int fd;
    struct sockaddr_un addr;

    fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (fd < 0) {
        LOGE("Failed to create socket: %s", strerror(errno));
        goto error;
    }

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
        goto error;
    }

    if (listen(fd, 3) < 0) {
        LOGE("Failed to listen on socket: %s", strerror(errno));
        goto error;
    }

    LOGD("Socket ready, waiting for connections");

    int client_fd;
    while ((client_fd = accept(fd, NULL, NULL)) >= 0) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            LOGE("Failed to fork: %s", strerror(errno));
        } else if (child_pid == 0) {
            int ret = client_connection(client_fd);
            close(client_fd);
            return ret;
        }
        close(client_fd);
    }

    if (client_fd < 0) {
        LOGE("Failed to accept connection on socket: %s", strerror(errno));
        goto error;
    }

    return 0;

error:
    if (fd > 0) {
        close(fd);
    }
    return -1;
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
