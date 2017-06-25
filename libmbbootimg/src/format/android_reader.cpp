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

#include "mbbootimg/format/android_reader_p.h"

#include <algorithm>
#include <type_traits>

#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "mbcommon/endian.h"
#include "mbcommon/file.h"
#include "mbcommon/file_util.h"
#include "mbcommon/libc/string.h"
#include "mbcommon/string.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/format/align_p.h"
#include "mbbootimg/format/bump_defs.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"


MB_BEGIN_C_DECLS

/*!
 * \brief Find and read Android boot image header
 *
 * \note The integral fields in the header will be converted to the host's byte
 *       order.
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use mb_file_seek() to return to a known position.
 *
 * \param[in] bir MbBiReader for setting error messages
 * \param[in] file MbFile handle
 * \param[in] max_header_offset Maximum offset that a header can start (must be
 *                              less than #ANDROID_MAX_HEADER_OFFSET)
 * \param[out] header_out Pointer to store header
 * \param[out] offset_out Pointer to store header offset
 *
 * \return
 *   * #MB_BI_OK if the header is found
 *   * #MB_BI_WARN if the header is not found
 *   * #MB_BI_FAILED if any file operation fails non-fatally
 *   * #MB_BI_FATAL if any file operation fails fatally
 */
int find_android_header(MbBiReader *bir, MbFile *file,
                        uint64_t max_header_offset,
                        AndroidHeader *header_out, uint64_t *offset_out)
{
    unsigned char buf[ANDROID_MAX_HEADER_OFFSET + sizeof(AndroidHeader)];
    size_t n;
    int ret;
    void *ptr;
    size_t offset;

    if (max_header_offset > ANDROID_MAX_HEADER_OFFSET) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_INVALID_ARGUMENT,
                               "Max header offset (%" PRIu64
                               ") must be less than %d",
                               max_header_offset, ANDROID_MAX_HEADER_OFFSET);
        return MB_BI_WARN;
    }

    ret = mb_file_seek(file, 0, SEEK_SET, nullptr);
    if (ret != MB_FILE_OK) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to seek to beginning: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    ret = mb_file_read_fully(
            file, buf, max_header_offset + sizeof(AndroidHeader), &n);
    if (ret != MB_FILE_OK) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to read header: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    ptr = mb_memmem(buf, n, ANDROID_BOOT_MAGIC, ANDROID_BOOT_MAGIC_SIZE);
    if (!ptr) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "Android magic not found in first %d bytes",
                               ANDROID_MAX_HEADER_OFFSET);
        return MB_BI_WARN;
    }

    offset = static_cast<unsigned char *>(ptr) - buf;

    if (n - offset < sizeof(AndroidHeader)) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "Android header at %" MB_PRIzu
                               " exceeds file size", offset);
        return MB_BI_WARN;
    }

    // Copy header
    memcpy(header_out, ptr, sizeof(AndroidHeader));
    android_fix_header_byte_order(header_out);
    *offset_out = offset;

    return MB_BI_OK;
}

/*!
 * \brief Find location of Samsung SEAndroid magic
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use mb_file_seek() to return to a known position.
 *
 * \param[in] bir MbBiReader for setting error messages
 * \param[in] file MbFile handle
 * \param[in] hdr Android boot image header (in host byte order)
 * \param[out] offset_out Pointer to store magic offset
 *
 * \return
 *   * #MB_BI_OK if the magic is found
 *   * #MB_BI_WARN if the magic is not found
 *   * #MB_BI_FAILED if any file operation fails non-fatally
 *   * #MB_BI_FATAL if any file operation fails fatally
 */
