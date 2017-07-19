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

#include "mbbootimg/format/android_writer_p.h"

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
#include "mbbootimg/format/bump_defs.h"
#include "mbbootimg/header.h"
#include "mbbootimg/writer.h"
#include "mbbootimg/writer_p.h"


MB_BEGIN_C_DECLS

int android_writer_get_header(MbBiWriter *biw, void *userdata,
                              MbBiHeader *header)
{
    (void) biw;
    (void) userdata;

    mb_bi_header_set_supported_fields(header, ANDROID_SUPPORTED_FIELDS);

    return MB_BI_OK;
}

int android_writer_write_header(MbBiWriter *biw, void *userdata,
                                MbBiHeader *header)
{
    AndroidWriterCtx *const ctx = static_cast<AndroidWriterCtx *>(userdata);
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

    ret = _segment_writer_entries_add(&ctx->segctx, MB_BI_ENTRY_SECONDBOOT,
                                      0, false, ctx->hdr.page_size, biw);
    if (ret != MB_BI_OK) return ret;

    ret = _segment_writer_entries_add(&ctx->segctx, MB_BI_ENTRY_DEVICE_TREE,
                                      0, false, ctx->hdr.page_size, biw);
    if (ret != MB_BI_OK) return ret;

    // Start writing after first page
    if (!biw->file->seek(ctx->hdr.page_size, SEEK_SET, nullptr)) {
        mb_bi_writer_set_error(biw, biw->file->error().value() /* TODO */,
                               "Failed to seek to first page: %s",
                               biw->file->error_string().c_str());
        return biw->file->is_fatal() ? MB_BI_FATAL : MB_BI_FAILED;
    }

    return MB_BI_OK;
}

int android_writer_get_entry(MbBiWriter *biw, void *userdata,
                             MbBiEntry *entry)
{
    AndroidWriterCtx *const ctx = static_cast<AndroidWriterCtx *>(userdata);

    return _segment_writer_get_entry(&ctx->segctx, biw->file, entry, biw);
}

int android_writer_write_entry(MbBiWriter *biw, void *userdata,
                               MbBiEntry *entry)
{
    AndroidWriterCtx *const ctx = static_cast<AndroidWriterCtx *>(userdata);

    return _segment_writer_write_entry(&ctx->segctx, biw->file, entry, biw);
}

int android_writer_write_data(MbBiWriter *biw, void *userdata,
                              const void *buf, size_t buf_size,
                              size_t &bytes_written)
{
    AndroidWriterCtx *const ctx = static_cast<AndroidWriterCtx *>(userdata);
    int ret;

    ret = _segment_writer_write_data(&ctx->segctx, biw->file, buf, buf_size,
                                     bytes_written, biw);
    if (ret != MB_BI_OK) {
        return ret;
    }

    // We always include the image in the hash. The size is sometimes included
    // and is handled in android_writer_finish_entry().
    if (!SHA1_Update(&ctx->sha_ctx, buf, buf_size)) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                               "Failed to update SHA1 hash");
        // This must be fatal as the write already happened and cannot be
        // reattempted
        return MB_BI_FATAL;
    }

    return MB_BI_OK;
}

int android_writer_finish_entry(MbBiWriter *biw, void *userdata)
{
    AndroidWriterCtx *const ctx = static_cast<AndroidWriterCtx *>(userdata);
    SegmentWriterEntry *swentry;
    int ret;

    ret = _segment_writer_finish_entry(&ctx->segctx, biw->file, biw);
    if (ret != MB_BI_OK) {
        return ret;
    }

    swentry = _segment_writer_entry(&ctx->segctx);

    // Update SHA1 hash
    uint32_t le32_size = mb_htole32(swentry->size);

    // Include size for everything except empty DT images
    if ((swentry->type != MB_BI_ENTRY_DEVICE_TREE || swentry->size > 0)
            && !SHA1_Update(&ctx->sha_ctx, &le32_size, sizeof(le32_size))) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
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
    case MB_BI_ENTRY_SECONDBOOT:
        ctx->hdr.second_size = swentry->size;
        break;
    case MB_BI_ENTRY_DEVICE_TREE:
        ctx->hdr.dt_size = swentry->size;
        break;
    }

    return MB_BI_OK;
}

