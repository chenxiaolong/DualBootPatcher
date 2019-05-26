/*
 * Copyright (C) 2015-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include "mbcommon/file_error.h"
#include "mbcommon/file_util.h"
#include "mbcommon/string.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/format/align_p.h"
#include "mbbootimg/format/android_error.h"
#include "mbbootimg/format/android_reader_p.h"
#include "mbbootimg/format/mtk_error.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader_p.h"


namespace mb::bootimg::mtk
{

/*!
 * \brief Read MTK header at offset
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use File::seek() to return to a known position.
 *
 * \param file File handle
 * \param offset Offset to read MTK header
 *
 * \return
 *   * The MTK header in host byte order if it is found and valid
 *   * An MtkError if the header is not found
 *   * A specific error if any file operation fails
 */
static oc::result<MtkHeader>
read_mtk_header(File &file, uint64_t offset)
{
    OUTCOME_TRYV(file.seek(static_cast<int64_t>(offset), SEEK_SET));

    MtkHeader hdr;

    auto ret = file_read_exact(file, &hdr, sizeof(hdr));
    if (!ret) {
        if (ret.error() == FileError::UnexpectedEof) {
            //DEBUG("MTK header not found at %" PRIu64, offset);
            return MtkError::MtkHeaderNotFound;
        } else {
            return ret.as_failure();
        }
    }

    if (memcmp(hdr.magic, MTK_MAGIC, MTK_MAGIC_SIZE) != 0) {
        //DEBUG("MTK header not found at %" PRIu64, offset);
        return MtkError::MtkHeaderNotFound;
    }

    mtk_fix_header_byte_order(hdr);

    return std::move(hdr);
}

/*!
 * \brief Find location of the MTK kernel and ramdisk headers
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use File::seek() to return to a known position.
 *
 * \param file File handle
 * \param ahdr Android boot image header (in host byte order)
 *
 * \return
 *   * If the headers are found, a tuple containing (in order): the kernel MTK
 *     header, the kernel image offset, the ramdisk MTK header, the ramdisk
 *     image offset
 *   * An MtkError if the kernel or ramdisk header is not found
 *   * A specific error if any file operation fails
 */
static oc::result<std::tuple<MtkHeader, uint64_t, MtkHeader, uint64_t>>
find_mtk_headers(File &file, const android::AndroidHeader &ahdr)
{
    uint64_t pos = 0;

    // Header
    pos += ahdr.page_size;

    // Kernel
    auto kernel_offset = pos;
    pos += ahdr.kernel_size;
    pos += align_page_size<uint64_t>(pos, ahdr.page_size);

    // Ramdisk
    auto ramdisk_offset = pos;

    OUTCOME_TRY(kernel_mtkhdr, read_mtk_header(file, kernel_offset));
    OUTCOME_TRY(ramdisk_mtkhdr, read_mtk_header(file, ramdisk_offset));

    return {
        std::move(kernel_mtkhdr),
        kernel_offset + sizeof(MtkHeader),
        std::move(ramdisk_mtkhdr),
        ramdisk_offset + sizeof(MtkHeader)
    };
}

MtkFormatReader::MtkFormatReader() noexcept
    : FormatReader()
    , m_hdr()
    , m_mtk_kernel_hdr()
    , m_mtk_ramdisk_hdr()
{
}

MtkFormatReader::~MtkFormatReader() noexcept = default;

Format MtkFormatReader::type()
{
    return Format::Mtk;
}

/*!
 * \brief Perform a bid
 *
 * \return
 *   * If \>= 0, the number of bits that conform to the MTK format
 *   * If \< 0, the bid cannot be won
 *   * A specific error code
 */