int find_samsung_seandroid_magic(MbBiReader *bir, MbFile *file,
                                 AndroidHeader *hdr, uint64_t *offset_out)
{
    unsigned char buf[SAMSUNG_SEANDROID_MAGIC_SIZE];
    size_t n;
    int ret;
    uint64_t pos = 0;

    // Skip header, whose size cannot exceed the page size
    pos += hdr->page_size;

    // Skip kernel
    pos += hdr->kernel_size;
    pos += align_page_size<uint64_t>(pos, hdr->page_size);

    // Skip ramdisk
    pos += hdr->ramdisk_size;
    pos += align_page_size<uint64_t>(pos, hdr->page_size);

    // Skip second bootloader
    pos += hdr->second_size;
    pos += align_page_size<uint64_t>(pos, hdr->page_size);

    // Skip device tree
    pos += hdr->dt_size;
    pos += align_page_size<uint64_t>(pos, hdr->page_size);

    ret = mb_file_seek(file, pos, SEEK_SET, nullptr);
    if (ret < 0) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "SEAndroid magic not found: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    ret = mb_file_read_fully(file, buf, sizeof(buf), &n);
    if (ret < 0) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to read SEAndroid magic: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    if (n != SAMSUNG_SEANDROID_MAGIC_SIZE
            || memcmp(buf, SAMSUNG_SEANDROID_MAGIC, n) != 0) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "SEAndroid magic not found in last %d bytes",
                               SAMSUNG_SEANDROID_MAGIC_SIZE);
        return MB_BI_WARN;
    }

    *offset_out = pos;
    return MB_BI_OK;
}

/*!
 * \brief Find location of Bump magic
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use mb_file_seek() to return to a known position.
 *
 * \param[in] bir MbBiReader for setting error messages
 * \param[in] file MbFile handle
 * \param[in] hdr Android boot image header (in host byte order)
 * \param[out] offset_out Pointer to store magic offset
 *
 * \return
 *   * #MB_BI_OK if the magic is found
 *   * #MB_BI_WARN if the magic is not found
 *   * #MB_BI_FAILED if any file operation fails non-fatally
 *   * #MB_BI_FATAL if any file operation fails fatally
 */
int find_bump_magic(MbBiReader *bir, MbFile *file,
                    AndroidHeader *hdr, uint64_t *offset_out)
{
    unsigned char buf[BUMP_MAGIC_SIZE];
    size_t n;
    int ret;
    uint64_t pos = 0;

    // Skip header, whose size cannot exceed the page size
    pos += hdr->page_size;

    // Skip kernel
    pos += hdr->kernel_size;
    pos += align_page_size<uint64_t>(pos, hdr->page_size);

    // Skip ramdisk
    pos += hdr->ramdisk_size;
    pos += align_page_size<uint64_t>(pos, hdr->page_size);

    // Skip second bootloader
    pos += hdr->second_size;
    pos += align_page_size<uint64_t>(pos, hdr->page_size);

    // Skip device tree
    pos += hdr->dt_size;
    pos += align_page_size<uint64_t>(pos, hdr->page_size);

    ret = mb_file_seek(file, pos, SEEK_SET, nullptr);
    if (ret < 0) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "SEAndroid magic not found: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    ret = mb_file_read_fully(file, buf, sizeof(buf), &n);
    if (ret < 0) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to read SEAndroid magic: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    if (n != BUMP_MAGIC_SIZE || memcmp(buf, BUMP_MAGIC, n) != 0) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "Bump magic not found in last %d bytes",
                               BUMP_MAGIC_SIZE);
        return MB_BI_WARN;
    }

    *offset_out = pos;
    return MB_BI_OK;
}

