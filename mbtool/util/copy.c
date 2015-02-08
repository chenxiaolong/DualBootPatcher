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

#include "util/copy.h"

#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>

#include "logging.h"

// WARNING: Everything operates on paths, so it's subject to race conditions
// Directory copy operations will not cross mountpoint boundaries

int mb_copy_data_fd(int fd_source, int fd_target)
{
    char buf[10240];
    ssize_t nread;

    while ((nread = read(fd_source, buf, sizeof buf)) > 0) {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            if ((nwritten = write(fd_target, out_ptr, nread)) < 0) {
                goto error;
            }

            nread -= nwritten;
            out_ptr += nwritten;
        } while (nread > 0);
    }

    if (nread == 0) {
        return 0;
    }

error:
    return -1;
}

static int copy_data(const char *source, const char *target)
{
    int fd_source = -1;
    int fd_target = -1;
    int saved_errno;

    fd_source = open(source, O_RDONLY);
    if (fd_source < 0) {
        goto error;
    }

    fd_target = open(target, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_target < 0) {
        goto error;
    }

    if (mb_copy_data_fd(fd_source, fd_target) < 0) {
        goto error;
    }

    if (close(fd_source) < 0) {
        fd_source = -1;
        goto error;
    }
    if (close(fd_target) < 0) {
        fd_target = -1;
        goto error;
    }

    return 0;

error:
    saved_errno = errno;

    if (fd_source >= 0) {
        close(fd_source);
    }
    if (fd_target >= 0) {
        close(fd_target);
    }

    errno = saved_errno;
    return -1;
}

static int copy_xattrs(const char *source, const char *target)
{
    ssize_t size;
    char *names = NULL;
    char *end_names;
    char *name;
    char *value = NULL;

    // xattr names are in a NULL-separated list
    size = llistxattr(source, NULL, 0);
    if (size < 0) {
        if (errno == ENOTSUP) {
            LOGV("%s: xattrs not supported on filesystem", source);
            goto success;
        } else {
            LOGE("%s: Failed to list xattrs: %s", source, strerror(errno));
            goto error;
        }
    }

    names = malloc(size + 1);
    if (!names) {
        errno = ENOMEM;
        goto error;
    }

    size = llistxattr(source, names, size);
    if (size < 0) {
        LOGE("%s: Failed to list xattrs on second try: %s",
             source, strerror(errno));
        goto error;
    } else {
        names[size] = '\0';
        end_names = names + size;
    }

    for (name = names; name != end_names; name = strchr(name, '\0') + 1) {
        if (!*name) {
            continue;
        }

        size = lgetxattr(source, name, NULL, 0);
        if (size < 0) {
            LOGW("%s: Failed to get attribute '%s': %s",
                 source, name, strerror(errno));
            continue;
        }

        char *temp_value;
        if ((temp_value = realloc(value, size))) {
            value = temp_value;
        } else {
            errno = ENOMEM;
            goto error;
        }
        size = lgetxattr(source, name, value, size);
        if (size < 0) {
            LOGW("%s: Failed to get attribute '%s' on second try: %s",
                 source, name, strerror(errno));
            continue;
        }

        if (lsetxattr(target, name, value, size, 0) < 0) {
            if (errno == ENOTSUP) {
                LOGV("%s: xattrs not supported on filesystem", target);
                break;
            } else {
                LOGE("%s: Failed to set xattrs: %s", target, strerror(errno));
                goto error;
            }
        }
    }

success:
    free(names);
    free(value);
    return 0;

error:
    free(names);
    free(value);
    return -1;
}

