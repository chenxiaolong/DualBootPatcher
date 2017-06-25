/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file/memory.h"

#include <algorithm>

#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "mbcommon/file/callbacks.h"
#include "mbcommon/file/memory_p.h"
#include "mbcommon/string.h"

/*!
 * \file mbcommon/file/memory.h
 * \brief Open file from memory
 */

MB_BEGIN_C_DECLS

static void free_ctx(MemoryFileCtx *ctx)
{
    free(ctx);
}

static int memory_close_cb(struct MbFile *file, void *userdata)
{
    (void) file;
    MemoryFileCtx *const ctx = static_cast<MemoryFileCtx *>(userdata);

    free_ctx(ctx);
    return MB_FILE_OK;
}

static int memory_read_cb(struct MbFile *file, void *userdata,
                          void *buf, size_t size, size_t *bytes_read)
{
    (void) file;
    MemoryFileCtx *const ctx = static_cast<MemoryFileCtx *>(userdata);

    size_t to_read = 0;
    if (ctx->pos < ctx->size) {
        to_read = std::min(ctx->size - ctx->pos, size);
    }

    memcpy(buf, static_cast<char *>(ctx->data) + ctx->pos, to_read);
    ctx->pos += to_read;

    *bytes_read = to_read;
    return MB_FILE_OK;
}

static int memory_write_cb(struct MbFile *file, void *userdata,
                           const void *buf, size_t size, size_t *bytes_written)
{
    MemoryFileCtx *const ctx = static_cast<MemoryFileCtx *>(userdata);

    if (ctx->pos > SIZE_MAX - size) {
        mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                          "Write would overflow size_t");
        return MB_FILE_FAILED;
    }

    size_t desired_size = ctx->pos + size;
    size_t to_write = size;

    if (desired_size > ctx->size) {
        if (ctx->fixed_size) {
            to_write = ctx->pos <= ctx->size ? ctx->size - ctx->pos : 0;
        } else {
            // Enlarge buffer
            void *new_data = realloc(ctx->data, desired_size);
            if (!new_data) {
                mb_file_set_error(file, -errno,
                                  "Failed to enlarge buffer: %s",
                                  strerror(errno));
                return MB_FILE_FAILED;
            }

            // Zero-initialize new space
            memset(static_cast<char *>(new_data) + ctx->size, 0,
                   desired_size - ctx->size);

            ctx->data = new_data;
            ctx->size = desired_size;
            if (ctx->data_ptr) {
                *ctx->data_ptr = ctx->data;
            }
            if (ctx->size_ptr) {
                *ctx->size_ptr = ctx->size;
            }
        }
    }

    memcpy(static_cast<char *>(ctx->data) + ctx->pos, buf, to_write);
    ctx->pos += to_write;

    *bytes_written = to_write;
    return MB_FILE_OK;
}

static int memory_seek_cb(struct MbFile *file, void *userdata,
                          int64_t offset, int whence, uint64_t *new_offset)
{
    MemoryFileCtx *const ctx = static_cast<MemoryFileCtx *>(userdata);

    switch (whence) {
    case SEEK_SET:
        if (offset < 0 || static_cast<uint64_t>(offset) > SIZE_MAX) {
            mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                              "Invalid SEEK_SET offset %" PRId64,
                              offset);
            return MB_FILE_FAILED;
        }
        *new_offset = ctx->pos = offset;
        break;
    case SEEK_CUR:
        if ((offset < 0 && static_cast<uint64_t>(-offset) > ctx->pos)
                || (offset > 0 && static_cast<uint64_t>(offset)
                        > SIZE_MAX - ctx->pos)) {
            mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                              "Invalid SEEK_CUR offset %" PRId64
                              " for position %" MB_PRIzu,
                              offset, ctx->pos);
            return MB_FILE_FAILED;
        }
        *new_offset = ctx->pos += offset;
        break;
    case SEEK_END:
        if ((offset < 0 && static_cast<size_t>(-offset) > ctx->size)
                || (offset > 0 && static_cast<uint64_t>(offset)
                        > SIZE_MAX - ctx->size)) {
            mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                              "Invalid SEEK_END offset %" PRId64
                              " for file of size %" MB_PRIzu,
                              offset, ctx->size);
            return MB_FILE_FAILED;
        }
        *new_offset = ctx->pos = ctx->size + offset;
        break;
    default:
        mb_file_set_error(file, MB_FILE_ERROR_INVALID_ARGUMENT,
                          "Invalid whence argument: %d", whence);
        return MB_FILE_FAILED;
    }

    return MB_FILE_OK;
}

