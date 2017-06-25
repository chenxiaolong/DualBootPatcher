/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbbootimg/format/loki_writer_p.h"

#include <algorithm>

#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstring>

#include <openssl/sha.h>

#include "mbcommon/endian.h"
#include "mbcommon/file.h"
#include "mbcommon/file_util.h"
#include "mbcommon/string.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/format/align_p.h"
#include "mbbootimg/format/loki_defs.h"
#include "mbbootimg/format/loki_p.h"
#include "mbbootimg/header.h"
#include "mbbootimg/writer.h"
#include "mbbootimg/writer_p.h"

#define MAX_ABOOT_SIZE              (2 * 1024 * 1024)


MB_BEGIN_C_DECLS

int loki_writer_get_header(MbBiWriter *biw, void *userdata,
                           MbBiHeader *header)
{
    (void) biw;
    (void) userdata;

    mb_bi_header_set_supported_fields(header, LOKI_NEW_SUPPORTED_FIELDS);

    return MB_BI_OK;
}

int loki_writer_write_header(MbBiWriter *biw, void *userdata,
                             MbBiHeader *header)
{
    LokiWriterCtx *const ctx = static_cast<LokiWriterCtx *>(userdata);
    int ret;

    // Construct header
    memset(&ctx->hdr, 0, sizeof(ctx->hdr));
    memcpy(ctx->hdr.magic, ANDROID_BOOT_MAGIC, ANDROID_BOOT_MAGIC_SIZE);

    if (mb_bi_header_kernel_address_is_set(header)) {
        ctx->hdr.kernel_addr = mb_bi_header_kernel_address(header);
    }
    if (mb_bi_header_ramdisk_address_is_set(header)) {
        ctx->hdr.ramdisk_addr = mb_bi_header_ramdisk_address(header);
    }
    if (mb_bi_header_secondboot_address_is_set(header)) {
        ctx->hdr.second_addr = mb_bi_header_secondboot_address(header);
    }
    if (mb_bi_header_kernel_tags_address_is_set(header)) {
        ctx->hdr.tags_addr = mb_bi_header_kernel_tags_address(header);
    }
    if (mb_bi_header_page_size_is_set(header)) {
        uint32_t page_size = mb_bi_header_page_size(header);

        switch (mb_bi_header_page_size(header)) {
        case 2048:
        case 4096:
        case 8192:
        case 16384:
        case 32768:
        case 65536:
        case 131072:
            ctx->hdr.page_size = page_size;
            break;
        default:
            mb_bi_writer_set_error(biw, MB_BI_ERROR_FILE_FORMAT,
                                   "Invalid page size: %" PRIu32, page_size);
            return MB_BI_FAILED;
        }
    } else {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_FILE_FORMAT,
                               "Page size field is required");
        return MB_BI_FAILED;
    }

    const char *board_name = mb_bi_header_board_name(header);
    const char *cmdline = mb_bi_header_kernel_cmdline(header);

    if (board_name) {
        if (strlen(board_name) >= sizeof(ctx->hdr.name)) {
            mb_bi_writer_set_error(biw, MB_BI_ERROR_FILE_FORMAT,
                                   "Board name too long");
            return MB_BI_FAILED;
        }

        strncpy(reinterpret_cast<char *>(ctx->hdr.name), board_name,
                sizeof(ctx->hdr.name) - 1);
        ctx->hdr.name[sizeof(ctx->hdr.name) - 1] = '\0';
    }
    if (cmdline) {
        if (strlen(cmdline) >= sizeof(ctx->hdr.cmdline)) {
            mb_bi_writer_set_error(biw, MB_BI_ERROR_FILE_FORMAT,
                                   "Kernel cmdline too long");
            return MB_BI_FAILED;
        }

        strncpy(reinterpret_cast<char *>(ctx->hdr.cmdline), cmdline,
                sizeof(ctx->hdr.cmdline) - 1);
        ctx->hdr.cmdline[sizeof(ctx->hdr.cmdline) - 1] = '\0';
    }

    // TODO: UNUSED
    // TODO: ID

    // Clear existing entries (none should exist unless this function fails and
    // the user reattempts to call it)
    _segment_writer_entries_clear(&ctx->segctx);

    ret = _segment_writer_entries_add(&ctx->segctx, MB_BI_ENTRY_KERNEL,
                                      0, false, ctx->hdr.page_size, biw);
    if (ret != MB_BI_OK) return ret;

    ret = _segment_writer_entries_add(&ctx->segctx, MB_BI_ENTRY_RAMDISK,
                                      0, false, ctx->hdr.page_size, biw);
    if (ret != MB_BI_OK) return ret;

    ret = _segment_writer_entries_add(&ctx->segctx, MB_BI_ENTRY_DEVICE_TREE,
                                      0, false, ctx->hdr.page_size, biw);
    if (ret != MB_BI_OK) return ret;

    ret = _segment_writer_entries_add(&ctx->segctx, MB_BI_ENTRY_ABOOT,
                                      0, true, 0, biw);
    if (ret != MB_BI_OK) return ret;

    // Start writing after first page
    ret = mb_file_seek(biw->file, ctx->hdr.page_size, SEEK_SET, nullptr);
    if (ret != MB_FILE_OK) {
        mb_bi_writer_set_error(biw, mb_file_error(biw->file),
                               "Failed to seek to first page: %s",
                               mb_file_error_string(biw->file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    return MB_BI_OK;
}

int loki_writer_get_entry(MbBiWriter *biw, void *userdata,
                          MbBiEntry *entry)
{
    LokiWriterCtx *const ctx = static_cast<LokiWriterCtx *>(userdata);

    return _segment_writer_get_entry(&ctx->segctx, biw->file, entry, biw);
}

int loki_writer_write_entry(MbBiWriter *biw, void *userdata,
                            MbBiEntry *entry)
{
    LokiWriterCtx *const ctx = static_cast<LokiWriterCtx *>(userdata);

    return _segment_writer_write_entry(&ctx->segctx, biw->file, entry, biw);
}

int loki_writer_write_data(MbBiWriter *biw, void *userdata,
                           const void *buf, size_t buf_size,
                           size_t *bytes_written)
{
    LokiWriterCtx *const ctx = static_cast<LokiWriterCtx *>(userdata);
    SegmentWriterEntry *swentry;
    int ret;

    swentry = _segment_writer_entry(&ctx->segctx);

    if (swentry->type == MB_BI_ENTRY_ABOOT) {
        if (buf_size > MAX_ABOOT_SIZE - ctx->aboot_size) {
            mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                                   "aboot image too large");
            return MB_BI_FATAL;
        }

        size_t new_aboot_size = ctx->aboot_size + buf_size;
        unsigned char *new_aboot;

        new_aboot = static_cast<unsigned char *>(
                realloc(ctx->aboot, new_aboot_size));
        if (!new_aboot) {
            mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                                   "Failed to expand aboot buffer");
            return MB_BI_FAILED;
        }

        memcpy(new_aboot + ctx->aboot_size, buf, buf_size);

        ctx->aboot = new_aboot;
        ctx->aboot_size = new_aboot_size;

        *bytes_written = buf_size;
    } else {
        ret = _segment_writer_write_data(&ctx->segctx, biw->file, buf, buf_size,
                                         bytes_written, biw);
        if (ret != MB_BI_OK) {
            return ret;
        }

        // We always include the image in the hash. The size is sometimes
        // included and is handled in loki_writer_finish_entry().
        if (!SHA1_Update(&ctx->sha_ctx, buf, buf_size)) {
            mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                                   "Failed to update SHA1 hash");
            // This must be fatal as the write already happened and cannot be
            // reattempted
            return MB_BI_FATAL;
        }
    }

    return MB_BI_OK;
}

