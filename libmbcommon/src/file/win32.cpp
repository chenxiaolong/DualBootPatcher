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

#include "mbcommon/file/win32.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>

#include "mbcommon/locale.h"

#include "mbcommon/file/callbacks.h"
#include "mbcommon/file/win32_p.h"

static_assert(sizeof(DWORD) == 4, "DWORD is not 32 bits");

/*!
 * \file mbcommon/file/win32.h
 * \brief Open file with Win32 `HANDLE` API
 */

MB_BEGIN_C_DECLS

static void free_ctx(Win32FileCtx *ctx)
{
    free(ctx->filename);
    free(ctx);
}

LPCWSTR win32_error_string(Win32FileCtx *ctx, DWORD error_code)
{
    LocalFree(ctx->error);

    size_t size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,            // dwFlags
        nullptr,                                        // lpSource
        error_code,                                     // dwMessageId
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),      // dwLanguageId
        reinterpret_cast<LPWSTR>(&ctx->error),          // lpBuffer
        0,                                              // nSize
        nullptr                                         // Arguments
    );

    if (size == 0) {
        ctx->error = nullptr;
        return L"(FormatMessageW failed)";
    }

    return ctx->error;
}

static int win32_open_cb(struct MbFile *file, void *userdata)
{
    Win32FileCtx *ctx = static_cast<Win32FileCtx *>(userdata);

    if (ctx->filename) {
        ctx->handle = ctx->vtable.fn_CreateFileW(
                ctx->vtable.userdata, ctx->filename, ctx->access, ctx->sharing,
                &ctx->sa, ctx->creation, ctx->attrib, nullptr);
        if (ctx->handle == INVALID_HANDLE_VALUE) {
            mb_file_set_error(file, -errno, "Failed to open file: %ls",
                              win32_error_string(ctx, GetLastError()));
            return MB_FILE_FAILED;
        }
    }

    return MB_FILE_OK;
}

static int win32_close_cb(struct MbFile *file, void *userdata)
{
    Win32FileCtx *ctx = static_cast<Win32FileCtx *>(userdata);
    int ret = MB_FILE_OK;

    if (ctx->owned && !ctx->vtable.fn_CloseHandle(
            ctx->vtable.userdata, ctx->handle)) {
        mb_file_set_error(file, -GetLastError(),
                          "Failed to close file: %ls",
                          win32_error_string(ctx, GetLastError()));
        ret = MB_FILE_FAILED;
    }

    free_ctx(ctx);

    return ret;
}

static int win32_read_cb(struct MbFile *file, void *userdata,
                         void *buf, size_t size,
                         size_t *bytes_read)
{
    Win32FileCtx *ctx = static_cast<Win32FileCtx *>(userdata);
    DWORD n = 0;

    if (size > UINT_MAX) {
        size = UINT_MAX;
    }

    bool ret = ctx->vtable.fn_ReadFile(
        ctx->vtable.userdata,   // userdata
        ctx->handle,            // hFile
        buf,                    // lpBuffer
        size,                   // nNumberOfBytesToRead
        &n,                     // lpNumberOfBytesRead
        nullptr                 // lpOverlapped
    );

    if (!ret) {
        mb_file_set_error(file, -GetLastError(),
                          "Failed to read file: %ls",
                          win32_error_string(ctx, GetLastError()));
        return MB_FILE_FAILED;
    }

    *bytes_read = n;
    return MB_FILE_OK;
}

static int win32_seek_cb(struct MbFile *file, void *userdata,
                         int64_t offset, int whence,
                         uint64_t *new_offset)
{
    Win32FileCtx *ctx = static_cast<Win32FileCtx *>(userdata);
    DWORD move_method;
    LARGE_INTEGER pos;
    LARGE_INTEGER new_pos;

    switch (whence) {
    case SEEK_CUR:
        move_method = FILE_CURRENT;
        break;
    case SEEK_SET:
        move_method = FILE_BEGIN;
        break;
    case SEEK_END:
        move_method = FILE_END;
        break;
    default:
        mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                          "Invalid whence argument: %d", whence);
        return MB_FILE_FAILED;
    }

    pos.QuadPart = offset;

    bool ret = ctx->vtable.fn_SetFilePointerEx(
        ctx->vtable.userdata,   // userdata
        ctx->handle,            // hFile
        pos,                    // liDistanceToMove
        &new_pos,               // lpNewFilePointer
        move_method             // dwMoveMethod
    );

    if (!ret) {
        mb_file_set_error(file, -GetLastError(),
                          "Failed to seek file: %ls",
                          win32_error_string(ctx, GetLastError()));
        return MB_FILE_FAILED;
    }

    *new_offset = new_pos.QuadPart;
    return MB_FILE_OK;
}

