/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2015 Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "initwrapper/util.h"

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/directory.h"
#include "mbutil/path.h"

#define LOG_TAG "mbtool/initwrapper/util"

/*
 * replaces any unacceptable characters with '_', the
 * length of the resulting string is equal to the input string
 */
void sanitize(char *s)
{
    const char *accept =
            "abcdefghijklmnopqrstuvwxyz"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "0123456789"
            "_-.";

    if (!s) {
        return;
    }

    while (*s) {
        s += strspn(s, accept);
        if (*s) {
            *s++ = '_';
        }
    }
}

void make_link_init(const char *oldpath, const char *newpath)
{
    if (!mb::util::mkdir_parent(newpath, 0755)) {
        LOGE("Failed to create parent directory of %s: %s",
             newpath, strerror(errno));
    }

    if (symlink(oldpath, newpath) < 0) {
        LOGE("Failed to symlink %s to %s: %s",
             oldpath, newpath, strerror(errno));
    }
}

void remove_link(const char *oldpath, const char *newpath)
{
    std::string path;
    if (!mb::util::read_link(newpath, path)) {
        return;
    }
    if (path == oldpath) {
        unlink(newpath);
    }
}

void open_devnull_stdio(void)
{
    static const char *name = "/dev/__null__";
    if (mknod(name, S_IFCHR | 0600, (1 << 8) | 3) == 0) {
        int fd = open(name, O_RDWR);
        unlink(name);
        if (fd >= 0) {
            dup2(fd, 0);
            dup2(fd, 1);
            dup2(fd, 2);
            if (fd > 2) {
                close(fd);
            }
            return;
        }
    }
}
