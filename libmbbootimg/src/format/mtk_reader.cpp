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

#include "mbbootimg/format/mtk_reader_p.h"

#include <algorithm>
#include <type_traits>

#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "mbcommon/endian.h"
#include "mbcommon/file.h"
#include "mbcommon/file_util.h"
#include "mbcommon/string.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/format/align_p.h"
#include "mbbootimg/format/android_reader_p.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"


MB_BEGIN_C_DECLS

/*!
 * \brief Read MTK header
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use mb_file_seek() to return to a known position.
 *
 * \param[in] bir MbBiReader for setting error messages
 * \param[in] file MbFile handle
 * \param[in] offset Offset to read MTK header
 * \param[out] mtkhdr_out Pointer to store MTK header (in host byte order)
 *
 * \return
 *   * #MB_BI_OK if the header is found
 *   * #MB_BI_WARN if the header is not found
 *   * #MB_BI_FAILED if any file operation fails non-fatally
 *   * #MB_BI_FATAL if any file operation fails fatally
 */
int read_mtk_header(MbBiReader *bir, MbFile *file,
                    uint64_t offset, MtkHeader *mtkhdr_out)
{
    MtkHeader mtkhdr;
    size_t n;
    int ret;

    ret = mb_file_seek(file, offset, SEEK_SET, nullptr);
    if (ret < 0) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to seek to MTK header at %" PRIu64 ": %s",
                               offset, mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    ret = mb_file_read_fully(file, &mtkhdr, sizeof(mtkhdr), &n);
    if (ret < 0) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to read MTK header: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    if (n != sizeof(MtkHeader)
            || memcmp(mtkhdr.magic, MTK_MAGIC, MTK_MAGIC_SIZE) != 0) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "MTK header not found at %" PRIu64,
                               offset);
        return MB_BI_WARN;
    }

    *mtkhdr_out = mtkhdr;
    mtk_fix_header_byte_order(mtkhdr_out);

    return MB_BI_OK;
}

/*!
 * \brief Find location of the MTK kernel and ramdisk headers
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use mb_file_seek() to return to a known position.
 *
 * \param[in] bir MbBiReader for setting error messages
 * \param[in] file MbFile handle
 * \param[in] hdr Android boot image header (in host byte order)
 * \param[out] kernel_mtkhdr_out Pointer to store kernel MTK header
 *                               (in host byte order)
 * \param[out] kernel_offset_out Pointer to store offset of kernel image
 * \param[out] ramdisk_mtkhdr_out Pointer to store ramdisk MTK header
 *                                (in host byte order)
 * \param[out] ramdisk_offset_out Pointer to store offset of ramdisk image
 *
 * \return
 *   * #MB_BI_OK if both the kernel and ramdisk headers are found
 *   * #MB_BI_WARN if the kernel or ramdisk header is not found
 *   * #MB_BI_FAILED if any file operation fails non-fatally
 *   * #MB_BI_FATAL if any file operation fails fatally
 */
int find_mtk_headers(MbBiReader *bir, MbFile *file,
                     AndroidHeader *hdr,
                     MtkHeader *kernel_mtkhdr_out,
                     uint64_t *kernel_offset_out,
                     MtkHeader *ramdisk_mtkhdr_out,
                     uint64_t *ramdisk_offset_out)
{
    uint64_t kernel_offset;
    uint64_t ramdisk_offset;
    int ret;
    uint64_t pos = 0;

    // Header
    pos += hdr->page_size;

    // Kernel
    kernel_offset = pos;
    pos += hdr->kernel_size;
    pos += align_page_size<uint64_t>(pos, hdr->page_size);

    // Ramdisk
    ramdisk_offset = pos;
    pos += hdr->ramdisk_size;
    pos += align_page_size<uint64_t>(pos, hdr->page_size);

    ret = read_mtk_header(bir, file, kernel_offset, kernel_mtkhdr_out);
    if (ret == MB_BI_OK) {
        *kernel_offset_out = kernel_offset + sizeof(MtkHeader);
    } else {
        return ret;
    }

    ret = read_mtk_header(bir, file, ramdisk_offset, ramdisk_mtkhdr_out);
    if (ret == MB_BI_OK) {
        *ramdisk_offset_out = ramdisk_offset + sizeof(MtkHeader);
    } else {
        return ret;
    }

    return MB_BI_OK;
}

/*!
 * \brief Perform a bid
 *
 * \return
 *   * If \>= 0, the number of bits that conform to the MTK format
 *   * #MB_BI_WARN if this is a bid that can't be won
 *   * #MB_BI_FAILED if any file operations fail non-fatally
 *   * #MB_BI_FATAL if any file operations fail fatally
 */
