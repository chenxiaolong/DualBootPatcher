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
#include <functional>
#include <type_traits>
#ifdef __ANDROID__
#  include <experimental/algorithm>
#  include <experimental/functional>
#endif

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
#include "mbbootimg/format/android_error.h"
#include "mbbootimg/format/bump_defs.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"


namespace mb::bootimg
{
namespace android
{

#ifdef __ANDROID__
namespace std2 = std::experimental;
#else
namespace std2 = std;
#endif

AndroidFormatReader::AndroidFormatReader(Reader &reader, bool is_bump)
    : FormatReader(reader)
    , m_is_bump(is_bump)
    , m_hdr()
    // Allow truncated device tree image by default
    , m_allow_truncated_dt(true)
{
}

AndroidFormatReader::~AndroidFormatReader() = default;

int AndroidFormatReader::type()
{
    if (m_is_bump) {
        return FORMAT_BUMP;
    } else {
        return FORMAT_ANDROID;
    }
}

std::string AndroidFormatReader::name()
{
    if (m_is_bump) {
        return FORMAT_NAME_BUMP;
    } else {
        return FORMAT_NAME_ANDROID;
    }
}

oc::result<void> AndroidFormatReader::set_option(const char *key,
                                                 const char *value)
{
    if (strcmp(key, "strict") == 0) {
        bool strict = strcasecmp(value, "true") == 0
                || strcasecmp(value, "yes") == 0
                || strcasecmp(value, "y") == 0
                || strcmp(value, "1") == 0;
        m_allow_truncated_dt = !strict;
        return oc::success();
    } else {
        return FormatReader::set_option(key, value);
    }
}

oc::result<int> AndroidFormatReader::open(File &file, int best_bid)
{
    if (m_is_bump) {
        return open_bump(file, best_bid);
    } else {
        return open_android(file, best_bid);
    }
}

oc::result<void> AndroidFormatReader::close(File &file)
{
    (void) file;

    m_hdr = {};
    m_header_offset = {};
    m_seg = {};

    return oc::success();
}

oc::result<void> AndroidFormatReader::read_header(File &file, Header &header)
{
    (void) file;

    if (!convert_header(m_hdr, header)) {
        return AndroidError::HeaderSetFieldsFailed;
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
    pos += *m_header_offset;
    pos += sizeof(AndroidHeader);
    pos += align_page_size<uint64_t>(pos, page_size);

    // Kernel
    kernel_offset = pos;
    pos += m_hdr.kernel_size;
    pos += align_page_size<uint64_t>(pos, page_size);

    // Ramdisk
    ramdisk_offset = pos;
    pos += m_hdr.ramdisk_size;
    pos += align_page_size<uint64_t>(pos, page_size);

    // Second bootloader
    second_offset = pos;
    pos += m_hdr.second_size;
    pos += align_page_size<uint64_t>(pos, page_size);

    // Device tree
    dt_offset = pos;
    pos += m_hdr.dt_size;
    pos += align_page_size<uint64_t>(pos, page_size);

    std::vector<SegmentReaderEntry> entries;

    entries.push_back({
        ENTRY_TYPE_KERNEL, kernel_offset, m_hdr.kernel_size, false
    });
    entries.push_back({
        ENTRY_TYPE_RAMDISK, ramdisk_offset, m_hdr.ramdisk_size, false
    });
    if (m_hdr.second_size > 0) {
        entries.push_back({
            ENTRY_TYPE_SECONDBOOT, second_offset, m_hdr.second_size, false
        });
    }
    if (m_hdr.dt_size > 0) {
        entries.push_back({
            ENTRY_TYPE_DEVICE_TREE, dt_offset, m_hdr.dt_size,
            m_allow_truncated_dt
        });
    }

    return m_seg->set_entries(std::move(entries));
}

oc::result<void> AndroidFormatReader::read_entry(File &file, Entry &entry)
{
    return m_seg->read_entry(file, entry, m_reader);
}

oc::result<void> AndroidFormatReader::go_to_entry(File &file, Entry &entry,
                                                  int entry_type)
{
    return m_seg->go_to_entry(file, entry, entry_type, m_reader);
}

oc::result<size_t> AndroidFormatReader::read_data(File &file, void *buf,
                                                  size_t buf_size)
{
    return m_seg->read_data(file, buf, buf_size, m_reader);
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
 *   * Nothing if the header is found
 *   * AndroidError::HeaderNotFound or AndroidError::HeaderOutOfBounds if the
 *     header is not found
 *   * A specific error code if any file operation fails
 */
oc::result<void>
AndroidFormatReader::find_header(Reader &reader, File &file,
                                 uint64_t max_header_offset,
                                 AndroidHeader &header_out,
                                 uint64_t &offset_out)
{
    unsigned char buf[MAX_HEADER_OFFSET + sizeof(AndroidHeader)];
    size_t offset;

    if (max_header_offset > MAX_HEADER_OFFSET) {
        //DEBUG("Max header offset (%" PRIu64 ") must be less than %" MB_PRIzu,
        //      max_header_offset, MAX_HEADER_OFFSET);
        return AndroidError::InvalidArgument;
    }

    OUTCOME_TRYV(file.seek(0, SEEK_SET));

    OUTCOME_TRY(n, file_read_retry(file, buf,
                                   static_cast<size_t>(max_header_offset)
                                       + sizeof(AndroidHeader)));

    auto buf_end = buf + n;
    auto it = std2::search(
            buf, buf_end, std2::boyer_moore_searcher<const unsigned char *>(
                    BOOT_MAGIC, BOOT_MAGIC + BOOT_MAGIC_SIZE));
    if (it == buf_end) {
        //DEBUG("Android magic not found in first %" MB_PRIzu " bytes",
        //      MAX_HEADER_OFFSET);
        return AndroidError::HeaderNotFound;
    }

    offset = static_cast<size_t>(it - buf);

    if (n - offset < sizeof(AndroidHeader)) {
        //DEBUG("Android header at %" MB_PRIzu " exceeds file size", offset);
        return AndroidError::HeaderOutOfBounds;
    }

    // Copy header
    memcpy(&header_out, it, sizeof(AndroidHeader));
    android_fix_header_byte_order(header_out);
    offset_out = offset;

    return oc::success();
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
 *   * Nothing if the magic is found
 *   * AndroidError::SamsungMagicNotFound if the magic is not found
 *   * A specific error code if any file operation fails
 */
oc::result<void>
AndroidFormatReader::find_samsung_seandroid_magic(Reader &reader, File &file,
                                                  const AndroidHeader &hdr,
                                                  uint64_t &offset_out)
{
    unsigned char buf[SAMSUNG_SEANDROID_MAGIC_SIZE];
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

    OUTCOME_TRYV(file.seek(static_cast<int64_t>(pos), SEEK_SET));

    OUTCOME_TRY(n, file_read_retry(file, buf, sizeof(buf)));

    if (n != SAMSUNG_SEANDROID_MAGIC_SIZE
            || memcmp(buf, SAMSUNG_SEANDROID_MAGIC, n) != 0) {
        //DEBUG("SEAndroid magic not found in last %" MB_PRIzu " bytes",
        //      SAMSUNG_SEANDROID_MAGIC_SIZE);
        return AndroidError::SamsungMagicNotFound;
    }

    offset_out = pos;
    return oc::success();
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
 *   * Nothing if the magic is found
 *   * AndroidError::BumpMagicNotFound if the magic is not found
 *   * A specific error code if any file operation fails
 */
oc::result<void>
AndroidFormatReader::find_bump_magic(Reader &reader, File &file,
                                     const AndroidHeader &hdr,
                                     uint64_t &offset_out)
{
    unsigned char buf[bump::BUMP_MAGIC_SIZE];
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

    OUTCOME_TRYV(file.seek(static_cast<int64_t>(pos), SEEK_SET));

    OUTCOME_TRY(n, file_read_retry(file, buf, sizeof(buf)));

    if (n != bump::BUMP_MAGIC_SIZE
            || memcmp(buf, bump::BUMP_MAGIC, n) != 0) {
        //DEBUG("Bump magic not found in last %" MB_PRIzu " bytes",
        //      bump::BUMP_MAGIC_SIZE);
        return AndroidError::BumpMagicNotFound;
    }

    offset_out = pos;
    return oc::success();
}

bool AndroidFormatReader::convert_header(const AndroidHeader &hdr,
                                         Header &header)
{
    auto *name_ptr = reinterpret_cast<const char *>(hdr.name);
    auto name_size = strnlen(name_ptr, sizeof(hdr.name));

    auto *cmdline_ptr = reinterpret_cast<const char *>(hdr.cmdline);
    auto cmdline_size = strnlen(cmdline_ptr, sizeof(hdr.cmdline));

    header.set_supported_fields(SUPPORTED_FIELDS);
    header.set_board_name({{name_ptr, name_size}});
    header.set_kernel_cmdline({{cmdline_ptr, cmdline_size}});
    header.set_page_size(hdr.page_size);
    header.set_kernel_address(hdr.kernel_addr);
    header.set_ramdisk_address(hdr.ramdisk_addr);
    header.set_secondboot_address(hdr.second_addr);
    header.set_kernel_tags_address(hdr.tags_addr);

    // TODO: unused
    // TODO: id

    return true;
}

/*!
 * \brief Perform a bid
 *
 * \return
 *   * If \>= 0, the number of bits that conform to the Bump format
 *   * If \< 0, the bid cannot be won
 *   * A specific error code
 */
oc::result<int> AndroidFormatReader::open_android(File &file, int best_bid)
{
    int bid = 0;

    if (best_bid >= static_cast<int>(
            BOOT_MAGIC_SIZE + SAMSUNG_SEANDROID_MAGIC_SIZE) * 8) {
        // This is a bid we can't win, so bail out
        return -1;
    }

    // Find the Android header
    uint64_t header_offset;
    auto ret = find_header(m_reader, file, MAX_HEADER_OFFSET, m_hdr,
                           header_offset);
    if (ret) {
        // Update bid to account for matched bits
        m_header_offset = header_offset;
        bid += static_cast<int>(BOOT_MAGIC_SIZE * 8);
    } else if (ret.error() == AndroidError::HeaderNotFound
            || ret.error() == AndroidError::HeaderOutOfBounds) {
        // Header not found. This can't be an Android boot image.
        return 0;
    } else {
        return ret.as_failure();
    }

    // Find the Samsung magic
    uint64_t samsung_offset;
    ret = find_samsung_seandroid_magic(m_reader, file, m_hdr, samsung_offset);
    if (ret) {
        // Update bid to account for matched bits
        bid += static_cast<int>(SAMSUNG_SEANDROID_MAGIC_SIZE * 8);
    } else if (ret.error() == AndroidError::SamsungMagicNotFound) {
        // Nothing found. Don't change bid
    } else {
        return ret.as_failure();
    }

    m_seg = SegmentReader();

    return bid;
}

/*!
 * \brief Perform a bid
 *
 * \return
 *   * If \>= 0, the number of bits that conform to the Bump format
 *   * If \< 0, the bid cannot be won
 *   * A specific error code
 */
oc::result<int> AndroidFormatReader::open_bump(File &file, int best_bid)
{
    int bid = 0;

    if (best_bid >= static_cast<int>(
            BOOT_MAGIC_SIZE + bump::BUMP_MAGIC_SIZE) * 8) {
        // This is a bid we can't win, so bail out
        return -1;
    }

    // Find the Android header
    uint64_t header_offset;
    auto ret = find_header(m_reader, file, MAX_HEADER_OFFSET, m_hdr,
                           header_offset);
    if (ret) {
        // Update bid to account for matched bits
        m_header_offset = header_offset;
        bid += static_cast<int>(BOOT_MAGIC_SIZE * 8);
    } else if (ret.error() == AndroidError::HeaderNotFound
            || ret.error() == AndroidError::HeaderOutOfBounds) {
        // Header not found. This can't be an Android boot image.
        return 0;
    } else {
        return ret.as_failure();
    }

    // Find the Bump magic
    uint64_t bump_offset;
    ret = find_bump_magic(m_reader, file, m_hdr, bump_offset);
    if (ret) {
        // Update bid to account for matched bits
        bid += static_cast<int>(bump::BUMP_MAGIC_SIZE * 8);
    } else if (ret.error() == AndroidError::BumpMagicNotFound) {
        // Nothing found. Don't change bid
    } else {
        return ret.as_failure();
    }

    m_seg = SegmentReader();

    return bid;
}

}

/*!
 * \brief Enable support for Android boot image format
 *
 * \return Nothing if the format is successfully enabled. Otherwise, the error
 *         code.
 */
oc::result<void> Reader::enable_format_android()
{
    return register_format(
            std::make_unique<android::AndroidFormatReader>(*this, false));
}

}
