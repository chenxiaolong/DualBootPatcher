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
#include "mbbootimg/format/android_error.h"
#include "mbbootimg/format/bump_defs.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"


namespace mb
{
namespace bootimg
{
namespace android
{

AndroidFormatReader::AndroidFormatReader(Reader &reader, bool is_bump)
    : FormatReader(reader)
    , _hdr()
    // Allow truncated device tree image by default
    , _allow_truncated_dt(true)
    , _is_bump(is_bump)
{
}

AndroidFormatReader::~AndroidFormatReader() = default;

int AndroidFormatReader::type()
{
    if (_is_bump) {
        return FORMAT_BUMP;
    } else {
        return FORMAT_ANDROID;
    }
}

std::string AndroidFormatReader::name()
{
    if (_is_bump) {
        return FORMAT_NAME_BUMP;
    } else {
        return FORMAT_NAME_ANDROID;
    }
}

int AndroidFormatReader::set_option(const char *key, const char *value)
{
    if (strcmp(key, "strict") == 0) {
        bool strict = strcasecmp(value, "true") == 0
                || strcasecmp(value, "yes") == 0
                || strcasecmp(value, "y") == 0
                || strcmp(value, "1") == 0;
        _allow_truncated_dt = !strict;
        return RET_OK;
    } else {
        return RET_WARN;
    }
}

int AndroidFormatReader::bid(File &file, int best_bid)
{
    if (_is_bump) {
        return bid_bump(file, best_bid);
    } else {
        return bid_android(file, best_bid);
    }
}

int AndroidFormatReader::read_header(File &file, Header &header)
{
    int ret;

    if (!_header_offset) {
        // A bid might not have been performed if the user forced a particular
        // format
        uint64_t header_offset;
        ret = find_header(_reader, file, MAX_HEADER_OFFSET, _hdr,
                          header_offset);
        if (ret < 0) {
            return ret;
        }
        _header_offset = header_offset;
    }

    ret = convert_header(_hdr, header);
    if (ret != RET_OK) {
        _reader.set_error(make_error_code(AndroidError::HeaderSetFieldsFailed),
                          "Failed to set header fields");
        return ret;
    }

    // Calculate offsets for each section

    uint64_t pos = 0;
    uint32_t page_size = *header.page_size();
    uint64_t kernel_offset;
    uint64_t ramdisk_offset;
    uint64_t second_offset;
    uint64_t dt_offset;

    // pos cannot overflow due to the nature of the operands (adding UINT32_MAX
    // a few times can't overflow a uint64_t). File length overflow is checked
    // during read.

    // Header
    pos += *_header_offset;
    pos += sizeof(AndroidHeader);
    pos += align_page_size<uint64_t>(pos, page_size);

    // Kernel
    kernel_offset = pos;
    pos += _hdr.kernel_size;
    pos += align_page_size<uint64_t>(pos, page_size);

    // Ramdisk
    ramdisk_offset = pos;
    pos += _hdr.ramdisk_size;
    pos += align_page_size<uint64_t>(pos, page_size);

    // Second bootloader
    second_offset = pos;
    pos += _hdr.second_size;
    pos += align_page_size<uint64_t>(pos, page_size);

    // Device tree
    dt_offset = pos;
    pos += _hdr.dt_size;
    pos += align_page_size<uint64_t>(pos, page_size);

    _seg.entries_clear();

    ret = _seg.entries_add(ENTRY_TYPE_KERNEL,
                           kernel_offset, _hdr.kernel_size, false, _reader);
    if (ret != RET_OK) { return ret; }

    ret = _seg.entries_add(ENTRY_TYPE_RAMDISK,
                           ramdisk_offset, _hdr.ramdisk_size, false, _reader);
    if (ret != RET_OK) { return ret; }

    if (_hdr.second_size > 0) {
        ret = _seg.entries_add(ENTRY_TYPE_SECONDBOOT,
                               second_offset, _hdr.second_size, false, _reader);
        if (ret != RET_OK) { return ret; }
    }

    if (_hdr.dt_size > 0) {
        ret = _seg.entries_add(ENTRY_TYPE_DEVICE_TREE,
                               dt_offset, _hdr.dt_size, _allow_truncated_dt,
                               _reader);
        if (ret != RET_OK) { return ret; }
    }

    return RET_OK;
}

int AndroidFormatReader::read_entry(File &file, Entry &entry)
{
    return _seg.read_entry(file, entry, _reader);
}

int AndroidFormatReader::go_to_entry(File &file, Entry &entry, int entry_type)
{
    return _seg.go_to_entry(file, entry, entry_type, _reader);
}

int AndroidFormatReader::read_data(File &file, void *buf, size_t buf_size,
                                   size_t &bytes_read)
{
    return _seg.read_data(file, buf, buf_size, bytes_read, _reader);
}

/*!
 * \brief Find and read Android boot image header
 *
 * \note The integral fields in the header will be converted to the host's byte
 *       order.
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use File::seek() to return to a known position.
 *
 * \param[in] reader Reader
 * \param[in] file File handle
 * \param[in] max_header_offset Maximum offset that a header can start (must be
 *                              less than #MAX_HEADER_OFFSET)
 * \param[out] header_out Pointer to store header
 * \param[out] offset_out Pointer to store header offset
 *
 * \return
 *   * #RET_OK if the header is found
 *   * #RET_WARN if the header is not found
 *   * #RET_FAILED if any file operation fails non-fatally
 *   * #RET_FATAL if any file operation fails fatally
 */
int AndroidFormatReader::find_header(Reader &reader, File &file,
                                     uint64_t max_header_offset,
                                     AndroidHeader &header_out,
                                     uint64_t &offset_out)
{
    unsigned char buf[MAX_HEADER_OFFSET + sizeof(AndroidHeader)];
    size_t n;
    void *ptr;
    size_t offset;

    if (max_header_offset > MAX_HEADER_OFFSET) {
        reader.set_error(make_error_code(AndroidError::InvalidArgument),
                         "Max header offset (%" PRIu64
                         ") must be less than %" MB_PRIzu,
                         max_header_offset, MAX_HEADER_OFFSET);
        return RET_WARN;
    }

    if (!file.seek(0, SEEK_SET, nullptr)) {
        reader.set_error(file.error(),
                         "Failed to seek to beginning: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    if (!file_read_fully(file, buf,
                         max_header_offset + sizeof(AndroidHeader), n)) {
        reader.set_error(file.error(),
                         "Failed to read header: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    ptr = mb_memmem(buf, n, BOOT_MAGIC, BOOT_MAGIC_SIZE);
    if (!ptr) {
        reader.set_error(make_error_code(AndroidError::HeaderNotFound),
                         "Android magic not found in first %" MB_PRIzu " bytes",
                         MAX_HEADER_OFFSET);
        return RET_WARN;
    }

    offset = static_cast<unsigned char *>(ptr) - buf;

    if (n - offset < sizeof(AndroidHeader)) {
        reader.set_error(make_error_code(AndroidError::HeaderOutOfBounds),
                         "Android header at %" MB_PRIzu " exceeds file size",
                         offset);
        return RET_WARN;
    }

    // Copy header
    memcpy(&header_out, ptr, sizeof(AndroidHeader));
    android_fix_header_byte_order(header_out);
    offset_out = offset;

    return RET_OK;
}

/*!
 * \brief Find location of Samsung SEAndroid magic
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use File::seek() to return to a known position.
 *
 * \param[in] reader Reader
 * \param[in] file File handle
 * \param[in] hdr Android boot image header (in host byte order)
 * \param[out] offset_out Pointer to store magic offset
 *
 * \return
 *   * #RET_OK if the magic is found
 *   * #RET_WARN if the magic is not found
 *   * #RET_FAILED if any file operation fails non-fatally
 *   * #RET_FATAL if any file operation fails fatally
 */
int AndroidFormatReader::find_samsung_seandroid_magic(Reader &reader,
                                                      File &file,
                                                      const AndroidHeader &hdr,
                                                      uint64_t &offset_out)
{
    unsigned char buf[SAMSUNG_SEANDROID_MAGIC_SIZE];
    size_t n;
    uint64_t pos = 0;

    // Skip header, whose size cannot exceed the page size
    pos += hdr.page_size;

    // Skip kernel
    pos += hdr.kernel_size;
    pos += align_page_size<uint64_t>(pos, hdr.page_size);

    // Skip ramdisk
    pos += hdr.ramdisk_size;
    pos += align_page_size<uint64_t>(pos, hdr.page_size);

    // Skip second bootloader
    pos += hdr.second_size;
    pos += align_page_size<uint64_t>(pos, hdr.page_size);

    // Skip device tree
    pos += hdr.dt_size;
    pos += align_page_size<uint64_t>(pos, hdr.page_size);

    if (!file.seek(pos, SEEK_SET, nullptr)) {
        reader.set_error(file.error(),
                         "SEAndroid magic not found: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    if (!file_read_fully(file, buf, sizeof(buf), n)) {
        reader.set_error(file.error(),
                         "Failed to read SEAndroid magic: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    if (n != SAMSUNG_SEANDROID_MAGIC_SIZE
            || memcmp(buf, SAMSUNG_SEANDROID_MAGIC, n) != 0) {
        reader.set_error(make_error_code(AndroidError::SamsungMagicNotFound),
                         "SEAndroid magic not found in last %" MB_PRIzu
                         " bytes", SAMSUNG_SEANDROID_MAGIC_SIZE);
        return RET_WARN;
    }

    offset_out = pos;
    return RET_OK;
}

/*!
 * \brief Find location of Bump magic
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use File::seek() to return to a known position.
 *
 * \param[in] reader Reader
 * \param[in] file File handle
 * \param[in] hdr Android boot image header (in host byte order)
 * \param[out] offset_out Pointer to store magic offset
 *
 * \return
 *   * #RET_OK if the magic is found
 *   * #RET_WARN if the magic is not found
 *   * #RET_FAILED if any file operation fails non-fatally
 *   * #RET_FATAL if any file operation fails fatally
 */
int AndroidFormatReader::find_bump_magic(Reader &reader, File &file,
                                         const AndroidHeader &hdr,
                                         uint64_t &offset_out)
{
    unsigned char buf[bump::BUMP_MAGIC_SIZE];
    size_t n;
    uint64_t pos = 0;

    // Skip header, whose size cannot exceed the page size
    pos += hdr.page_size;

    // Skip kernel
    pos += hdr.kernel_size;
    pos += align_page_size<uint64_t>(pos, hdr.page_size);

    // Skip ramdisk
    pos += hdr.ramdisk_size;
    pos += align_page_size<uint64_t>(pos, hdr.page_size);

    // Skip second bootloader
    pos += hdr.second_size;
    pos += align_page_size<uint64_t>(pos, hdr.page_size);

    // Skip device tree
    pos += hdr.dt_size;
    pos += align_page_size<uint64_t>(pos, hdr.page_size);

    if (!file.seek(pos, SEEK_SET, nullptr)) {
        reader.set_error(file.error(),
                         "SEAndroid magic not found: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    if (!file_read_fully(file, buf, sizeof(buf), n)) {
        reader.set_error(file.error(),
                         "Failed to read SEAndroid magic: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    if (n != bump::BUMP_MAGIC_SIZE || memcmp(buf, bump::BUMP_MAGIC, n) != 0) {
        reader.set_error(make_error_code(AndroidError::BumpMagicNotFound),
                         "Bump magic not found in last %" MB_PRIzu " bytes",
                         bump::BUMP_MAGIC_SIZE);
        return RET_WARN;
    }

    offset_out = pos;
    return RET_OK;
}

int AndroidFormatReader::convert_header(const AndroidHeader &hdr,
                                        Header &header)
{
    char board_name[sizeof(hdr.name) + 1];
    char cmdline[sizeof(hdr.cmdline) + 1];

    strncpy(board_name, reinterpret_cast<const char *>(hdr.name),
            sizeof(hdr.name));
    strncpy(cmdline, reinterpret_cast<const char *>(hdr.cmdline),
            sizeof(hdr.cmdline));
    board_name[sizeof(hdr.name)] = '\0';
    cmdline[sizeof(hdr.cmdline)] = '\0';

    header.set_supported_fields(SUPPORTED_FIELDS);

    if (!header.set_board_name({board_name})
            || !header.set_kernel_cmdline({cmdline})
            || !header.set_page_size(hdr.page_size)
            || !header.set_kernel_address(hdr.kernel_addr)
            || !header.set_ramdisk_address(hdr.ramdisk_addr)
            || !header.set_secondboot_address(hdr.second_addr)
            || !header.set_kernel_tags_address(hdr.tags_addr)) {
        return RET_UNSUPPORTED;
    }

    // TODO: unused
    // TODO: id

    return RET_OK;
}

/*!
 * \brief Perform a bid
 *
 * \return
 *   * If \>= 0, the number of bits that conform to the Android format
 *   * #RET_WARN if this is a bid that can't be won
 *   * #RET_FAILED if any file operations fail non-fatally
 *   * #RET_FATAL if any file operations fail fatally
 */
int AndroidFormatReader::bid_android(File &file, int best_bid)
{
    int bid = 0;
    int ret;

    if (best_bid >= static_cast<int>(
            BOOT_MAGIC_SIZE + SAMSUNG_SEANDROID_MAGIC_SIZE) * 8) {
        // This is a bid we can't win, so bail out
        return RET_WARN;
    }

    // Find the Android header
    uint64_t header_offset;
    ret = find_header(_reader, file, MAX_HEADER_OFFSET, _hdr, header_offset);
    if (ret == RET_OK) {
        // Update bid to account for matched bits
        _header_offset = header_offset;
        bid += BOOT_MAGIC_SIZE * 8;
    } else if (ret == RET_WARN) {
        // Header not found. This can't be an Android boot image.
        return 0;
    } else {
        return ret;
    }

    // Find the Samsung magic
    uint64_t samsung_offset;
    ret = find_samsung_seandroid_magic(_reader, file, _hdr, samsung_offset);
    if (ret == RET_OK) {
        // Update bid to account for matched bits
        _samsung_offset = samsung_offset;
        bid += SAMSUNG_SEANDROID_MAGIC_SIZE * 8;
    } else if (ret == RET_WARN) {
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
 *   * #RET_WARN if this is a bid that can't be won
 *   * #RET_FAILED if any file operations fail non-fatally
 *   * #RET_FATAL if any file operations fail fatally
 */
int AndroidFormatReader::bid_bump(File &file, int best_bid)
{
    int bid = 0;
    int ret;

    if (best_bid >= static_cast<int>(
            BOOT_MAGIC_SIZE + bump::BUMP_MAGIC_SIZE) * 8) {
        // This is a bid we can't win, so bail out
        return RET_WARN;
    }

    // Find the Android header
    uint64_t header_offset;
    ret = find_header(_reader, file, MAX_HEADER_OFFSET, _hdr, header_offset);
    if (ret == RET_OK) {
        // Update bid to account for matched bits
        _header_offset = header_offset;
        bid += BOOT_MAGIC_SIZE * 8;
    } else if (ret == RET_WARN) {
        // Header not found. This can't be an Android boot image.
        return 0;
    } else {
        return ret;
    }

    // Find the Bump magic
    uint64_t bump_offset;
    ret = find_bump_magic(_reader, file, _hdr, bump_offset);
    if (ret == RET_OK) {
        // Update bid to account for matched bits
        _bump_offset = bump_offset;
        bid += bump::BUMP_MAGIC_SIZE * 8;
    } else if (ret == RET_WARN) {
        // Nothing found. Don't change bid
    } else {
        return ret;
    }

    return bid;
}

}

/*!
 * \brief Enable support for Android boot image format
 *
 * \return
 *   * #RET_OK if the format is successfully enabled
 *   * #RET_WARN if the format is already enabled
 *   * \<= #RET_FAILED if an error occurs
 */
int Reader::enable_format_android()
{
    using namespace android;

    MB_PRIVATE(Reader);

    std::unique_ptr<FormatReader> format{new AndroidFormatReader(*this, false)};
    return priv->register_format(std::move(format));
}

}
}