int mtk_reader_bid(MbBiReader *bir, void *userdata, int best_bid)
{
    MtkReaderCtx *const ctx = static_cast<MtkReaderCtx *>(userdata);
    int bid = 0;
    int ret;

    if (best_bid >= (ANDROID_BOOT_MAGIC_SIZE + 2 * MTK_MAGIC_SIZE) * 8) {
        // This is a bid we can't win, so bail out
        return MB_BI_WARN;
    }

    // Find the Android header
    ret = find_android_header(bir, bir->file, ANDROID_MAX_HEADER_OFFSET,
                              &ctx->hdr, &ctx->header_offset);
    if (ret == MB_BI_OK) {
        // Update bid to account for matched bits
        ctx->have_header_offset = true;
        bid += ANDROID_BOOT_MAGIC_SIZE * 8;
    } else if (ret == MB_BI_WARN) {
        // Header not found. This can't be an Android boot image.
        return 0;
    } else {
        return ret;
    }

    ret = find_mtk_headers(bir, bir->file, &ctx->hdr,
                           &ctx->mtk_kernel_hdr, &ctx->mtk_kernel_offset,
                           &ctx->mtk_ramdisk_hdr, &ctx->mtk_ramdisk_offset);
    if (ret == MB_BI_OK) {
        // Update bid to account for matched bids
        ctx->have_mtkhdr_offsets = true;
        bid += 2 * MTK_MAGIC_SIZE * 8;
    } else {
        return ret;
    }

    return bid;
}

int mtk_reader_read_header(MbBiReader *bir, void *userdata,
                           MbBiHeader *header)
{
    MtkReaderCtx *const ctx = static_cast<MtkReaderCtx *>(userdata);
    int ret;

    if (!ctx->have_header_offset) {
        // A bid might not have been performed if the user forced a particular
        // format
        ret = find_android_header(bir, bir->file, ANDROID_MAX_HEADER_OFFSET,
                                  &ctx->hdr, &ctx->header_offset);
        if (ret < 0) {
            return ret;
        }
        ctx->have_header_offset = true;
    }
    if (!ctx->have_mtkhdr_offsets) {
        ret = find_mtk_headers(bir, bir->file, &ctx->hdr,
                               &ctx->mtk_kernel_hdr, &ctx->mtk_kernel_offset,
                               &ctx->mtk_ramdisk_hdr, &ctx->mtk_ramdisk_offset);
        if (ret < 0) {
            return ret;
        }
        ctx->have_mtkhdr_offsets = true;
    }

    // Validate that the kernel and ramdisk sizes are consistent
    if (ctx->hdr.kernel_size != static_cast<uint64_t>(
            ctx->mtk_kernel_hdr.size) + sizeof(MtkHeader)) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "Mismatched kernel size in Android and "
                               "MTK headers");
        return MB_BI_FAILED;
    }
    if (ctx->hdr.ramdisk_size != static_cast<uint64_t>(
            ctx->mtk_ramdisk_hdr.size) + sizeof(MtkHeader)) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "Mismatched ramdisk size in Android and "
                               "MTK headers");
        return MB_BI_FAILED;
    }

    ret = android_set_header(&ctx->hdr, header);
    if (ret != MB_BI_OK) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_INTERNAL_ERROR,
                               "Failed to set header fields");
        return ret;
    }

    // Calculate offsets for each section

    uint64_t pos = 0;
    uint32_t page_size = mb_bi_header_page_size(header);
    uint64_t kernel_offset;
    uint64_t ramdisk_offset;
    uint64_t second_offset;
    uint64_t dt_offset;

    // pos cannot overflow due to the nature of the operands (adding UINT32_MAX
    // a few times can't overflow a uint64_t). File length overflow is checked
    // during read.

    // Header
    pos += ctx->header_offset;
    pos += sizeof(AndroidHeader);
    pos += align_page_size<uint64_t>(pos, page_size);

    // Kernel
    kernel_offset = pos;
    pos += ctx->hdr.kernel_size;
    pos += align_page_size<uint64_t>(pos, page_size);

    // Ramdisk
    ramdisk_offset = pos;
    pos += ctx->hdr.ramdisk_size;
    pos += align_page_size<uint64_t>(pos, page_size);

    // Second bootloader
    second_offset = pos;
    pos += ctx->hdr.second_size;
    pos += align_page_size<uint64_t>(pos, page_size);

    // Device tree
    dt_offset = pos;
    pos += ctx->hdr.dt_size;
    pos += align_page_size<uint64_t>(pos, page_size);

    _segment_reader_entries_clear(&ctx->segctx);

    ret = _segment_reader_entries_add(&ctx->segctx,
                                      MB_BI_ENTRY_MTK_KERNEL_HEADER,
                                      kernel_offset, sizeof(MtkHeader), false,
                                      bir);
    if (ret != MB_BI_OK) return ret;

    ret = _segment_reader_entries_add(&ctx->segctx, MB_BI_ENTRY_KERNEL,
                                      ctx->mtk_kernel_offset,
                                      ctx->mtk_kernel_hdr.size, false, bir);
    if (ret != MB_BI_OK) return ret;

    ret = _segment_reader_entries_add(&ctx->segctx,
                                      MB_BI_ENTRY_MTK_RAMDISK_HEADER,
                                      ramdisk_offset, sizeof(MtkHeader), false,
                                      bir);
    if (ret != MB_BI_OK) return ret;

    ret = _segment_reader_entries_add(&ctx->segctx, MB_BI_ENTRY_RAMDISK,
                                      ctx->mtk_ramdisk_offset,
                                      ctx->mtk_ramdisk_hdr.size, false, bir);
    if (ret != MB_BI_OK) return ret;

    if (ctx->hdr.second_size > 0) {
        ret = _segment_reader_entries_add(&ctx->segctx, MB_BI_ENTRY_SECONDBOOT,
                                          second_offset, ctx->hdr.second_size,
                                          false, bir);
        if (ret != MB_BI_OK) return ret;
    }

    if (ctx->hdr.dt_size > 0) {
        ret = _segment_reader_entries_add(&ctx->segctx, MB_BI_ENTRY_DEVICE_TREE,
                                          dt_offset, ctx->hdr.dt_size, false,
                                          bir);
        if (ret != MB_BI_OK) return ret;
    }

    return MB_BI_OK;
}