static int win32_write_cb(struct MbFile *file, void *userdata,
                          const void *buf, size_t size,
                          size_t *bytes_written)
{
    Win32FileCtx *ctx = static_cast<Win32FileCtx *>(userdata);
    DWORD n = 0;

    // We have to seek manually in append mode because the Win32 API has no
    // native append mode.
    if (ctx->append) {
        uint64_t pos;
        int seek_ret = win32_seek_cb(file, userdata, 0, SEEK_END, &pos);
        if (seek_ret != MB_FILE_OK) {
            return seek_ret;
        }
    }

    if (size > UINT_MAX) {
        size = UINT_MAX;
    }

    bool ret = ctx->vtable.fn_WriteFile(
        ctx->vtable.userdata,   // userdata
        ctx->handle,            // hFile
        buf,                    // lpBuffer
        size,                   // nNumberOfBytesToWrite
        &n,                     // lpNumberOfBytesWritten
        nullptr                 // lpOverlapped
    );

    if (!ret) {
        mb_file_set_error(file, -GetLastError(),
                          "Failed to write file: %ls",
                          win32_error_string(ctx, GetLastError()));
        return MB_FILE_FAILED;
    }

    *bytes_written = n;
    return MB_FILE_OK;
}

static int win32_truncate_cb(struct MbFile *file, void *userdata,
                              uint64_t size)
{
    Win32FileCtx *ctx = static_cast<Win32FileCtx *>(userdata);
    int ret = MB_FILE_OK, ret2;
    uint64_t current_pos;
    uint64_t temp;

    // Get current position
    ret2 = win32_seek_cb(file, userdata, 0, SEEK_CUR, &current_pos);
    if (ret2 != MB_FILE_OK) {
        return ret2;
    }

    // Move to new position
    ret2 = win32_seek_cb(file, userdata, size, SEEK_SET, &temp);
    if (ret2 != MB_FILE_OK) {
        return ret2;
    }

    // Truncate
    if (!ctx->vtable.fn_SetEndOfFile(ctx->vtable.userdata, ctx->handle)) {
        mb_file_set_error(file, -GetLastError(),
                          "Failed to set EOF position: %ls",
                          win32_error_string(ctx, GetLastError()));
        ret = MB_FILE_FAILED;
    }

    // Move back to initial position
    ret2 = win32_seek_cb(file, userdata, current_pos, SEEK_SET, &temp);
    if (ret2 != MB_FILE_OK) {
        // We can't guarantee the file position so the handle shouldn't be used
        // anymore
        ret = MB_FILE_FATAL;
    }

    return ret;
}

static bool check_vtable(SysVtable *vtable, bool needs_open)
{
    return vtable
            && vtable->fn_CloseHandle
            && (needs_open ? !!vtable->fn_CreateFileW : true)
            && vtable->fn_ReadFile
            && vtable->fn_SetEndOfFile
            && vtable->fn_SetFilePointerEx
            && vtable->fn_WriteFile;
}

static Win32FileCtx * create_ctx(struct MbFile *file, SysVtable *vtable,
                                 bool needs_open)
{
    if (!check_vtable(vtable, needs_open)) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Invalid or incomplete vtable");
        return nullptr;
    }

    Win32FileCtx *ctx = static_cast<Win32FileCtx *>(
            calloc(1, sizeof(Win32FileCtx)));
    if (!ctx) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Failed to allocate Win32FileCtx: %s",
                          strerror(errno));
        return nullptr;
    }

    ctx->vtable = *vtable;

    return ctx;
}

static int open_ctx(struct MbFile *file, Win32FileCtx *ctx)
{
    return mb_file_open_callbacks(file,
                                  &win32_open_cb,
                                  &win32_close_cb,
                                  &win32_read_cb,
                                  &win32_write_cb,
                                  &win32_seek_cb,
                                  &win32_truncate_cb,
                                  ctx);
}

static bool convert_mode(Win32FileCtx *ctx, int mode)
{
    DWORD access = 0;
    // Match open()/_wopen() behavior
    DWORD sharing = FILE_SHARE_READ | FILE_SHARE_WRITE;
    SECURITY_ATTRIBUTES sa;
    DWORD creation = 0;
    DWORD attrib = 0;
    // Win32 does not have a native append mode
    bool append = false;

    switch (mode) {
    case MB_FILE_OPEN_READ_ONLY:
        access = GENERIC_READ;
        creation = OPEN_EXISTING;
        break;
    case MB_FILE_OPEN_READ_WRITE:
        access = GENERIC_READ | GENERIC_WRITE;
        creation = OPEN_EXISTING;
        break;
    case MB_FILE_OPEN_WRITE_ONLY:
        access = GENERIC_WRITE;
        creation = CREATE_ALWAYS;
        break;
    case MB_FILE_OPEN_READ_WRITE_TRUNC:
        access = GENERIC_READ | GENERIC_WRITE;
        creation = CREATE_ALWAYS;
        break;
    case MB_FILE_OPEN_APPEND:
        access = GENERIC_WRITE;
        creation = OPEN_ALWAYS;
        append = true;
        break;
    case MB_FILE_OPEN_READ_APPEND:
        access = GENERIC_READ | GENERIC_WRITE;
        creation = OPEN_ALWAYS;
        append = true;
        break;
    default:
        return false;
    }

    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = false;

    ctx->access = access;
    ctx->sharing = sharing;
    ctx->sa = sa;
    ctx->creation = creation;
    ctx->attrib = attrib;
    ctx->append = append;

    return true;
}

