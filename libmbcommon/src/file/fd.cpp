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

#include "mbcommon/file/fd.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/file/callbacks.h"
#include "mbcommon/file/fd_p.h"

MB_BEGIN_C_DECLS

static int fd_open_cb(struct MbFile *file, void *userdata)
{
    FdFileCtx *ctx = static_cast<FdFileCtx *>(userdata);
    struct stat sb;

    if (ctx->vtable.fn_fstat(ctx->vtable.userdata, ctx->fd, &sb) < 0) {
        mb_file_set_error(file, -errno,
                          "Failed to stat file: %s", strerror(errno));
        return MB_FILE_FAILED;
    }

    if (S_ISDIR(sb.st_mode)) {
        mb_file_set_error(file, -EISDIR, "Cannot open directory");
        return MB_FILE_FAILED;
    }

    return MB_FILE_OK;
}

static int fd_close_cb(struct MbFile *file, void *userdata)
{
    FdFileCtx *ctx = static_cast<FdFileCtx *>(userdata);
    int ret = MB_FILE_OK;

    if (ctx->owned && ctx->vtable.fn_close(
            ctx->vtable.userdata, ctx->fd) < 0) {
        mb_file_set_error(file, -errno,
                          "Failed to close file: %s", strerror(errno));
        ret = MB_FILE_FAILED;
    }

    free(ctx);

    return ret;
}

static int fd_read_cb(struct MbFile *file, void *userdata,
                      void *buf, size_t size,
                      size_t *bytes_read)
{
    FdFileCtx *ctx = static_cast<FdFileCtx *>(userdata);

    if (size > SSIZE_MAX) {
        size = SSIZE_MAX;
    }

    ssize_t n = ctx->vtable.fn_read(ctx->vtable.userdata, ctx->fd, buf, size);
    if (n < 0) {
        mb_file_set_error(file, -errno,
                          "Failed to read file: %s", strerror(errno));
        return errno == EINTR ? MB_FILE_RETRY : MB_FILE_FAILED;
    }

    *bytes_read = n;
    return MB_FILE_OK;
}

static int fd_write_cb(struct MbFile *file, void *userdata,
                       const void *buf, size_t size,
                       size_t *bytes_written)
{
    FdFileCtx *ctx = static_cast<FdFileCtx *>(userdata);

    if (size > SSIZE_MAX) {
        size = SSIZE_MAX;
    }

    ssize_t n = ctx->vtable.fn_write(ctx->vtable.userdata, ctx->fd, buf, size);
    if (n < 0) {
        mb_file_set_error(file, -errno,
                          "Failed to write file: %s", strerror(errno));
        return errno == EINTR ? MB_FILE_RETRY : MB_FILE_FAILED;
    }

    *bytes_written = n;
    return MB_FILE_OK;
}

static int fd_seek_cb(struct MbFile *file, void *userdata,
                      int64_t offset, int whence,
                      uint64_t *new_offset)
{
    FdFileCtx *ctx = static_cast<FdFileCtx *>(userdata);

    off64_t ret = ctx->vtable.fn_lseek64(
            ctx->vtable.userdata, ctx->fd, offset, whence);
    if (ret < 0) {
        mb_file_set_error(file, -errno,
                          "Failed to seek file: %s", strerror(errno));
        return MB_FILE_FAILED;
    }

    *new_offset = ret;
    return MB_FILE_OK;
}

static int fd_truncate_cb(struct MbFile *file, void *userdata,
                          uint64_t size)
{
    FdFileCtx *ctx = static_cast<FdFileCtx *>(userdata);

    if (ctx->vtable.fn_ftruncate64(ctx->vtable.userdata, ctx->fd, size) < 0) {
        mb_file_set_error(file, -errno,
                          "Failed to truncate file: %s", strerror(errno));
        return MB_FILE_FAILED;
    }

    return MB_FILE_OK;
}

int _mb_file_open_fd(struct MbFile *file, int fd, bool owned,
                     SysVtable *vtable)
{
    if (!vtable
            || !vtable->fn_fstat
            || !vtable->fn_close
            || !vtable->fn_ftruncate64
            || !vtable->fn_lseek64
            || !vtable->fn_read
            || !vtable->fn_write) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Invalid or incomplete vtable");
        return MB_FILE_FATAL;
    }

    FdFileCtx *ctx = static_cast<FdFileCtx *>(calloc(1, sizeof(FdFileCtx)));
    if (!ctx) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Failed to allocate FdFileCtx: %s",
                          strerror(errno));
        return MB_FILE_FATAL;
    }

    ctx->fd = fd;
    ctx->owned = owned;
    ctx->vtable = *vtable;

    return mb_file_open_callbacks(file,
                                  &fd_open_cb,
                                  &fd_close_cb,
                                  &fd_read_cb,
                                  &fd_write_cb,
                                  &fd_seek_cb,
                                  &fd_truncate_cb,
                                  ctx);
}

int mb_file_open_fd(struct MbFile *file, int fd, bool owned)
{
    SysVtable vtable{};
    _vtable_fill_system_funcs(&vtable);
    return _mb_file_open_fd(file, fd, owned, &vtable);
}

MB_END_C_DECLS