int mtk_reader_read_entry(MbBiReader *bir, void *userdata,
                          MbBiEntry *entry)
{
    MtkReaderCtx *const ctx = static_cast<MtkReaderCtx *>(userdata);

    return _segment_reader_read_entry(&ctx->segctx, bir->file, entry, bir);
}

int mtk_reader_go_to_entry(MbBiReader *bir, void *userdata, MbBiEntry *entry,
                           int entry_type)
{
    MtkReaderCtx *const ctx = static_cast<MtkReaderCtx *>(userdata);

    return _segment_reader_go_to_entry(&ctx->segctx, bir->file, entry,
                                       entry_type, bir);
}

int mtk_reader_read_data(MbBiReader *bir, void *userdata,
                         void *buf, size_t buf_size,
                         size_t *bytes_read)
{
    MtkReaderCtx *const ctx = static_cast<MtkReaderCtx *>(userdata);

    return _segment_reader_read_data(&ctx->segctx, bir->file, buf, buf_size,
                                     bytes_read, bir);
}

int mtk_reader_free(MbBiReader *bir, void *userdata)
{
    (void) bir;
    MtkReaderCtx *const ctx = static_cast<MtkReaderCtx *>(userdata);
    _segment_reader_deinit(&ctx->segctx);
    free(ctx);
    return MB_BI_OK;
}

/*!
 * \brief Enable support for MTK boot image format
 *
 * \param bir MbBiReader
 *
 * \return
 *   * #MB_BI_OK if the format is successfully enabled
 *   * #MB_BI_WARN if the format is already enabled
 *   * \<= #MB_BI_FAILED if an error occurs
 */
int mb_bi_reader_enable_format_mtk(MbBiReader *bir)
{
    MtkReaderCtx *const ctx = static_cast<MtkReaderCtx *>(
            calloc(1, sizeof(MtkReaderCtx)));
    if (!ctx) {
        mb_bi_reader_set_error(bir, -errno,
                               "Failed to allocate MtkReaderCtx: %s",
                               strerror(errno));
        return MB_BI_FAILED;
    }

    _segment_reader_init(&ctx->segctx);

    return _mb_bi_reader_register_format(bir,
                                         ctx,
                                         MB_BI_FORMAT_MTK,
                                         MB_BI_FORMAT_NAME_MTK,
                                         &mtk_reader_bid,
                                         nullptr,
                                         &mtk_reader_read_header,
                                         &mtk_reader_read_entry,
                                         &mtk_reader_go_to_entry,
                                         &mtk_reader_read_data,
                                         &mtk_reader_free);
}

MB_END_C_DECLS
