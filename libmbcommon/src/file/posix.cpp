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

#include "mbcommon/file/posix.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/locale.h"

#include "mbcommon/file/callbacks.h"
#include "mbcommon/file/posix_p.h"

#ifndef __ANDROID__
static_assert(sizeof(off_t) > 4, "Not compiling with LFS support!");
#endif

/*!
 * \file mbcommon/file/posix.h
 * \brief Open file with POSIX stdio `FILE *` API
 */

MB_BEGIN_C_DECLS

static void free_ctx(PosixFileCtx *ctx)
{
    free(ctx->filename);
    free(ctx);
}

static int posix_open_cb(struct MbFile *file, void *userdata)
{
    PosixFileCtx *ctx = static_cast<PosixFileCtx *>(userdata);
    struct stat sb;
    int fd;

    if (ctx->filename) {
#ifdef _WIN32
        ctx->fp = ctx->vtable.fn_wfopen(
#else
        ctx->fp = ctx->vtable.fn_fopen(
#endif
                ctx->vtable.userdata, ctx->filename, ctx->mode);
        if (!ctx->fp) {
            mb_file_set_error(file, -errno, "Failed to open file: %s",
                              strerror(errno));
            return MB_FILE_FAILED;
        }
    }

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

        // Enable seekability based on file type because lseek(fd, 0, SEEK_CUR)
        // does not always fail on non-seekable file descriptors
        if (S_ISREG(sb.st_mode)
#ifdef __linux__
                || S_ISBLK(sb.st_mode)
#endif
        ) {
            ctx->can_seek = true;
        }
    }

    return MB_FILE_OK;
}

static int posix_close_cb(struct MbFile *file, void *userdata)
{
    PosixFileCtx *ctx = static_cast<PosixFileCtx *>(userdata);
    int ret = MB_FILE_OK;

    if (ctx->owned && ctx->fp && ctx->vtable.fn_fclose(
            ctx->vtable.userdata, ctx->fp) == EOF) {
        mb_file_set_error(file, -errno,
                          "Failed to close file: %s", strerror(errno));
        ret = MB_FILE_FAILED;
    }

    // Clean up resources
    free_ctx(ctx);

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

    if (!ctx->can_seek) {
        mb_file_set_error(file, MB_FILE_ERROR_UNSUPPORTED,
                          "Seek not supported: %s", strerror(errno));
        return MB_FILE_UNSUPPORTED;
    }

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

static bool check_vtable(SysVtable *vtable, bool needs_fopen)
{
    return vtable
            && vtable->fn_fstat
            && vtable->fn_fclose
            && vtable->fn_ferror
            && vtable->fn_fileno
#ifdef _WIN32
            && (needs_fopen ? !!vtable->fn_wfopen : true)
#else
            && (needs_fopen ? !!vtable->fn_fopen : true)
#endif
            && vtable->fn_fread
            && vtable->fn_fseeko
            && vtable->fn_ftello
            && vtable->fn_fwrite
            && vtable->fn_ftruncate64;
}

static PosixFileCtx * create_ctx(struct MbFile *file, SysVtable *vtable,
                                 bool needs_fopen)
{
    if (!check_vtable(vtable, needs_fopen)) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Invalid or incomplete vtable");
        return nullptr;
    }

    PosixFileCtx *ctx = static_cast<PosixFileCtx *>(
            calloc(1, sizeof(PosixFileCtx)));
    if (!ctx) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Failed to allocate PosixFileCtx: %s",
                          strerror(errno));
        return nullptr;
    }

    ctx->vtable = *vtable;

    return ctx;
}

static int open_ctx(struct MbFile *file, PosixFileCtx *ctx)
{
    return mb_file_open_callbacks(file,
                                  &posix_open_cb,
                                  &posix_close_cb,
                                  &posix_read_cb,
                                  &posix_write_cb,
                                  &posix_seek_cb,
                                  &posix_truncate_cb,
                                  ctx);
}

#ifdef _WIN32
static const wchar_t * convert_mode(int mode)
{
    switch (mode) {
    case MB_FILE_OPEN_READ_ONLY:
        return L"rbN";
    case MB_FILE_OPEN_READ_WRITE:
        return L"r+bN";
    case MB_FILE_OPEN_WRITE_ONLY:
        return L"wbN";
    case MB_FILE_OPEN_READ_WRITE_TRUNC:
        return L"w+bN";
    case MB_FILE_OPEN_APPEND:
        return L"abN";
    case MB_FILE_OPEN_READ_APPEND:
        return L"a+bN";
    default:
        return nullptr;
    }
}
#else
static const char * convert_mode(int mode)
{
    switch (mode) {
    case MB_FILE_OPEN_READ_ONLY:
        return "rbe";
    case MB_FILE_OPEN_READ_WRITE:
        return "r+be";
    case MB_FILE_OPEN_WRITE_ONLY:
        return "wbe";
    case MB_FILE_OPEN_READ_WRITE_TRUNC:
        return "w+be";
    case MB_FILE_OPEN_APPEND:
        return "abe";
    case MB_FILE_OPEN_READ_APPEND:
        return "a+be";
    default:
        return nullptr;
    }
}
#endif