static int copy_stat(const char *source, const char *target)
{
    struct stat sb;

    if (lstat(source, &sb) < 0) {
        LOGE("%s: Failed to stat: %s", source, strerror(errno));
        return -1;
    }

    if (lchown(target, sb.st_uid, sb.st_gid) < 0) {
        LOGE("%s: Failed to chown: %s", target, strerror(errno));
        return -1;
    }

    if (!S_ISLNK(sb.st_mode)) {
        if (chmod(target, sb.st_mode & (S_ISUID | S_ISGID | S_ISVTX
                                      | S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
            LOGE("%s: Failed to chmod: %s", target, strerror(errno));
            return -1;
        }
    }

    return 0;
}

// Dynamically memory allocation version of readlink()
static int readlink2(const char *path, char **out_ptr)
{
    size_t buf_len;
    char *buf = NULL;
    ssize_t len;

    buf_len = 64;
    buf = malloc(buf_len);
    if (!buf) {
        errno = ENOMEM;
        return -1;
    }

    for (;;) {
        len = readlink(path, buf, buf_len - 1);
        if (len < 0) {
            free(buf);
            return len;
        } else if ((size_t) len == buf_len - 1) {
            // Increase buffer size
            char *temp_buf;

            buf_len *= 2;
            if ((temp_buf = realloc(buf, buf_len))) {
                buf = temp_buf;
            } else {
                free(buf);
                errno = ENOMEM;
                return -1;
            }
        } else {
            break;
        }
    }

    buf[len] = '\0';
    *out_ptr = buf;
    return 0;
}

int mb_copy_contents(const char *source, const char *target)
{
    int fd_source = -1;
    int fd_target = -1;

    if ((fd_source = open(source, O_RDONLY)) < 0) {
        goto error;
    }

    if ((fd_target = open(target, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
        goto error;
    }

    if (mb_copy_data_fd(fd_source, fd_target) < 0) {
        goto error;
    }

    close(fd_source);
    close(fd_target);

    return 0;

error:
    close(fd_source);
    close(fd_target);

    return -1;
}

int mb_copy_file(const char *source, const char *target, int flags)
{
    mode_t old_umask = umask(0);

    if (unlink(target) < 0 && errno != ENOENT) {
        LOGE("%s: Failed to remove old file: %s",
             target, strerror(errno));
        goto error;
    }

    struct stat sb;
    if (((flags & MB_COPY_FOLLOW_SYMLINKS) ? stat : lstat)(source, &sb) < 0) {
        LOGE("%s: Failed to stat: %s",
             source, strerror(errno));
        goto error;
    }

    switch (sb.st_mode & S_IFMT) {
    case S_IFBLK:
        if (mknod(target, S_IFBLK | S_IRWXU, sb.st_rdev) < 0) {
            LOGW("%s: Failed to create block device: %s",
                 target, strerror(errno));
            goto error;
        }
        break;

    case S_IFCHR:
        if (mknod(target, S_IFCHR | S_IRWXU, sb.st_rdev) < 0) {
            LOGW("%s: Failed to create character device: %s",
                 target, strerror(errno));
            goto error;
        }
        break;

    case S_IFIFO:
        if (mkfifo(target, S_IRWXU) < 0) {
            LOGW("%s: Failed to create FIFO pipe: %s",
                 target, strerror(errno));
            goto error;
        }
        break;

    case S_IFLNK:
        if (!(flags & MB_COPY_FOLLOW_SYMLINKS)) {
            char *symlink_path = NULL;
            if (readlink2(source, &symlink_path) < 0) {
                LOGW("%s: Failed to read symlink path: %s",
                     source, strerror(errno));
                goto error;
            }

            if (symlink(symlink_path, target) < 0) {
                LOGW("%s: Failed to create symlink: %s",
                     target, strerror(errno));
                free(symlink_path);
                goto error;
            }
            free(symlink_path);

            break;
        }

        // Treat as file

    case S_IFREG:
        if (copy_data(source, target) < 0) {
            LOGE("%s: Failed to copy data: %s",
                 target, strerror(errno));
            goto error;
        }
        break;

    case S_IFSOCK:
        LOGE("%s: Cannot copy socket", target);
        errno = EINVAL;
        goto error;

    case S_IFDIR:
        LOGE("%s: Cannot copy directory", target);
        errno = EINVAL;
        goto error;
    }

    if ((flags & MB_COPY_ATTRIBUTES) && copy_stat(source, target) < 0) {
        LOGE("%s: Failed to copy attributes: %s",
             target, strerror(errno));
        goto error;
    }
    if ((flags & MB_COPY_XATTRS) && copy_xattrs(source, target) < 0) {
        LOGE("%s: Failed to copy xattrs: %s",
             target, strerror(errno));
        goto error;
    }

    umask(old_umask);
    return 0;

error:
    umask(old_umask);
    return -1;
}

// Copy as much as possible
int mb_copy_dir(const char *source, const char *target, int flags)
{
    mode_t old_umask = umask(0);
    int ret = 0;
    FTS *ftsp = NULL;
    FTSENT *curr;
    FTSENT *root = NULL;
    char *pathbuf = NULL;
    size_t pathsize = 0;

    // This is almost *never* useful, so we won't allow it
    if (flags & MB_COPY_FOLLOW_SYMLINKS) {
        LOGE("MB_COPY_FOLLOW_SYMLINKS not allowed for recursive copies");
        errno = EINVAL;
        ret = -1;
        goto finish;
    }

    struct stat sb;

    if (mkdir(target, S_IRWXU | S_IRWXG | S_IRWXO) < 0 && errno != EEXIST) {
        LOGE("%s: Failed to create directory: %s", target, strerror(errno));
        ret = -1;
        goto finish;
    }

    if (stat(target, &sb) < 0) {
        LOGE("%s: Failed to stat: %s", target, strerror(errno));
        ret = -1;
        goto finish;
    }

    if (!S_ISDIR(sb.st_mode)) {
        LOGE("%s: Target exists but is not a directory", target);
        ret = -1;
        goto finish;
    }

    char *files[] = { (char *) source, NULL };

    ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, NULL);
    if (!ftsp) {
        LOGE("%s: fts_open failed: %s", source, strerror(errno));
        ret = -1;
        goto finish;
    }

    // Copy as much as possible, skipping errors
    // (but still returning -1 at the end)
    while ((curr = fts_read(ftsp))) {
        switch (curr->fts_info) {
        case FTS_NS:  // no stat()
        case FTS_DNR: // directory not read
        case FTS_ERR: // other error
            LOGW("%s: fts_read error: %s",
                 curr->fts_accpath, strerror(curr->fts_errno));
            continue;

        case FTS_DC:
            // Not reached unless FTS_LOGICAL specified (FTS_PHYSICAL doesn't
            // follow symlinks)
            continue;

        case FTS_DOT:
            // Not reached unless FTS_SEEDOT specified
            continue;

        case FTS_NSOK:
            // Not reached unless FTS_NOSTAT specified
            continue;
        }

        if (sb.st_dev == curr->fts_statp->st_dev
                && sb.st_ino == curr->fts_statp->st_ino) {
            LOGE("%s: Cannot copy on top of itself", curr->fts_path);
            ret = -1;
            goto finish;
        }

        if (curr->fts_level == 0) {
            root = curr;
        }

        // According to fts_read()'s manpage, fts_path includes the path given
        // to fts_open() as a prefix, so 'curr->fts_path + strlen(source)' will
        // give us a relative path we can append to the target.
        pathsize = strlen(target) + 1
                + strlen(curr->fts_path) - strlen(source) + 1;

        // If we're not excluding the top level, add the length of the search
        // root + 1 (for slash).
        if (!(flags & MB_COPY_EXCLUDE_TOP_LEVEL)) {
            pathsize += root->fts_namelen + 1;
        }

        char *temp_pathbuf;
        if ((temp_pathbuf = realloc(pathbuf, pathsize))) {
            pathbuf = temp_pathbuf;
        } else {
            ret = -1;
            errno = ENOMEM;
            goto finish;
        }
        pathbuf[0] = '\0';

        char *relpath = curr->fts_path + strlen(source);

        strlcat(pathbuf, target, pathsize);
        if (!(flags & MB_COPY_EXCLUDE_TOP_LEVEL)) {
            if (pathbuf[strlen(pathbuf) - 1] != '/') {
                strlcat(pathbuf, "/", pathsize);
            }
            strlcat(pathbuf, root->fts_name, pathsize);
        }
        if (pathbuf[strlen(pathbuf) - 1] != '/'
                && *relpath != '/' && *relpath != '\0') {
            strlcat(pathbuf, "/", pathsize);
        }
        strlcat(pathbuf, relpath, pathsize);

        struct stat sb_temp;
        int skip = 0;

        switch (curr->fts_info) {
        case FTS_D:
            if (mkdir(pathbuf, S_IRWXU | S_IRWXG | S_IRWXO) < 0
                    && errno != EEXIST) {
                LOGW("%s: Failed to create directory: %s",
                     pathbuf, strerror(errno));
                ret = -1;
                skip = 1;
            }
            if (!skip && stat(pathbuf, &sb_temp) == 0
                    && !S_ISDIR(sb_temp.st_mode)) {
                LOGW("%s: Exists but is not a directory",
                     pathbuf);
                ret = -1;
                skip = 1;
            }

            if (skip) {
                // If we're skipping, then we have to set the attributes now,
                // since we won't ever hit FTS_DP for this directory.

                if ((flags & MB_COPY_ATTRIBUTES)
                        && copy_stat(curr->fts_accpath, pathbuf) < 0) {
                    LOGW("%s: Failed to copy attributes: %s",
                        pathbuf, strerror(errno));
                    ret = -1;
                }

                if ((flags & MB_COPY_XATTRS)
                        && copy_xattrs(curr->fts_accpath, pathbuf) < 0) {
                    LOGW("%s: Failed to copy xattrs: %s",
                         pathbuf, strerror(errno));
                    ret = -1;
                }

                fts_set(ftsp, curr, FTS_SKIP);
                continue;
            }

            continue;

        case FTS_DP:
            if ((flags & MB_COPY_ATTRIBUTES)
                    && copy_stat(curr->fts_accpath, pathbuf) < 0) {
                LOGW("%s: Failed to copy attributes: %s",
                     pathbuf, strerror(errno));
                ret = -1;
                continue;
            }
            if ((flags & MB_COPY_XATTRS)
                    && copy_xattrs(curr->fts_accpath, pathbuf) < 0) {
                LOGW("%s: Failed to copy xattrs: %s",
                     pathbuf, strerror(errno));
                ret = -1;
                continue;
            }
            continue;

        case FTS_F:
            if (unlink(pathbuf) < 0 && errno != ENOENT) {
                LOGW("%s: Failed to remove old file: %s",
                     pathbuf, strerror(errno));
                ret = -1;
                continue;
            }
            if (copy_data(curr->fts_accpath, pathbuf) < 0) {
                LOGW("%s: Failed to copy data: %s",
                     pathbuf, strerror(errno));
                ret = -1;
                continue;
            }
            if ((flags & MB_COPY_ATTRIBUTES)
                    && copy_stat(curr->fts_accpath, pathbuf) < 0) {
                LOGW("%s: Failed to copy attributes: %s",
                     pathbuf, strerror(errno));
                ret = -1;
                continue;
            }
            if ((flags & MB_COPY_XATTRS)
                    && copy_xattrs(curr->fts_accpath, pathbuf) < 0) {
                LOGW("%s: Failed to copy xattrs: %s",
                     pathbuf, strerror(errno));
                ret = -1;
                continue;
            }
            continue;

        case FTS_SL:
        case FTS_SLNONE:
            if (unlink(pathbuf) < 0 && errno != ENOENT) {
                LOGW("%s: Failed to remove old symlink: %s",
                     pathbuf, strerror(errno));
                ret = -1;
                continue;
            }

            char *symlink_path = NULL;
            if (readlink2(curr->fts_accpath, &symlink_path) < 0) {
                LOGW("%s: Failed to read symlink path: %s",
                     curr->fts_accpath, strerror(errno));
                ret = -1;
                continue;
            }

            if (symlink(symlink_path, pathbuf) < 0) {
                LOGW("%s: Failed to create symlink: %s",
                     pathbuf, strerror(errno));
                free(symlink_path);
                ret = -1;
                continue;
            }
            free(symlink_path);
            if ((flags & MB_COPY_ATTRIBUTES)
                    && copy_stat(curr->fts_accpath, pathbuf) < 0) {
                LOGW("%s: Failed to copy attributes: %s",
                     pathbuf, strerror(errno));
                ret = -1;
                continue;
            }
            if ((flags & MB_COPY_XATTRS)
                    && copy_xattrs(curr->fts_accpath, pathbuf) < 0) {
                LOGW("%s: Failed to copy xattrs: %s",
                     pathbuf, strerror(errno));
            }
            continue;

        case FTS_DEFAULT:
            switch (curr->fts_statp->st_mode & S_IFMT) {
            case S_IFBLK:
                if (mknod(pathbuf, S_IFBLK | S_IRWXU,
                        curr->fts_statp->st_rdev) < 0) {
                    LOGW("%s: Failed to create block device: %s",
                         pathbuf, strerror(errno));
                    ret = -1;
                    continue;
                }
                break;

            case S_IFCHR:
                if (mknod(pathbuf, S_IFCHR | S_IRWXU,
                        curr->fts_statp->st_rdev) < 0) {
                    LOGW("%s: Failed to create character device: %s",
                         pathbuf, strerror(errno));
                    ret = -1;
                    continue;
                }
                break;

            case S_IFIFO:
                if (mkfifo(pathbuf, S_IRWXU) < 0) {
                    LOGW("%s: Failed to create FIFO pipe: %s",
                         pathbuf, strerror(errno));
                    ret = -1;
                    continue;
                }
                break;

            case S_IFSOCK:
                LOGD("%s: Skipping socket", curr->fts_accpath);
                continue;
            }

            if ((flags & MB_COPY_ATTRIBUTES)
                    && copy_stat(curr->fts_accpath, pathbuf) < 0) {
                LOGW("%s: Failed to copy attributes: %s",
                     pathbuf, strerror(errno));
                ret = -1;
                continue;
            }
            if ((flags & MB_COPY_XATTRS)
                    && copy_xattrs(curr->fts_accpath, pathbuf) < 0) {
                LOGW("%s: Failed to copy xattrs: %s",
                     pathbuf, strerror(errno));
                ret = -1;
                continue;
            }

            continue;
        }
    }

finish:
    if (ftsp) {
        fts_close(ftsp);
    }
    free(pathbuf);

    umask(old_umask);

    return ret;
}
