/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mbcommon/file/fd.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/locale.h"

#include "mbcommon/file/callbacks.h"
#include "mbcommon/file/fd_p.h"

#define DEFAULT_MODE \
    (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

/*!
 * \file mbcommon/file/fd.h
 * \brief Open file with POSIX file descriptors API
 */

MB_BEGIN_C_DECLS

static void free_ctx(FdFileCtx *ctx)
{
    free(ctx->filename);
    free(ctx);
}

static int fd_open_cb(struct MbFile *file, void *userdata)
{
    FdFileCtx *ctx = static_cast<FdFileCtx *>(userdata);
    struct stat sb;

    if (ctx->filename) {
#ifdef _WIN32
        ctx->fd = ctx->vtable.fn_wopen(
#else
        ctx->fd = ctx->vtable.fn_open(
#endif
                ctx->vtable.userdata, ctx->filename, ctx->flags, DEFAULT_MODE);
        if (ctx->fd < 0) {
            mb_file_set_error(file, -errno, "Failed to open file: %s",
                              strerror(errno));
            return MB_FILE_FAILED;
        }
    }

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

    if (ctx->owned && ctx->fd >= 0 && ctx->vtable.fn_close(
            ctx->vtable.userdata, ctx->fd) < 0) {
        mb_file_set_error(file, -errno,
                          "Failed to close file: %s", strerror(errno));
        ret = MB_FILE_FAILED;
    }

    free_ctx(ctx);

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

static bool check_vtable(SysVtable *vtable, bool needs_open)
{
    return vtable
#ifdef _WIN32
            && (needs_open ? !!vtable->fn_wopen : true)
#else
            && (needs_open ? !!vtable->fn_open : true)
#endif
            && vtable->fn_fstat
            && vtable->fn_close
            && vtable->fn_ftruncate64
            && vtable->fn_lseek64
            && vtable->fn_read
            && vtable->fn_write;
}

static FdFileCtx * create_ctx(struct MbFile *file, SysVtable *vtable,
                              bool needs_open)
{
    if (!check_vtable(vtable, needs_open)) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Invalid or incomplete vtable");
        return nullptr;
    }

    FdFileCtx *ctx = static_cast<FdFileCtx *>(calloc(1, sizeof(FdFileCtx)));
    if (!ctx) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Failed to allocate FdFileCtx: %s",
                          strerror(errno));
        return nullptr;
    }

    ctx->vtable = *vtable;

    return ctx;
}

static int open_ctx(struct MbFile *file, FdFileCtx *ctx)
{
    return mb_file_open_callbacks(file,
                                  &fd_open_cb,
                                  &fd_close_cb,
                                  &fd_read_cb,
                                  &fd_write_cb,
                                  &fd_seek_cb,
                                  &fd_truncate_cb,
                                  ctx);
}

static int convert_mode(int mode)
{
    int ret = 0;

#ifdef _WIN32
    ret |= O_NOINHERIT | O_BINARY;
#else
    ret |= O_CLOEXEC;
#endif

    switch (mode) {
    case MB_FILE_OPEN_READ_ONLY:
        ret |= O_RDONLY;
        break;
    case MB_FILE_OPEN_READ_WRITE:
        ret |= O_RDWR;
        break;
    case MB_FILE_OPEN_WRITE_ONLY:
        ret |= O_WRONLY | O_CREAT | O_TRUNC;
        break;
    case MB_FILE_OPEN_READ_WRITE_TRUNC:
        ret |= O_RDWR | O_CREAT | O_TRUNC;
        break;
    case MB_FILE_OPEN_APPEND:
        ret |= O_WRONLY | O_CREAT | O_APPEND;
        break;
    case MB_FILE_OPEN_READ_APPEND:
        ret |= O_RDWR | O_CREAT | O_APPEND;
        break;
    default:
        ret = -1;
        break;
    }

    return ret;
}

int _mb_file_open_fd(SysVtable *vtable, struct MbFile *file, int fd, bool owned)
{
    FdFileCtx *ctx = create_ctx(file, vtable, false);
    if (!ctx) {
        return MB_FILE_FATAL;
    }

    ctx->fd = fd;
    ctx->owned = owned;

    return open_ctx(file, ctx);
}