int loki_writer_finish_entry(MbBiWriter *biw, void *userdata)
{
    LokiWriterCtx *const ctx = static_cast<LokiWriterCtx *>(userdata);
    SegmentWriterEntry *swentry;
    int ret;

    ret = _segment_writer_finish_entry(&ctx->segctx, biw->file, biw);
    if (ret != MB_BI_OK) {
        return ret;
    }

    swentry = _segment_writer_entry(&ctx->segctx);

    // Update SHA1 hash
    uint32_t le32_size = mb_htole32(swentry->size);

    // Include fake 0 size for unsupported secondboot image
    if (swentry->type == MB_BI_ENTRY_DEVICE_TREE
            && !SHA1_Update(&ctx->sha_ctx, "\x00\x00\x00\x00", 4)) {
        mb_bi_writer_set_error(biw, mb_file_error(biw->file),
                               "Failed to update SHA1 hash");
        return MB_BI_FATAL;
    }

    // Include size for everything except empty DT images
    if (swentry->type != MB_BI_ENTRY_ABOOT
            && (swentry->type != MB_BI_ENTRY_DEVICE_TREE || swentry->size > 0)
            && !SHA1_Update(&ctx->sha_ctx, &le32_size, sizeof(le32_size))) {
        mb_bi_writer_set_error(biw, mb_file_error(biw->file),
                               "Failed to update SHA1 hash");
        return MB_BI_FATAL;
    }

    switch (swentry->type) {
    case MB_BI_ENTRY_KERNEL:
        ctx->hdr.kernel_size = swentry->size;
        break;
    case MB_BI_ENTRY_RAMDISK:
        ctx->hdr.ramdisk_size = swentry->size;
        break;
    case MB_BI_ENTRY_DEVICE_TREE:
        ctx->hdr.dt_size = swentry->size;
        break;
    }

    return MB_BI_OK;
}