int _mb_file_open_FILE(SysVtable *vtable, struct MbFile *file, FILE *fp,
                       bool owned)
{
    PosixFileCtx *ctx = create_ctx(file, vtable, false);
    if (!ctx) {
        return MB_FILE_FATAL;
    }

    ctx->fp = fp;
    ctx->owned = owned;

    return open_ctx(file, ctx);
}

int _mb_file_open_FILE_filename(SysVtable *vtable, struct MbFile *file,
                                const char *filename, int mode)
{
    PosixFileCtx *ctx = create_ctx(file, vtable, true);
    if (!ctx) {
        return MB_FILE_FATAL;
    }

    ctx->owned = true;

#ifdef _WIN32
    ctx->filename = mb::mbs_to_wcs(filename);
    if (!ctx->filename) {
        mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                          "Failed to convert MBS filename to WCS");
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

    ctx->mode = convert_mode(mode);
    if (!ctx->mode) {
        mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                          "Invalid mode: %d", mode);
        free_ctx(ctx);
        return MB_FILE_FATAL;
    }

    return open_ctx(file, ctx);
}

int _mb_file_open_FILE_filename_w(SysVtable *vtable, struct MbFile *file,
                                  const wchar_t *filename, int mode)
{
    PosixFileCtx *ctx = create_ctx(file, vtable, true);
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
                          "Failed to convert WCS filename to MBS");
        free_ctx(ctx);
        return MB_FILE_FATAL;
    }
#endif

    ctx->mode = convert_mode(mode);
    if (!ctx->mode) {
        mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                          "Invalid mode: %d", mode);
        free_ctx(ctx);
        return MB_FILE_FATAL;
    }

    return open_ctx(file, ctx);
}

/*!
 * Open MbFile handle from `FILE *`.
 *
 * If \p owned is true, then the MbFile handle will take ownership of the
 * `FILE *` instance. In other words, the `FILE *` instance will be closed when
 * the MbFile handle is closed.
 *
 * \param file MbFile handle
 * \param fp `FILE *` instance
 * \param owned Whether the `FILE *` instance should be owned by the MbFile
 *              handle
 *
 * \return
 *   * #MB_FILE_OK if the `FILE *` instance was successfully opened
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_open_FILE(struct MbFile *file, FILE *fp, bool owned)
{
    SysVtable vtable{};
    _vtable_fill_system_funcs(&vtable);
    return _mb_file_open_FILE(&vtable, file, fp, owned);
}

/*!
 * Open MbFile handle from a multi-byte filename.
 *
 * On Unix-like systems, \p filename is directly passed to `fopen()`. On Windows
 * systems, \p filename is converted to WCS using mb::mbs_to_wcs() before being
 * passed to `_wfopen()`.
 *
 * \param file MbFile handle
 * \param filename MBS filename
 * \param mode Open mode (\ref MbFileOpenMode)
 *
 * \return
 *   * #MB_FILE_OK if the file was successfully opened
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_open_FILE_filename(struct MbFile *file, const char *filename,
                               int mode)
{
    SysVtable vtable{};
    _vtable_fill_system_funcs(&vtable);
    return _mb_file_open_FILE_filename(&vtable, file, filename, mode);
}

/*!
 * Open MbFile handle from a wide-character filename.
 *
 * On Unix-like systems, \p filename is converted to MBS using mb::wcs_to_mbs()
 * before being passed to `fopen()`. On Windows systems, \p filename is directly
 * passed to `_wfopen()`.
 *
 * \param file MbFile handle
 * \param filename WCS filename
 * \param mode Open mode (\ref MbFileOpenMode)
 *
 * \return
 *   * #MB_FILE_OK if the file was successfully opened
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_open_FILE_filename_w(struct MbFile *file, const wchar_t *filename,
                                 int mode)
{
    SysVtable vtable{};
    _vtable_fill_system_funcs(&vtable);
    return _mb_file_open_FILE_filename_w(&vtable, file, filename, mode);
}

MB_END_C_DECLS