int android_writer_close(MbBiWriter *biw, void *userdata)
{
    AndroidWriterCtx *const ctx = static_cast<AndroidWriterCtx *>(userdata);
    SegmentWriterEntry *swentry;
    size_t n;

    if (ctx->have_file_size) {
        if (!biw->file->seek(ctx->file_size, SEEK_SET, nullptr)) {
            mb_bi_writer_set_error(biw, biw->file->error().value() /* TODO */,
                                   "Failed to seek to end of file: %s",
                                   biw->file->error_string().c_str());
            return biw->file->is_fatal() ? MB_BI_FATAL : MB_BI_FAILED;
        }
    } else {
        if (!biw->file->seek(0, SEEK_CUR, &ctx->file_size)) {
            mb_bi_writer_set_error(biw, biw->file->error().value() /* TODO */,
                                   "Failed to get file offset: %s",
                                   biw->file->error_string().c_str());
            return biw->file->is_fatal() ? MB_BI_FATAL : MB_BI_FAILED;
        }

        ctx->have_file_size = true;
    }

    swentry = _segment_writer_entry(&ctx->segctx);

    // If successful, finish up the boot image
    if (!swentry) {
        // Write bump magic if we're outputting a bump'd image. Otherwise, write
        // the Samsung SEAndroid magic.
        if (ctx->is_bump) {
            if (!mb::file_write_fully(*biw->file, BUMP_MAGIC,
                                      BUMP_MAGIC_SIZE, n)
                    || n != BUMP_MAGIC_SIZE) {
                mb_bi_writer_set_error(biw, biw->file->error().value() /* TODO */,
                                       "Failed to write Bump magic: %s",
                                       biw->file->error_string().c_str());
                return biw->file->is_fatal() ? MB_BI_FATAL : MB_BI_FAILED;
            }
        } else {
            if (!mb::file_write_fully(*biw->file, SAMSUNG_SEANDROID_MAGIC,
                                      SAMSUNG_SEANDROID_MAGIC_SIZE, n)
                    || n != SAMSUNG_SEANDROID_MAGIC_SIZE) {
                mb_bi_writer_set_error(biw, biw->file->error().value() /* TODO */,
                                       "Failed to write SEAndroid magic: %s",
                                       biw->file->error_string().c_str());
                return biw->file->is_fatal() ? MB_BI_FATAL : MB_BI_FAILED;
            }
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
        if (!biw->file->seek(0, SEEK_SET, nullptr)) {
            mb_bi_writer_set_error(biw, biw->file->error().value() /* TODO */,
                                   "Failed to seek to beginning: %s",
                                   biw->file->error_string().c_str());
            return biw->file->is_fatal() ? MB_BI_FATAL : MB_BI_FAILED;
        }

        // Write header
        if (!mb::file_write_fully(*biw->file, &hdr, sizeof(hdr), n)
                || n != sizeof(hdr)) {
            mb_bi_writer_set_error(biw, biw->file->error().value() /* TODO */,
                                   "Failed to write header: %s",
                                   biw->file->error_string().c_str());
            return biw->file->is_fatal() ? MB_BI_FATAL : MB_BI_FAILED;
        }
    }

    return MB_BI_OK;
}

int android_writer_free(MbBiWriter *bir, void *userdata)
{
    (void) bir;
    AndroidWriterCtx *const ctx = static_cast<AndroidWriterCtx *>(userdata);
    _segment_writer_deinit(&ctx->segctx);
    free(ctx);
    return MB_BI_OK;
}

/*!
 * \brief Set Android boot image output format
 *
 * \param biw MbBiWriter
 *
 * \return
 *   * #MB_BI_OK if the format is successfully enabled
 *   * #MB_BI_WARN if the format is already enabled
 *   * \<= #MB_BI_FAILED if an error occurs
 */
int mb_bi_writer_set_format_android(MbBiWriter *biw)
{
    AndroidWriterCtx *const ctx = static_cast<AndroidWriterCtx *>(
            calloc(1, sizeof(AndroidWriterCtx)));
    if (!ctx) {
        mb_bi_writer_set_error(biw, -errno,
                               "Failed to allocate AndroidWriterCtx: %s",
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
                                         MB_BI_FORMAT_ANDROID,
                                         MB_BI_FORMAT_NAME_ANDROID,
                                         nullptr,
                                         &android_writer_get_header,
                                         &android_writer_write_header,
                                         &android_writer_get_entry,
                                         &android_writer_write_entry,
                                         &android_writer_write_data,
                                         &android_writer_finish_entry,
                                         &android_writer_close,
                                         &android_writer_free);
}

MB_END_C_DECLS