int android_set_header(AndroidHeader *hdr, MbBiHeader *header)
{
    int ret;

    char board_name[sizeof(hdr->name) + 1];
    char cmdline[sizeof(hdr->cmdline) + 1];

    strncpy(board_name, reinterpret_cast<char *>(hdr->name),
            sizeof(hdr->name));
    strncpy(cmdline, reinterpret_cast<char *>(hdr->cmdline),
            sizeof(hdr->cmdline));
    board_name[sizeof(hdr->name)] = '\0';
    cmdline[sizeof(hdr->cmdline)] = '\0';

    mb_bi_header_set_supported_fields(header, ANDROID_SUPPORTED_FIELDS);

    ret = mb_bi_header_set_board_name(header, board_name);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_kernel_cmdline(header, cmdline);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_page_size(header, hdr->page_size);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_kernel_address(header, hdr->kernel_addr);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_ramdisk_address(header, hdr->ramdisk_addr);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_secondboot_address(header, hdr->second_addr);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_kernel_tags_address(header, hdr->tags_addr);
    if (ret != MB_BI_OK) return ret;

    // TODO: unused
    // TODO: id

    return MB_BI_OK;
}

/*!
 * \brief Perform a bid
 *
 * \return
 *   * If \>= 0, the number of bits that conform to the Android format
 *   * #MB_BI_WARN if this is a bid that can't be won
 *   * #MB_BI_FAILED if any file operations fail non-fatally
 *   * #MB_BI_FATAL if any file operations fail fatally
 */
