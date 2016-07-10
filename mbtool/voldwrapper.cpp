/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "voldwrapper.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/file.h"


#define SOCKET_DIR                  "/dev/socket"

#define SOCKET_ENV_PREFIX           "ANDROID_SOCKET_"

#define SOCKET_NAME_CRYPTD          "cryptd"
#define SOCKET_NAME_VOLD            "vold"

#define VOLD_PATH                   "/system/bin/vold"

namespace mb
{

static int create_socket(const char *name, int type, mode_t perm, uid_t uid,
                         gid_t gid)
{
    struct sockaddr_un addr;
    int fd, ret;

    fd = socket(AF_LOCAL, type, 0);
    if (fd < 0) {
        LOGE("%s: Failed to open socket: %s", name, strerror(errno));
        return -1;
    }

    memset(&addr, 0 , sizeof(addr));
    addr.sun_family = AF_UNIX;
    snprintf(addr.sun_path, sizeof(addr.sun_path), SOCKET_DIR "/%s", name);

    ret = unlink(addr.sun_path);
    if (ret < 0 && errno != ENOENT) {
        LOGE("%s: Failed to unlink old socket: %s", name, strerror(errno));
        goto out_close;
    }

    ret = bind(fd, (struct sockaddr *) &addr, sizeof(addr));
    if (ret < 0) {
        LOGE("%s: Failed to bind socket: %s", name, strerror(errno));
        goto out_unlink;
    }

    chown(addr.sun_path, uid, gid);
    chmod(addr.sun_path, perm);

    LOGI("%s: Create socket with mode=%o, uid=%d, gid=%d",
         addr.sun_path, perm, uid, gid);

    return fd;

out_unlink:
    unlink(addr.sun_path);
out_close:
    close(fd);
    return -1;
}

static bool publish_socket(const char *name, int fd)
{
    char key[64] = SOCKET_ENV_PREFIX;
    char val[64];

    strlcat(key, name, sizeof(key));
    snprintf(val, sizeof(val), "%d", fd);
    if (setenv(key, val, 1) < 0) {
        return false;
    }

    // Don't close the socket on exec
    fcntl(fd, F_SETFD, 0);

    return true;
}

static void voldwrapper_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: voldwrapper [OPTION]...\n\n"
            "Options:\n"
            "  -h, --help       Display this help message\n");
}

int voldwrapper_main(int argc, char *argv[])
{
    int opt;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            voldwrapper_usage(stdout);
            return EXIT_SUCCESS;

        default:
            voldwrapper_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    // There should be no more arguments
    if (argc - optind != 0) {
        voldwrapper_usage(stderr);
        return EXIT_FAILURE;
    }

    int fd_vold = -1;
    int fd_cryptd = -1;

    fd_vold = create_socket("vold", SOCK_STREAM, 0600, 0, 0);
    if (fd_vold < 0 || !publish_socket("vold", fd_vold)) {
        LOGE("Failed to create vold socket");
        goto error;
    }

    // Only create cryptd socket if vold needs it
    if (util::file_find_one_of(VOLD_PATH, { "cryptd" })) {
        LOGD("vold uses cryptd socket");

        fd_cryptd = create_socket("cryptd", SOCK_STREAM, 0600, 0, 0);
        if (fd_cryptd < 0 || !publish_socket("cryptd", fd_cryptd)) {
            LOGE("Failed to create cryptd socket");
            goto error;
        }
    }

    execl(VOLD_PATH, VOLD_PATH, nullptr);
    LOGE("%s: Failed to exec: %s", VOLD_PATH, strerror(errno));

error:
    if (fd_vold >= 0) {
        close(fd_vold);
    }
    if (fd_cryptd >= 0) {
        close(fd_cryptd);
    }
    return EXIT_FAILURE;
}

}