int loki_writer_close(MbBiWriter *biw, void *userdata)
{
    LokiWriterCtx *const ctx = static_cast<LokiWriterCtx *>(userdata);
    SegmentWriterEntry *swentry;
    int ret;
    size_t n;

    if (ctx->have_file_size) {
        ret = mb_file_seek(biw->file, ctx->file_size, SEEK_SET, nullptr);
        if (ret != MB_FILE_OK) {
            mb_bi_writer_set_error(biw, mb_file_error(biw->file),
                                   "Failed to seek to end of file: %s",
                                   mb_file_error_string(biw->file));
            return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        }
    } else {
        ret = mb_file_seek(biw->file, 0, SEEK_CUR, &ctx->file_size);
        if (ret != MB_FILE_OK) {
            mb_bi_writer_set_error(biw, mb_file_error(biw->file),
                                   "Failed to get file offset: %s",
                                   mb_file_error_string(biw->file));
            return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        }

        ctx->have_file_size = true;
    }

    swentry = _segment_writer_entry(&ctx->segctx);

    // If successful, finish up the boot image
    if (!swentry) {
        // Truncate to set size
        ret = mb_file_truncate(biw->file, ctx->file_size);
        if (ret < 0) {
            mb_bi_writer_set_error(biw, mb_file_error(biw->file),
                                   "Failed to truncate file: %s",
                                   mb_file_error_string(biw->file));
            return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        }

        // Set ID
        unsigned char digest[SHA_DIGEST_LENGTH];
        if (!SHA1_Final(digest, &ctx->sha_ctx)) {
            mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                                   "Failed to update SHA1 hash");
            return MB_BI_FATAL;
        }
        memcpy(ctx->hdr.id, digest, SHA_DIGEST_LENGTH);

        // Convert fields back to little-endian
        AndroidHeader hdr = ctx->hdr;
        android_fix_header_byte_order(&hdr);

        // Seek back to beginning to write header
        ret = mb_file_seek(biw->file, 0, SEEK_SET, nullptr);
        if (ret != MB_FILE_OK) {
            mb_bi_writer_set_error(biw, mb_file_error(biw->file),
                                   "Failed to seek to beginning: %s",
                                   mb_file_error_string(biw->file));
            return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        }

        // Write header
        ret = mb_file_write_fully(biw->file, &hdr, sizeof(hdr), &n);
        if (ret != MB_FILE_OK || n != sizeof(hdr)) {
            mb_bi_writer_set_error(biw, mb_file_error(biw->file),
                                   "Failed to write header: %s",
                                   mb_file_error_string(biw->file));
            return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        }

        // Patch with Loki
        ret = _loki_patch_file(biw, biw->file, ctx->aboot, ctx->aboot_size);
        if (ret != MB_BI_OK) {
            return ret;
        }
    }

    return MB_BI_OK;
}

int loki_writer_free(MbBiWriter *bir, void *userdata)
{
    (void) bir;
    LokiWriterCtx *const ctx = static_cast<LokiWriterCtx *>(userdata);
    _segment_writer_deinit(&ctx->segctx);
    free(ctx->aboot);
    free(ctx);
    return MB_BI_OK;
}

/*!
 * \brief Set Loki boot image output format
 *
 * \param biw MbBiWriter
 *
 * \return
 *   * #MB_BI_OK if the format is successfully enabled
 *   * #MB_BI_WARN if the format is already enabled
 *   * \<= #MB_BI_FAILED if an error occurs
 */
int mb_bi_writer_set_format_loki(MbBiWriter *biw)
{
    LokiWriterCtx *const ctx = static_cast<LokiWriterCtx *>(
            calloc(1, sizeof(LokiWriterCtx)));
    if (!ctx) {
        mb_bi_writer_set_error(biw, -errno,
                               "Failed to allocate LokiWriterCtx: %s",
                               strerror(errno));
        return MB_BI_FAILED;
    }

    if (!SHA1_Init(&ctx->sha_ctx)) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                               "Failed to initialize SHA_CTX");
        free(ctx);
        return false;
    }

    _segment_writer_init(&ctx->segctx);

    return _mb_bi_writer_register_format(biw,
                                         ctx,
                                         MB_BI_FORMAT_LOKI,
                                         MB_BI_FORMAT_NAME_LOKI,
                                         nullptr,
                                         &loki_writer_get_header,
                                         &loki_writer_write_header,
                                         &loki_writer_get_entry,
                                         &loki_writer_write_entry,
                                         &loki_writer_write_data,
                                         &loki_writer_finish_entry,
                                         &loki_writer_close,
                                         &loki_writer_free);
}

MB_END_C_DECLS
