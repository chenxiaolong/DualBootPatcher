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

#include "mbbootimg/format/mtk_writer_p.h"

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
#include "mbbootimg/header.h"
#include "mbbootimg/writer.h"
#include "mbbootimg/writer_p.h"


MB_BEGIN_C_DECLS

static int _mtk_header_update_size(MbBiWriter *biw, MbFile *file,
                                   uint64_t offset, uint32_t size)
{
    uint32_t le32_size = mb_htole32(size);
    size_t n;
    int ret;

    if (offset > SIZE_MAX - offsetof(MtkHeader, size)) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                               "MTK header offset too large");
        return MB_BI_FATAL;
    }

    ret = mb_file_seek(biw->file, offset + offsetof(MtkHeader, size),
                       SEEK_SET, nullptr);
    if (ret != MB_FILE_OK) {
        mb_bi_writer_set_error(biw, mb_file_error(biw->file),
                               "Failed to seek to MTK size field: %s",
                               mb_file_error_string(biw->file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    ret = mb_file_write_fully(file, &le32_size, sizeof(le32_size), &n);
    if (ret != MB_FILE_OK) {
        mb_bi_writer_set_error(biw, mb_file_error(biw->file),
                               "Failed to write MTK size field: %s",
                               mb_file_error_string(biw->file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    } else if (n != sizeof(le32_size)) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_FILE_FORMAT,
                               "Unexpected EOF when writing MTK size field");
        return MB_BI_FAILED;
    }

    return MB_BI_OK;
}

static int _mtk_compute_sha1(MbBiWriter *biw, SegmentWriterCtx *segctx,
                             MbFile *file,
                             unsigned char digest[SHA_DIGEST_LENGTH])
{
    SHA_CTX sha_ctx;
    char buf[10240];
    size_t n;
    int ret;

    uint32_t kernel_mtkhdr_size = 0;
    uint32_t ramdisk_mtkhdr_size = 0;

    if (!SHA1_Init(&sha_ctx)) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                               "Failed to initialize SHA_CTX");
        return MB_BI_FAILED;
    }

    for (size_t i = 0; i < _segment_writer_entries_size(segctx); ++i) {
        SegmentWriterEntry *entry = _segment_writer_entries_get(segctx, i);
        uint64_t remain = entry->size;

        ret = mb_file_seek(file, entry->offset, SEEK_SET, nullptr);
        if (ret != MB_FILE_OK) {
            mb_bi_writer_set_error(biw, mb_file_error(file),
                                   "Failed to seek to entry %" MB_PRIzu ": %s",
                                   i, mb_file_error_string(file));
            return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        }

        // Update checksum with data
        while (remain > 0) {
            size_t to_read = std::min<uint64_t>(remain, sizeof(buf));

            ret = mb_file_read_fully(file, buf, to_read, &n);
            if (ret != MB_FILE_OK) {
                mb_bi_writer_set_error(biw, mb_file_error(file),
                                       "Failed to read entry %" MB_PRIzu ": %s",
                                       i, mb_file_error_string(file));
                return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
            } else if (n != to_read) {
                mb_bi_writer_set_error(biw, mb_file_error(file),
                                       "Unexpected EOF when reading entry");
                return MB_BI_FAILED;
            }

            if (!SHA1_Update(&sha_ctx, buf, n)) {
                mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                                       "Failed to update SHA1 hash");
                return MB_BI_FAILED;
            }

            remain -= to_read;
        }

        uint32_t le32_size;

        // Update checksum with size
        switch (entry->type) {
        case MB_BI_ENTRY_MTK_KERNEL_HEADER:
            kernel_mtkhdr_size = entry->size;
            continue;
        case MB_BI_ENTRY_MTK_RAMDISK_HEADER:
            ramdisk_mtkhdr_size = entry->size;
            continue;
        case MB_BI_ENTRY_KERNEL:
            le32_size = mb_htole32(entry->size + kernel_mtkhdr_size);
            break;
        case MB_BI_ENTRY_RAMDISK:
            le32_size = mb_htole32(entry->size + ramdisk_mtkhdr_size);
            break;
        case MB_BI_ENTRY_SECONDBOOT:
            le32_size = mb_htole32(entry->size);
            break;
        case MB_BI_ENTRY_DEVICE_TREE:
            if (entry->size == 0) {
                continue;
            }
            le32_size = mb_htole32(entry->size);
            break;
        default:
            continue;
        }

        if (!SHA1_Update(&sha_ctx, &le32_size, sizeof(le32_size))) {
            mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                                   "Failed to update SHA1 hash");
            return MB_BI_FAILED;
        }
    }

    if (!SHA1_Final(digest, &sha_ctx)) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_INTERNAL_ERROR,
                               "Failed to finalize SHA1 hash");
        return MB_BI_FATAL;
    }

    return MB_BI_OK;
}