oc::result<int> MtkFormatReader::open(File &file, int best_bid)
{
    int bid = 0;

    if (best_bid >= static_cast<int>(
            android::BOOT_MAGIC_SIZE + 2 * MTK_MAGIC_SIZE) * 8) {
        // This is a bid we can't win, so bail out
        return -1;
    }

    // Find the Android header
    auto ahdr = android::AndroidFormatReader::find_header(
            file, android::MAX_HEADER_OFFSET);
    if (ahdr) {
        // Update bid to account for matched bits
        m_hdr = std::move(ahdr.value().first);
        m_hdr_offset = ahdr.value().second;
        bid += static_cast<int>(android::BOOT_MAGIC_SIZE * 8);
    } else if (ahdr.error() == android::AndroidError::HeaderNotFound
            || ahdr.error() == android::AndroidError::HeaderOutOfBounds) {
        // Header not found. This can't be an Android boot image.
        return 0;
    } else {
        return ahdr.as_failure();
    }

    auto ret = find_mtk_headers(file, m_hdr);
    if (ret) {
        // Update bid to account for matched bids
        std::tie(m_mtk_kernel_hdr, m_mtk_kernel_offset,
                 m_mtk_ramdisk_hdr, m_mtk_ramdisk_offset) = ret.value();
        bid += static_cast<int>(2 * MTK_MAGIC_SIZE * 8);
    } else if (ret.error().category() == mtk_error_category()) {
        // Headers not found. This can't be an MTK boot image.
        return 0;
    } else {
        return ret.as_failure();
    }

    m_seg = SegmentReader();

    return bid;
}

oc::result<void> MtkFormatReader::close(File &file)
{
    (void) file;

    m_hdr = {};
    m_mtk_kernel_hdr = {};
    m_mtk_ramdisk_hdr = {};
    m_hdr_offset = {};
    m_mtk_kernel_offset = {};
    m_mtk_ramdisk_offset = {};
    m_seg = {};

    return oc::success();
}

oc::result<Header> MtkFormatReader::read_header(File &file)
{
    (void) file;

    // Validate that the kernel and ramdisk sizes are consistent
    if (m_hdr.kernel_size != static_cast<uint64_t>(
            m_mtk_kernel_hdr.size) + sizeof(MtkHeader)) {
        return MtkError::MismatchedKernelSizeInHeaders;
    }
    if (m_hdr.ramdisk_size != static_cast<uint64_t>(
            m_mtk_ramdisk_hdr.size) + sizeof(MtkHeader)) {
        return MtkError::MismatchedRamdiskSizeInHeaders;
    }

    auto header = android::AndroidFormatReader::convert_header(m_hdr);

    // Calculate offsets for each section

    uint64_t pos = 0;
    uint32_t page_size = *header.page_size();

    // pos cannot overflow due to the nature of the operands (adding UINT32_MAX
    // a few times can't overflow a uint64_t). File length overflow is checked
    // during read.

    // Header
    pos += *m_hdr_offset;
    pos += sizeof(android::AndroidHeader);
    pos += align_page_size<uint64_t>(pos, page_size);

    // Kernel
    uint64_t kernel_offset = pos;
    pos += m_hdr.kernel_size;
    pos += align_page_size<uint64_t>(pos, page_size);

    // Ramdisk
    uint64_t ramdisk_offset = pos;
    pos += m_hdr.ramdisk_size;
    pos += align_page_size<uint64_t>(pos, page_size);

    // Second bootloader
    uint64_t second_offset = pos;
    pos += m_hdr.second_size;
    pos += align_page_size<uint64_t>(pos, page_size);

    // Device tree
    uint64_t dt_offset = pos;

    std::vector<SegmentReaderEntry> entries;

    entries.push_back({
        EntryType::MtkKernelHeader, kernel_offset, sizeof(MtkHeader), false,
    });
    entries.push_back({
        EntryType::Kernel, *m_mtk_kernel_offset, m_mtk_kernel_hdr.size, false,
    });
    entries.push_back({
        EntryType::MtkRamdiskHeader, ramdisk_offset, sizeof(MtkHeader), false,
    });
    entries.push_back({
        EntryType::Ramdisk, *m_mtk_ramdisk_offset, m_mtk_ramdisk_hdr.size, false,
    });
    if (m_hdr.second_size > 0) {
        entries.push_back({
            EntryType::SecondBoot, second_offset, m_hdr.second_size, false,
        });
    }
    if (m_hdr.dt_size > 0) {
        entries.push_back({
            EntryType::DeviceTree, dt_offset, m_hdr.dt_size, false,
        });
    }

    OUTCOME_TRYV(m_seg->set_entries(std::move(entries)));

    return std::move(header);
}

oc::result<Entry> MtkFormatReader::read_entry(File &file)
{
    return m_seg->read_entry(file);
}

oc::result<Entry>
MtkFormatReader::go_to_entry(File &file, std::optional<EntryType> entry_type)
{
    return m_seg->go_to_entry(file, entry_type);
}

oc::result<size_t>
MtkFormatReader::read_data(File &file, void *buf, size_t buf_size)
{
    return m_seg->read_data(file, buf, buf_size);
}

}