int _mb_file_open_fd_filename(SysVtable *vtable, struct MbFile *file,
                              const char *filename, int mode)
{
    FdFileCtx *ctx = create_ctx(file, vtable, false);
    if (!ctx) {
        return MB_FILE_FATAL;
    }

    ctx->owned = true;

#ifdef _WIN32
    ctx->filename = mb::mbs_to_wcs(filename);
    if (!ctx->filename) {
        mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                          "Failed to convert MBS filename or mode to WCS");
        free_ctx(ctx);
        return MB_FILE_FATAL;
    }
#else
    ctx->filename = strdup(filename);
    if (!ctx->filename) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Failed to allocate string: %s", strerror(errno));
        free_ctx(ctx);
        return MB_FILE_FATAL;
    }
#endif

    ctx->flags = convert_mode(mode);
    if (ctx->flags < 0) {
        mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                          "Invalid mode: %d", mode);
        free_ctx(ctx);
        return MB_FILE_FATAL;
    }

    return open_ctx(file, ctx);
}

int _mb_file_open_fd_filename_w(SysVtable *vtable, struct MbFile *file,
                                const wchar_t *filename, int mode)
{
    FdFileCtx *ctx = create_ctx(file, vtable, false);
    if (!ctx) {
        return MB_FILE_FATAL;
    }

    ctx->owned = true;

#ifdef _WIN32
    ctx->filename = wcsdup(filename);
    if (!ctx->filename) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Failed to allocate string: %s", strerror(errno));
        free_ctx(ctx);
        return MB_FILE_FATAL;
    }
#else
    ctx->filename = mb::wcs_to_mbs(filename);
    if (!ctx->filename) {
        mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                          "Failed to convert WCS filename or mode to MBS");
        free_ctx(ctx);
        return MB_FILE_FATAL;
    }
#endif

    ctx->flags = convert_mode(mode);
    if (ctx->flags < 0) {
        mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                          "Invalid mode: %d", mode);
        free_ctx(ctx);
        return MB_FILE_FATAL;
    }

    return open_ctx(file, ctx);
}

/*!
 * Open MbFile handle from file descriptor.
 *
 * If \p owned is true, then the MbFile handle will take ownership of the file
 * descriptor. In other words, the file descriptor will be closed when the
 * MbFile handle is closed.
 *
 * \param file MbFile handle
 * \param fd File descriptor
 * \param owned Whether the file descriptor should be owned by the MbFile
 *              handle
 *
 * \return
 *   * #MB_FILE_OK if the file descriptor was successfully opened
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_open_fd(struct MbFile *file, int fd, bool owned)
{
    SysVtable vtable{};
    _vtable_fill_system_funcs(&vtable);
    return _mb_file_open_fd(&vtable, file, fd, owned);
}

/*!
 * Open MbFile handle from a multi-byte filename.
 *
 * On Unix-like systems, \p filename is directly passed to `open()`. On Windows
 * systems, \p filename is converted to WCS using mb::mbs_to_wcs() before being
 * passed to `_wopen()`.
 *
 * \param file MbFile handle
 * \param filename MBS filename
 * \param mode Open mode (\ref MbFileOpenMode)
 *
 * \return
 *   * #MB_FILE_OK if the file was successfully opened
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_open_fd_filename(struct MbFile *file, const char *filename,
                             int mode)
{
    SysVtable vtable{};
    _vtable_fill_system_funcs(&vtable);
    return _mb_file_open_fd_filename(&vtable, file, filename, mode);
}

/*!
 * Open MbFile handle from a wide-character filename.
 *
 * On Unix-like systems, \p filename is converted to MBS using mb::wcs_to_mbs()
 * before being passed to `open()`. On Windows systems, \p filename is directly
 * passed to `_wopen()`.
 *
 * \param file MbFile handle
 * \param filename WCS filename
 * \param mode Open mode (\ref MbFileOpenMode)
 *
 * \return
 *   * #MB_FILE_OK if the file was successfully opened
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_open_fd_filename_w(struct MbFile *file, const wchar_t *filename,
                               int mode)
{
    SysVtable vtable{};
    _vtable_fill_system_funcs(&vtable);
    return _mb_file_open_fd_filename_w(&vtable, file, filename, mode);
}

MB_END_C_DECLS
