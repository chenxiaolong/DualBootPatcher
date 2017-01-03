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

#include "mbcommon/file/win32.h"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>

#include "mbcommon/file/callbacks.h"
#include "mbcommon/file/win32_p.h"

static_assert(sizeof(DWORD) == 4, "DWORD is not 32 bits");

MB_BEGIN_C_DECLS

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
    (void) file;
    (void) userdata;
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

    free(ctx);

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
                          "Failed to read file: %s",
                          win32_error_string(ctx, GetLastError()));
        return MB_FILE_FAILED;
    }

    *bytes_read = n;
    return MB_FILE_OK;
}

static int win32_write_cb(struct MbFile *file, void *userdata,
                          const void *buf, size_t size,
                          size_t *bytes_written)
{
    Win32FileCtx *ctx = static_cast<Win32FileCtx *>(userdata);
    DWORD n = 0;

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
                          "Failed to write file: %s",
                          win32_error_string(ctx, GetLastError()));
        return MB_FILE_FAILED;
    }

    *bytes_written = n;
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
                          "Failed to seek file: %s",
                          win32_error_string(ctx, GetLastError()));
        return MB_FILE_FAILED;
    }

    *new_offset = new_pos.QuadPart;
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
                          "Failed to set EOF position: %s",
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

int _mb_file_open_HANDLE(struct MbFile *file, HANDLE handle, bool owned,
                         SysVtable *vtable)
{
    if (!vtable
            || !vtable->fn_CloseHandle
            || !vtable->fn_ReadFile
            || !vtable->fn_SetEndOfFile
            || !vtable->fn_SetFilePointerEx
            || !vtable->fn_WriteFile) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Invalid or incomplete vtable");
        return MB_FILE_FATAL;
    }

    Win32FileCtx *ctx = static_cast<Win32FileCtx *>(
            calloc(1, sizeof(Win32FileCtx)));
    if (!ctx) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Failed to allocate Win32FileCtx: %s",
                          strerror(errno));
        return MB_FILE_FATAL;
    }

    ctx->handle = handle;
    ctx->owned = owned;
    ctx->vtable = *vtable;

    return mb_file_open_callbacks(file,
                                  &win32_open_cb,
                                  &win32_close_cb,
                                  &win32_read_cb,
                                  &win32_write_cb,
                                  &win32_seek_cb,
                                  &win32_truncate_cb,
                                  ctx);
}

int mb_file_open_HANDLE(struct MbFile *file,
                        HANDLE handle, bool owned)
{
    SysVtable vtable{};
    _vtable_fill_system_funcs(&vtable);
    return _mb_file_open_HANDLE(file, handle, owned, &vtable);
}

MB_END_C_DECLS