int android_reader_bid(MbBiReader *bir, void *userdata, int best_bid)
{
    AndroidReaderCtx *const ctx = static_cast<AndroidReaderCtx *>(userdata);
    int bid = 0;
    int ret;

    if (best_bid >= (ANDROID_BOOT_MAGIC_SIZE
            + SAMSUNG_SEANDROID_MAGIC_SIZE) * 8) {
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

    // Find the Samsung magic
    ret = find_samsung_seandroid_magic(bir, bir->file, &ctx->hdr,
                                       &ctx->samsung_offset);
    if (ret == MB_BI_OK) {
        // Update bid to account for matched bits
        ctx->have_samsung_offset = true;
        bid += SAMSUNG_SEANDROID_MAGIC_SIZE * 8;
    } else if (ret == MB_BI_WARN) {
        // Nothing found. Don't change bid
    } else {
        return ret;
    }

    return bid;
}

/*!
 * \brief Perform a bid
 *
 * \return
 *   * If \>= 0, the number of bits that conform to the Bump format
 *   * #MB_BI_WARN if this is a bid that can't be won
 *   * #MB_BI_FAILED if any file operations fail non-fatally
 *   * #MB_BI_FATAL if any file operations fail fatally
 */
int bump_reader_bid(MbBiReader *bir, void *userdata, int best_bid)
{
    AndroidReaderCtx *const ctx = static_cast<AndroidReaderCtx *>(userdata);
    int bid = 0;
    int ret;

    if (best_bid >= (ANDROID_BOOT_MAGIC_SIZE + BUMP_MAGIC_SIZE) * 8) {
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

    // Find the Bump magic
    ret = find_bump_magic(bir, bir->file, &ctx->hdr, &ctx->bump_offset);
    if (ret == MB_BI_OK) {
        // Update bid to account for matched bits
        ctx->have_bump_offset = true;
        bid += BUMP_MAGIC_SIZE * 8;
    } else if (ret == MB_BI_WARN) {
        // Nothing found. Don't change bid
    } else {
        return ret;
    }

    return bid;
}

int android_reader_set_option(MbBiReader *bir, void *userdata,
                              const char *key, const char *value)
{
    (void) bir;

    AndroidReaderCtx *const ctx = static_cast<AndroidReaderCtx *>(userdata);

    if (strcmp(key, "strict") == 0) {
        bool strict = strcasecmp(value, "true") == 0
                || strcasecmp(value, "yes") == 0
                || strcasecmp(value, "y") == 0
                || strcmp(value, "1") == 0;
        ctx->allow_truncated_dt = !strict;
        return MB_BI_OK;
    } else {
        return MB_BI_WARN;
    }
}

int android_reader_read_header(MbBiReader *bir, void *userdata,
                               MbBiHeader *header)
{
    AndroidReaderCtx *const ctx = static_cast<AndroidReaderCtx *>(userdata);
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

    ret = _segment_reader_entries_add(&ctx->segctx, MB_BI_ENTRY_KERNEL,
                                      kernel_offset, ctx->hdr.kernel_size,
                                      false, bir);
    if (ret != MB_BI_OK) return ret;

    ret = _segment_reader_entries_add(&ctx->segctx, MB_BI_ENTRY_RAMDISK,
                                      ramdisk_offset, ctx->hdr.ramdisk_size,
                                      false, bir);
    if (ret != MB_BI_OK) return ret;

    if (ctx->hdr.second_size > 0) {
        ret = _segment_reader_entries_add(&ctx->segctx, MB_BI_ENTRY_SECONDBOOT,
                                          second_offset, ctx->hdr.second_size,
                                          false, bir);
        if (ret != MB_BI_OK) return ret;
    }

    if (ctx->hdr.dt_size > 0) {
        ret = _segment_reader_entries_add(&ctx->segctx, MB_BI_ENTRY_DEVICE_TREE,
                                          dt_offset, ctx->hdr.dt_size,
                                          ctx->allow_truncated_dt, bir);
        if (ret != MB_BI_OK) return ret;
    }

    return MB_BI_OK;
}

int android_reader_read_entry(MbBiReader *bir, void *userdata,
                              MbBiEntry *entry)
{
    AndroidReaderCtx *const ctx = static_cast<AndroidReaderCtx *>(userdata);

    return _segment_reader_read_entry(&ctx->segctx, bir->file, entry, bir);
}

int android_reader_go_to_entry(MbBiReader *bir, void *userdata,
                               MbBiEntry *entry, int entry_type)
{
    AndroidReaderCtx *const ctx = static_cast<AndroidReaderCtx *>(userdata);

    return _segment_reader_go_to_entry(&ctx->segctx, bir->file, entry,
                                       entry_type, bir);
}

int android_reader_read_data(MbBiReader *bir, void *userdata,
                             void *buf, size_t buf_size,
                             size_t *bytes_read)
{
    AndroidReaderCtx *const ctx = static_cast<AndroidReaderCtx *>(userdata);

    return _segment_reader_read_data(&ctx->segctx, bir->file, buf, buf_size,
                                     bytes_read, bir);
}

int android_reader_free(MbBiReader *bir, void *userdata)
{
    (void) bir;
    AndroidReaderCtx *const ctx = static_cast<AndroidReaderCtx *>(userdata);
    _segment_reader_deinit(&ctx->segctx);
    free(ctx);
    return MB_BI_OK;
}

/*!
 * \brief Enable support for Android boot image format
 *
 * \param bir MbBiReader
 *
 * \return
 *   * #MB_BI_OK if the format is successfully enabled
 *   * #MB_BI_WARN if the format is already enabled
 *   * \<= #MB_BI_FAILED if an error occurs
 */
int mb_bi_reader_enable_format_android(MbBiReader *bir)
{
    AndroidReaderCtx *const ctx = static_cast<AndroidReaderCtx *>(
            calloc(1, sizeof(AndroidReaderCtx)));
    if (!ctx) {
        mb_bi_reader_set_error(bir, -errno,
                               "Failed to allocate AndroidReaderCtx: %s",
                               strerror(errno));
        return MB_BI_FAILED;
    }

    _segment_reader_init(&ctx->segctx);

    // Allow truncated dt image by default
    ctx->allow_truncated_dt = true;

    return _mb_bi_reader_register_format(bir,
                                         ctx,
                                         MB_BI_FORMAT_ANDROID,
                                         MB_BI_FORMAT_NAME_ANDROID,
                                         &android_reader_bid,
                                         &android_reader_set_option,
                                         &android_reader_read_header,
                                         &android_reader_read_entry,
                                         &android_reader_go_to_entry,
                                         &android_reader_read_data,
                                         &android_reader_free);
}

MB_END_C_DECLS