static int memory_truncate_cb(struct MbFile *file, void *userdata,
                              uint64_t size)
{
    MemoryFileCtx *const ctx = static_cast<MemoryFileCtx *>(userdata);

    if (ctx->fixed_size) {
        mb_file_set_error(file, MB_FILE_ERROR_UNSUPPORTED,
                          "Cannot truncate fixed buffer");
        return MB_FILE_UNSUPPORTED;
    } else {
        void *new_data = realloc(ctx->data, size);
        if (!new_data) {
            mb_file_set_error(file, -errno,
                              "Failed to resize buffer: %s",
                              strerror(errno));
            return MB_FILE_FAILED;
        }

        // Zero-initialize new space
        if (size > ctx->size) {
            memset(static_cast<char *>(new_data) + ctx->size, 0,
                   size - ctx->size);
        }

        ctx->data = new_data;
        ctx->size = size;
        if (ctx->data_ptr) {
            *ctx->data_ptr = ctx->data;
        }
        if (ctx->size_ptr) {
            *ctx->size_ptr = ctx->size;
        }
    }

    return MB_FILE_OK;
}

static MemoryFileCtx * create_ctx(struct MbFile *file)
{
    MemoryFileCtx *ctx = static_cast<MemoryFileCtx *>(
            calloc(1, sizeof(MemoryFileCtx)));
    if (!ctx) {
        mb_file_set_error(file, MB_FILE_ERROR_INTERNAL_ERROR,
                          "Failed to allocate MemoryFileCtx: %s",
                          strerror(errno));
        return nullptr;
    }

    return ctx;
}

static int open_ctx(struct MbFile *file, MemoryFileCtx *ctx)
{
    return mb_file_open_callbacks(file,
                                  nullptr,
                                  &memory_close_cb,
                                  &memory_read_cb,
                                  &memory_write_cb,
                                  &memory_seek_cb,
                                  &memory_truncate_cb,
                                  ctx);
}

/*!
 * Open MbFile handle from fixed size memory buffer.
 *
 * \param file MbFile handle
 * \param buf Data buffer
 * \param size Size of data buffer
 *
 * \return
 *   * #MB_FILE_OK if the buffer is successfully opened
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_open_memory_static(struct MbFile *file,
                               const void *buf, size_t size)
{
    MemoryFileCtx *ctx = create_ctx(file);
    if (!ctx) {
        return MB_FILE_FATAL;
    }

    ctx->data = const_cast<void *>(buf);
    ctx->size = size;
    ctx->fixed_size = true;

    return open_ctx(file, ctx);
}

/*!
 * Open MbFile handle from dynamically sized memory buffer.
 *
 * \param[in] file MbFile handle
 * \param[in,out] buf_ptr Pointer to data buffer
 * \param[in,out] size_ptr Pointer to size of data buffer
 *
 * \return
 *   * #MB_FILE_OK if the buffer is successfully opened
 *   * \<= #MB_FILE_WARN if an error occurs
 */
int mb_file_open_memory_dynamic(struct MbFile *file,
                                void **buf_ptr, size_t *size_ptr)
{
    MemoryFileCtx *ctx = create_ctx(file);
    if (!ctx) {
        return MB_FILE_FATAL;
    }

    ctx->data = *buf_ptr;
    ctx->size = *size_ptr;
    ctx->data_ptr = buf_ptr;
    ctx->size_ptr = size_ptr;

    return open_ctx(file, ctx);
}

MB_END_C_DECLS