int mtk_writer_get_header(MbBiWriter *biw, void *userdata,
                          MbBiHeader *header)
{
    (void) biw;
    (void) userdata;

    mb_bi_header_set_supported_fields(header, MTK_SUPPORTED_FIELDS);

    return MB_BI_OK;
}

int mtk_writer_write_header(MbBiWriter *biw, void *userdata,
                            MbBiHeader *header)
{
    MtkWriterCtx *const ctx = static_cast<MtkWriterCtx *>(userdata);
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

    ret = _segment_writer_entries_add(&ctx->segctx,
                                      MB_BI_ENTRY_MTK_KERNEL_HEADER,
                                      0, false, 0, biw);
    if (ret != MB_BI_OK) return ret;

    ret = _segment_writer_entries_add(&ctx->segctx, MB_BI_ENTRY_KERNEL,
                                      0, false, ctx->hdr.page_size, biw);
    if (ret != MB_BI_OK) return ret;

    ret = _segment_writer_entries_add(&ctx->segctx,
                                      MB_BI_ENTRY_MTK_RAMDISK_HEADER,
                                      0, false, 0, biw);
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
    ret = mb_file_seek(biw->file, ctx->hdr.page_size, SEEK_SET, nullptr);
    if (ret != MB_FILE_OK) {
        mb_bi_writer_set_error(biw, mb_file_error(biw->file),
                               "Failed to seek to first page: %s",
                               mb_file_error_string(biw->file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    return MB_BI_OK;
}

int mtk_writer_get_entry(MbBiWriter *biw, void *userdata,
                         MbBiEntry *entry)
{
    MtkWriterCtx *const ctx = static_cast<MtkWriterCtx *>(userdata);

    return _segment_writer_get_entry(&ctx->segctx, biw->file, entry, biw);
}

int mtk_writer_write_entry(MbBiWriter *biw, void *userdata,
                           MbBiEntry *entry)
{
    MtkWriterCtx *const ctx = static_cast<MtkWriterCtx *>(userdata);

    return _segment_writer_write_entry(&ctx->segctx, biw->file, entry, biw);
}

int mtk_writer_write_data(MbBiWriter *biw, void *userdata,
                          const void *buf, size_t buf_size,
                          size_t *bytes_written)
{
    MtkWriterCtx *const ctx = static_cast<MtkWriterCtx *>(userdata);

    return _segment_writer_write_data(&ctx->segctx, biw->file, buf, buf_size,
                                      bytes_written, biw);
}

int mtk_writer_finish_entry(MbBiWriter *biw, void *userdata)
{
    MtkWriterCtx *const ctx = static_cast<MtkWriterCtx *>(userdata);
    SegmentWriterEntry *swentry;
    int ret;

    ret = _segment_writer_finish_entry(&ctx->segctx, biw->file, biw);
    if (ret != MB_BI_OK) {
        return ret;
    }

    swentry = _segment_writer_entry(&ctx->segctx);

    if ((swentry->type == MB_BI_ENTRY_KERNEL
            || swentry->type == MB_BI_ENTRY_RAMDISK)
            && swentry->size == UINT32_MAX - sizeof(MtkHeader)) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_FILE_FORMAT,
                               "Entry size too large to accomodate MTK header");
        return MB_BI_FATAL;
    } else if ((swentry->type == MB_BI_ENTRY_MTK_KERNEL_HEADER
            || swentry->type == MB_BI_ENTRY_MTK_RAMDISK_HEADER)
            && swentry->size != sizeof(MtkHeader)) {
        mb_bi_writer_set_error(biw, MB_BI_ERROR_FILE_FORMAT,
                               "Invalid size for MTK header entry");
        return MB_BI_FATAL;
    }

    switch (swentry->type) {
    case MB_BI_ENTRY_KERNEL:
        ctx->hdr.kernel_size = swentry->size + sizeof(MtkHeader);
        break;
    case MB_BI_ENTRY_RAMDISK:
        ctx->hdr.ramdisk_size = swentry->size + sizeof(MtkHeader);
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

int mtk_writer_close(MbBiWriter *biw, void *userdata)
{
    MtkWriterCtx *const ctx = static_cast<MtkWriterCtx *>(userdata);
    SegmentWriterEntry *swentry;
    int ret;
    size_t n;

    if (!ctx->have_file_size) {
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

        // Update MTK header sizes
        for (size_t i = 0; i < _segment_writer_entries_size(&ctx->segctx); ++i) {
            SegmentWriterEntry *entry =
                    _segment_writer_entries_get(&ctx->segctx, i);
            switch (entry->type) {
            case MB_BI_ENTRY_MTK_KERNEL_HEADER:
                ret = _mtk_header_update_size(biw, biw->file, entry->offset,
                                              ctx->hdr.kernel_size
                                              - sizeof(MtkHeader));
                break;
            case MB_BI_ENTRY_MTK_RAMDISK_HEADER:
                ret = _mtk_header_update_size(biw, biw->file, entry->offset,
                                              ctx->hdr.ramdisk_size
                                              - sizeof(MtkHeader));
                break;
            default:
                continue;
            }

            if (ret != MB_BI_OK) {
                return ret;
            }
        }

        // We need to take the performance hit and compute the SHA1 here.
        // We can't fill in the sizes in the MTK headers when we're writing
        // them. Thus, if we calculated the SHA1sum during write, it would be
        // incorrect.
        ret = _mtk_compute_sha1(biw, &ctx->segctx, biw->file,
                                reinterpret_cast<unsigned char *>(ctx->hdr.id));
        if (ret != MB_BI_OK) {
            return ret;
        }

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
    }

    return MB_BI_OK;
}

int mtk_writer_free(MbBiWriter *bir, void *userdata)
{
    (void) bir;
    MtkWriterCtx *const ctx = static_cast<MtkWriterCtx *>(userdata);
    _segment_writer_deinit(&ctx->segctx);
    free(ctx);
    return MB_BI_OK;
}

/*!
 * \brief Set MTK boot image output format
 *
 * \param biw MbBiWriter
 *
 * \return
 *   * #MB_BI_OK if the format is successfully enabled
 *   * #MB_BI_WARN if the format is already enabled
 *   * \<= #MB_BI_FAILED if an error occurs
 */
int mb_bi_writer_set_format_mtk(MbBiWriter *biw)
{
    MtkWriterCtx *const ctx = static_cast<MtkWriterCtx *>(
            calloc(1, sizeof(MtkWriterCtx)));
    if (!ctx) {
        mb_bi_writer_set_error(biw, -errno,
                               "Failed to allocate MtkWriterCtx: %s",
                               strerror(errno));
        return MB_BI_FAILED;
    }

    _segment_writer_init(&ctx->segctx);

    return _mb_bi_writer_register_format(biw,
                                         ctx,
                                         MB_BI_FORMAT_MTK,
                                         MB_BI_FORMAT_NAME_MTK,
                                         nullptr,
                                         &mtk_writer_get_header,
                                         &mtk_writer_write_header,
                                         &mtk_writer_get_entry,
                                         &mtk_writer_write_entry,
                                         &mtk_writer_write_data,
                                         &mtk_writer_finish_entry,
                                         &mtk_writer_close,
                                         &mtk_writer_free);
}

MB_END_C_DECLS
