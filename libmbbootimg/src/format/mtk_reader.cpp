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
#include "mbbootimg/format/android_error.h"
#include "mbbootimg/format/android_reader_p.h"
#include "mbbootimg/format/mtk_error.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"


namespace mb::bootimg
{
namespace mtk
{

/*!
 * \brief Read MTK header
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use File::seek() to return to a known position.
 *
 * \param[in] reader Reader for setting error messages
 * \param[in] file File handle
 * \param[in] offset Offset to read MTK header
 * \param[out] mtkhdr_out Pointer to store MTK header (in host byte order)
 *
 * \return
 *   * Nothing if the header is found
 *   * An MtkError if the header is not found
 *   * A specific error if any file operation fails
 */
static oc::result<void>
read_mtk_header(Reader &reader, File &file,
                uint64_t offset, MtkHeader &mtkhdr_out)
{
    MtkHeader mtkhdr;

    OUTCOME_TRYV(file.seek(static_cast<int64_t>(offset), SEEK_SET));

    auto ret = file_read_exact(file, &mtkhdr, sizeof(mtkhdr));
    if (!ret) {
        if (ret.error() == FileError::UnexpectedEof) {
            //DEBUG("MTK header not found at %" PRIu64, offset);
            return MtkError::MtkHeaderNotFound;
        } else {
            return ret.as_failure();
        }
    }

    if (memcmp(mtkhdr.magic, MTK_MAGIC, MTK_MAGIC_SIZE) != 0) {
        //DEBUG("MTK header not found at %" PRIu64, offset);
        return MtkError::MtkHeaderNotFound;
    }

    mtkhdr_out = mtkhdr;
    mtk_fix_header_byte_order(mtkhdr_out);

    return oc::success();
}

/*!
 * \brief Find location of the MTK kernel and ramdisk headers
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use File::seek() to return to a known position.
 *
 * \param[in] reader Reader for setting error messages
 * \param[in] file File handle
 * \param[in] hdr Android boot image header (in host byte order)
 * \param[out] kernel_mtkhdr_out Pointer to store kernel MTK header
 *                               (in host byte order)
 * \param[out] kernel_offset_out Pointer to store offset of kernel image
 * \param[out] ramdisk_mtkhdr_out Pointer to store ramdisk MTK header
 *                                (in host byte order)
 * \param[out] ramdisk_offset_out Pointer to store offset of ramdisk image
 *
 * \return
 *   * Nothing if both the kernel and ramdisk headers are found
 *   * An MtkError if the kernel or ramdisk header is not found
 *   * A specific error if any file operation fails
 */
static oc::result<void>
find_mtk_headers(Reader &reader, File &file,
                 const android::AndroidHeader &hdr,
                 MtkHeader &kernel_mtkhdr_out,
                 uint64_t &kernel_offset_out,
                 MtkHeader &ramdisk_mtkhdr_out,
                 uint64_t &ramdisk_offset_out)
{
    uint64_t kernel_offset;
    uint64_t ramdisk_offset;
    uint64_t pos = 0;

    // Header
    pos += hdr.page_size;

    // Kernel
    kernel_offset = pos;
    pos += hdr.kernel_size;
    pos += align_page_size<uint64_t>(pos, hdr.page_size);

    // Ramdisk
    ramdisk_offset = pos;
    pos += hdr.ramdisk_size;
    pos += align_page_size<uint64_t>(pos, hdr.page_size);

    OUTCOME_TRYV(read_mtk_header(reader, file, kernel_offset,
                                 kernel_mtkhdr_out));
    kernel_offset_out = kernel_offset + sizeof(MtkHeader);

    OUTCOME_TRYV(read_mtk_header(reader, file, ramdisk_offset,
                                 ramdisk_mtkhdr_out));
    ramdisk_offset_out = ramdisk_offset + sizeof(MtkHeader);

    return oc::success();
}

MtkFormatReader::MtkFormatReader(Reader &reader)
    : FormatReader(reader)
    , m_hdr()
    , m_mtk_kernel_hdr()
    , m_mtk_ramdisk_hdr()
{
}

MtkFormatReader::~MtkFormatReader() = default;

int MtkFormatReader::type()
{
    return FORMAT_MTK;
}

std::string MtkFormatReader::name()
{
    return FORMAT_NAME_MTK;
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
    uint64_t header_offset;
    auto ret = android::AndroidFormatReader::find_header(
            m_reader, file, android::MAX_HEADER_OFFSET, m_hdr, header_offset);
    if (ret) {
        // Update bid to account for matched bits
        m_header_offset = header_offset;
        bid += static_cast<int>(android::BOOT_MAGIC_SIZE * 8);
    } else if (ret.error() == android::AndroidError::HeaderNotFound
            || ret.error() == android::AndroidError::HeaderOutOfBounds) {
        // Header not found. This can't be an Android boot image.
        return 0;
    } else {
        return ret.as_failure();
    }

    uint64_t mtk_kernel_offset;
    uint64_t mtk_ramdisk_offset;
    ret = find_mtk_headers(m_reader, file, m_hdr,
                           m_mtk_kernel_hdr, mtk_kernel_offset,
                           m_mtk_ramdisk_hdr, mtk_ramdisk_offset);
    if (ret) {
        // Update bid to account for matched bids
        m_mtk_kernel_offset = mtk_kernel_offset;
        m_mtk_ramdisk_offset = mtk_ramdisk_offset;
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
    m_header_offset = {};
    m_mtk_kernel_offset = {};
    m_mtk_ramdisk_offset = {};
    m_seg = {};

    return oc::success();
}

oc::result<void> MtkFormatReader::read_header(File &file, Header &header)
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

    if (!android::AndroidFormatReader::convert_header(m_hdr, header)) {
        return MtkError::HeaderSetFieldsFailed;
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
    pos += sizeof(android::AndroidHeader);
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
        ENTRY_TYPE_MTK_KERNEL_HEADER, kernel_offset, sizeof(MtkHeader), false
    });
    entries.push_back({
        ENTRY_TYPE_KERNEL, *m_mtk_kernel_offset, m_mtk_kernel_hdr.size, false
    });
    entries.push_back({
        ENTRY_TYPE_MTK_RAMDISK_HEADER, ramdisk_offset, sizeof(MtkHeader), false
    });
    entries.push_back({
        ENTRY_TYPE_RAMDISK, *m_mtk_ramdisk_offset, m_mtk_ramdisk_hdr.size, false
    });
    if (m_hdr.second_size > 0) {
        entries.push_back({
            ENTRY_TYPE_SECONDBOOT, second_offset, m_hdr.second_size, false
        });
    }
    if (m_hdr.dt_size > 0) {
        entries.push_back({
            ENTRY_TYPE_DEVICE_TREE, dt_offset, m_hdr.dt_size, false
        });
    }

    return m_seg->set_entries(std::move(entries));
}

oc::result<void> MtkFormatReader::read_entry(File &file, Entry &entry)
{
    return m_seg->read_entry(file, entry, m_reader);
}

oc::result<void> MtkFormatReader::go_to_entry(File &file, Entry &entry,
                                              int entry_type)
{
    return m_seg->go_to_entry(file, entry, entry_type, m_reader);
}

oc::result<size_t> MtkFormatReader::read_data(File &file, void *buf,
                                              size_t buf_size)
{
    return m_seg->read_data(file, buf, buf_size, m_reader);
}

}

/*!
 * \brief Enable support for MTK boot image format
 *
 * \return Nothing if the format is successfully enabled. Otherwise, the error
 *         code.
 */
oc::result<void> Reader::enable_format_mtk()
{
    return register_format(std::make_unique<mtk::MtkFormatReader>(*this));
}

}