int _mb_file_open_HANDLE(SysVtable *vtable, struct MbFile *file, HANDLE handle,
                         bool owned, bool append)
{
    Win32FileCtx *ctx = create_ctx(file, vtable, false);
    if (!ctx) {
        return MB_FILE_FATAL;
    }

    ctx->handle = handle;
    ctx->owned = owned;
    ctx->append = append;

    return open_ctx(file, ctx);
}

int _mb_file_open_HANDLE_filename(SysVtable *vtable, struct MbFile *file,
                                  const char *filename, int mode)
{
    Win32FileCtx *ctx = create_ctx(file, vtable, false);
    if (!ctx) {
        return MB_FILE_FATAL;
    }

    ctx->owned = true;

    ctx->filename = mb::mbs_to_wcs(filename);
    if (!ctx->filename) {
        mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                          "Failed to convert MBS filename or mode to WCS");
        free_ctx(ctx);
        return MB_FILE_FATAL;
    }

    if (!convert_mode(ctx, mode)) {
        mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                          "Invalid mode: %d", mode);
        free_ctx(ctx);
        return MB_FILE_FATAL;
    }

    return open_ctx(file, ctx);
}

int _mb_file_open_HANDLE_filename_w(SysVtable *vtable, struct MbFile *file,
                                    const wchar_t *filename, int mode)
{
    Win32FileCtx *ctx = create_ctx(file, vtable, false);
    if (!ctx) {
        return MB_FILE_FATAL;
    }

    ctx->owned = true;

    ctx->filename = wcsdup(filename);
    if (!ctx->filename) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Failed to allocate string: %s", strerror(errno));
        free_ctx(ctx);
        return MB_FILE_FATAL;
    }

    if (!convert_mode(ctx, mode)) {
        mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                          "Invalid mode: %d", mode);
        free_ctx(ctx);
        return MB_FILE_FATAL;
    }

    return open_ctx(file, ctx);
}

/*!
 * Open MbFile handle from Win32 `HANDLE`.
 *
 * If \p owned is true, then the MbFile handle will take ownership of the
 * Win32 `HANDLE`. In other words, the Win32 `HANDLE` will be closed when the
 * MbFile handle is closed.
 *
 * The \p append parameter exists because the Win32 API does not have a native
 * append mode.
 *
 * \param file MbFile handle
 * \param handle Win32 `HANDLE`
 * \param owned Whether the Win32 `HANDLE` should be owned by the MbFile handle
 * \param append Whether append mode should be enabled
 *
 * \return
 *   * #MB_FILE_OK if the Win32 `HANDLE` was successfully opened
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_open_HANDLE(struct MbFile *file,
                        HANDLE handle, bool owned, bool append)
{
    SysVtable vtable{};
    _vtable_fill_system_funcs(&vtable);
    return _mb_file_open_HANDLE(&vtable, file, handle, owned, append);
}

/*!
 * Open MbFile handle from a multi-byte filename.
 *
 * \p filename is converted to WCS using mb::mbs_to_wcs() before being used.
 *
 * \param file MbFile handle
 * \param filename MBS filename
 * \param mode Open mode (\ref MbFileOpenMode)
 *
 * \return
 *   * #MB_FILE_OK if the file was successfully opened
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_open_HANDLE_filename(struct MbFile *file, const char *filename,
                                 int mode)
{
    SysVtable vtable{};
    _vtable_fill_system_funcs(&vtable);
    return _mb_file_open_HANDLE_filename(&vtable, file, filename, mode);
}

/*!
 * Open MbFile handle from a wide-character filename.
 *
 * \p filename is used directly without any conversions.
 *
 * \param file MbFile handle
 * \param filename WCS filename
 * \param mode Open mode (\ref MbFileOpenMode)
 *
 * \return
 *   * #MB_FILE_OK if the file was successfully opened
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_open_HANDLE_filename_w(struct MbFile *file, const wchar_t *filename,
                                   int mode)
{
    SysVtable vtable{};
    _vtable_fill_system_funcs(&vtable);
    return _mb_file_open_HANDLE_filename_w(&vtable, file, filename, mode);
}

MB_END_C_DECLS
