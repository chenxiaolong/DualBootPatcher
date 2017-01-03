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

#include "mbcommon/file/posix.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/file/callbacks.h"
#include "mbcommon/file/posix_p.h"

#ifndef __ANDROID__
static_assert(sizeof(off_t) > 4, "Not compiling with LFS support!");
#endif

MB_BEGIN_C_DECLS

static int posix_open_cb(struct MbFile *file, void *userdata)
{
    PosixFileCtx *ctx = static_cast<PosixFileCtx *>(userdata);
    struct stat sb;
    int fd;

    fd = ctx->vtable.fn_fileno(ctx->vtable.userdata, ctx->fp);
    if (fd >= 0) {
        if (ctx->vtable.fn_fstat(ctx->vtable.userdata, fd, &sb) < 0) {
            mb_file_set_error(file, -errno,
                              "Failed to stat file: %s", strerror(errno));
            return MB_FILE_FAILED;
        }

        if (S_ISDIR(sb.st_mode)) {
            mb_file_set_error(file, -EISDIR, "Cannot open directory");
            return MB_FILE_FAILED;
        }
    }

    return MB_FILE_OK;
}

static int posix_close_cb(struct MbFile *file, void *userdata)
{
    PosixFileCtx *ctx = static_cast<PosixFileCtx *>(userdata);
    int ret = MB_FILE_OK;

    if (ctx->owned && ctx->vtable.fn_fclose(
            ctx->vtable.userdata, ctx->fp) == EOF) {
        mb_file_set_error(file, -errno,
                          "Failed to close file: %s", strerror(errno));
        ret = MB_FILE_FAILED;
    }

    // Clean up resources
    free(ctx);

    return ret;
}

static int posix_read_cb(struct MbFile *file, void *userdata,
                         void *buf, size_t size,
                         size_t *bytes_read)
{
    PosixFileCtx *ctx = static_cast<PosixFileCtx *>(userdata);

    size_t n = ctx->vtable.fn_fread(
            ctx->vtable.userdata, buf, 1, size, ctx->fp);

    if (n < size && ctx->vtable.fn_ferror(ctx->vtable.userdata, ctx->fp)) {
        mb_file_set_error(file, -errno,
                          "Failed to read file: %s", strerror(errno));
        return errno == EINTR ? MB_FILE_RETRY : MB_FILE_FAILED;
    }

    *bytes_read = n;
    return MB_FILE_OK;
}

static int posix_write_cb(struct MbFile *file, void *userdata,
                          const void *buf, size_t size,
                          size_t *bytes_written)
{
    PosixFileCtx *ctx = static_cast<PosixFileCtx *>(userdata);

    size_t n = ctx->vtable.fn_fwrite(
            ctx->vtable.userdata, buf, 1, size, ctx->fp);

    if (n < size && ctx->vtable.fn_ferror(ctx->vtable.userdata, ctx->fp)) {
        mb_file_set_error(file, -errno,
                          "Failed to write file: %s", strerror(errno));
        return errno == EINTR ? MB_FILE_RETRY : MB_FILE_FAILED;
    }

    *bytes_written = n;
    return MB_FILE_OK;
}

static int posix_seek_cb(struct MbFile *file, void *userdata,
                         int64_t offset, int whence,
                         uint64_t *new_offset)
{
    PosixFileCtx *ctx = static_cast<PosixFileCtx *>(userdata);
    off64_t old_pos, new_pos;

    // Get current file position
    old_pos = ctx->vtable.fn_ftello(ctx->vtable.userdata, ctx->fp);
    if (old_pos < 0) {
        mb_file_set_error(file, -errno,
                          "Failed to get file position: %s", strerror(errno));
        return MB_FILE_FAILED;
    }

    // Try to seek
    if (ctx->vtable.fn_fseeko(
            ctx->vtable.userdata, ctx->fp, offset, whence) < 0) {
        mb_file_set_error(file, -errno,
                          "Failed to seek file: %s", strerror(errno));
        return MB_FILE_FAILED;
    }

    // Get new position
    new_pos = ctx->vtable.fn_ftello(ctx->vtable.userdata, ctx->fp);
    if (new_pos < 0) {
        // Try to restore old position
        mb_file_set_error(file, -errno,
                          "Failed to get file position: %s", strerror(errno));
        return ctx->vtable.fn_fseeko(
                ctx->vtable.userdata, ctx->fp, old_pos, SEEK_SET) == 0
                ? MB_FILE_FAILED : MB_FILE_FATAL;
    }

    *new_offset = new_pos;
    return MB_FILE_OK;
}

static int posix_truncate_cb(struct MbFile *file, void *userdata,
                             uint64_t size)
{
    PosixFileCtx *ctx = static_cast<PosixFileCtx *>(userdata);

    int fd = ctx->vtable.fn_fileno(ctx->vtable.userdata, ctx->fp);
    if (fd < 0) {
        mb_file_set_error(file, MB_FILE_ERROR_UNSUPPORTED,
                          "fileno() not supported for fp");
        return MB_FILE_UNSUPPORTED;
    }

    if (ctx->vtable.fn_ftruncate64(ctx->vtable.userdata, fd, size) < 0) {
        mb_file_set_error(file, -errno,
                          "Failed to truncate file: %s", strerror(errno));
        return MB_FILE_FAILED;
    }

    return MB_FILE_OK;
}

int _mb_file_open_FILE(struct MbFile *file, FILE *fp, bool owned,
                       SysVtable *vtable)
{
    if (!vtable
            || !vtable->fn_fstat
            || !vtable->fn_fclose
            || !vtable->fn_ferror
            || !vtable->fn_fileno
            || !vtable->fn_fread
            || !vtable->fn_fseeko
            || !vtable->fn_ftello
            || !vtable->fn_fwrite
            || !vtable->fn_ftruncate64) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Invalid or incomplete vtable");
        return MB_FILE_FATAL;
    }

    PosixFileCtx *ctx = static_cast<PosixFileCtx *>(
            calloc(1, sizeof(PosixFileCtx)));
    if (!ctx) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Failed to allocate PosixFileCtx: %s",
                          strerror(errno));
        return MB_FILE_FATAL;
    }

    ctx->fp = fp;
    ctx->owned = owned;
    ctx->vtable = *vtable;

    return mb_file_open_callbacks(file,
                                  &posix_open_cb,
                                  &posix_close_cb,
                                  &posix_read_cb,
                                  &posix_write_cb,
                                  &posix_seek_cb,
                                  &posix_truncate_cb,
                                  ctx);
}

int mb_file_open_FILE(struct MbFile *file, FILE *fp, bool owned)
{
    SysVtable vtable{};
    _vtable_fill_system_funcs(&vtable);
    return _mb_file_open_FILE(file, fp, owned, &vtable);
}

MB_END_C_DECLS
