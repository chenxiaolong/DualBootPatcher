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
#include "mbbootimg/format/mtk_error.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"


namespace mb
{
namespace bootimg
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
 *   * #RET_OK if the header is found
 *   * #RET_WARN if the header is not found
 *   * #RET_FAILED if any file operation fails non-fatally
 *   * #RET_FATAL if any file operation fails fatally
 */
static int read_mtk_header(Reader &reader, File &file,
                           uint64_t offset, MtkHeader &mtkhdr_out)
{
    MtkHeader mtkhdr;
    size_t n;

    if (!file.seek(static_cast<int64_t>(offset), SEEK_SET, nullptr)) {
        reader.set_error(file.error(),
                         "Failed to seek to MTK header at %" PRIu64 ": %s",
                         offset, file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    if (!file_read_fully(file, &mtkhdr, sizeof(mtkhdr), n)) {
        reader.set_error(file.error(),
                         "Failed to read MTK header: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    if (n != sizeof(MtkHeader)
            || memcmp(mtkhdr.magic, MTK_MAGIC, MTK_MAGIC_SIZE) != 0) {
        reader.set_error(make_error_code(MtkError::MtkHeaderNotFound),
                         "MTK header not found at %" PRIu64, offset);
        return RET_WARN;
    }

    mtkhdr_out = mtkhdr;
    mtk_fix_header_byte_order(mtkhdr_out);

    return RET_OK;
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
 *   * #RET_OK if both the kernel and ramdisk headers are found
 *   * #RET_WARN if the kernel or ramdisk header is not found
 *   * #RET_FAILED if any file operation fails non-fatally
 *   * #RET_FATAL if any file operation fails fatally
 */
static int find_mtk_headers(Reader &reader, File &file,
                            const android::AndroidHeader &hdr,
                            MtkHeader &kernel_mtkhdr_out,
                            uint64_t &kernel_offset_out,
                            MtkHeader &ramdisk_mtkhdr_out,
                            uint64_t &ramdisk_offset_out)
{
    uint64_t kernel_offset;
    uint64_t ramdisk_offset;
    int ret;
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

    ret = read_mtk_header(reader, file, kernel_offset, kernel_mtkhdr_out);
    if (ret == RET_OK) {
        kernel_offset_out = kernel_offset + sizeof(MtkHeader);
    } else {
        return ret;
    }

    ret = read_mtk_header(reader, file, ramdisk_offset, ramdisk_mtkhdr_out);
    if (ret == RET_OK) {
        ramdisk_offset_out = ramdisk_offset + sizeof(MtkHeader);
    } else {
        return ret;
    }

    return RET_OK;
}

MtkFormatReader::MtkFormatReader(Reader &reader)
    : FormatReader(reader)
    , _hdr()
    , _mtk_kernel_hdr()
    , _mtk_ramdisk_hdr()
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
 *   * #RET_WARN if this is a bid that can't be won
 *   * #RET_FAILED if any file operations fail non-fatally
 *   * #RET_FATAL if any file operations fail fatally
 */
int MtkFormatReader::bid(File &file, int best_bid)
{
    int bid = 0;
    int ret;

    if (best_bid >= static_cast<int>(
            android::BOOT_MAGIC_SIZE + 2 * MTK_MAGIC_SIZE) * 8) {
        // This is a bid we can't win, so bail out
        return RET_WARN;
    }

    // Find the Android header
    uint64_t header_offset;
    ret = android::AndroidFormatReader::find_header(
            _reader, file, android::MAX_HEADER_OFFSET, _hdr, header_offset);
    if (ret == RET_OK) {
        // Update bid to account for matched bits
        _header_offset = header_offset;
        bid += static_cast<int>(android::BOOT_MAGIC_SIZE * 8);
    } else if (ret == RET_WARN) {
        // Header not found. This can't be an Android boot image.
        return 0;
    } else {
        return ret;
    }

    uint64_t mtk_kernel_offset;
    uint64_t mtk_ramdisk_offset;
    ret = find_mtk_headers(_reader, file, _hdr,
                           _mtk_kernel_hdr, mtk_kernel_offset,
                           _mtk_ramdisk_hdr, mtk_ramdisk_offset);
    if (ret == RET_OK) {
        // Update bid to account for matched bids
        _mtk_kernel_offset = mtk_kernel_offset;
        _mtk_ramdisk_offset = mtk_ramdisk_offset;
        bid += static_cast<int>(2 * MTK_MAGIC_SIZE * 8);
    } else {
        return ret;
    }

    return bid;
}

int MtkFormatReader::read_header(File &file, Header &header)
{
    int ret;

    if (!_header_offset) {
        // A bid might not have been performed if the user forced a particular
        // format
        uint64_t header_offset;
        ret = android::AndroidFormatReader::find_header(
                _reader, file, android::MAX_HEADER_OFFSET, _hdr,
                header_offset);
        if (ret < 0) {
            return ret;
        }
        _header_offset = header_offset;
    }
    if (!_mtk_kernel_offset || !_mtk_ramdisk_offset) {
        uint64_t mtk_kernel_offset;
        uint64_t mtk_ramdisk_offset;
        ret = find_mtk_headers(_reader, file, _hdr,
                               _mtk_kernel_hdr, mtk_kernel_offset,
                               _mtk_ramdisk_hdr, mtk_ramdisk_offset);
        if (ret < 0) {
            return ret;
        }
        _mtk_kernel_offset = mtk_kernel_offset;
        _mtk_ramdisk_offset = mtk_ramdisk_offset;
    }

    // Validate that the kernel and ramdisk sizes are consistent
    if (_hdr.kernel_size != static_cast<uint64_t>(
            _mtk_kernel_hdr.size) + sizeof(MtkHeader)) {
        _reader.set_error(make_error_code(
                MtkError::MismatchedKernelSizeInHeaders));
        return RET_FAILED;
    }
    if (_hdr.ramdisk_size != static_cast<uint64_t>(
            _mtk_ramdisk_hdr.size) + sizeof(MtkHeader)) {
        _reader.set_error(make_error_code(
                MtkError::MismatchedRamdiskSizeInHeaders));
        return RET_FAILED;
    }

    ret = android::AndroidFormatReader::convert_header(_hdr, header);
    if (ret != RET_OK) {
        _reader.set_error(make_error_code(MtkError::HeaderSetFieldsFailed));
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
    pos += sizeof(android::AndroidHeader);
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

    ret = _seg.entries_add(ENTRY_TYPE_MTK_KERNEL_HEADER,
                           kernel_offset, sizeof(MtkHeader), false, _reader);
    if (ret != RET_OK) { return ret; }

    ret = _seg.entries_add(ENTRY_TYPE_KERNEL,
                           *_mtk_kernel_offset, _mtk_kernel_hdr.size, false,
                           _reader);
    if (ret != RET_OK) { return ret; }

    ret = _seg.entries_add(ENTRY_TYPE_MTK_RAMDISK_HEADER,
                           ramdisk_offset, sizeof(MtkHeader), false, _reader);
    if (ret != RET_OK) { return ret; }

    ret = _seg.entries_add(ENTRY_TYPE_RAMDISK,
                           *_mtk_ramdisk_offset, _mtk_ramdisk_hdr.size, false,
                           _reader);
    if (ret != RET_OK) { return ret; }

    if (_hdr.second_size > 0) {
        ret = _seg.entries_add(ENTRY_TYPE_SECONDBOOT,
                               second_offset, _hdr.second_size, false, _reader);
        if (ret != RET_OK) { return ret; }
    }

    if (_hdr.dt_size > 0) {
        ret = _seg.entries_add(ENTRY_TYPE_DEVICE_TREE,
                               dt_offset, _hdr.dt_size, false, _reader);
        if (ret != RET_OK) { return ret; }
    }

    return RET_OK;
}

int MtkFormatReader::read_entry(File &file, Entry &entry)
{
    return _seg.read_entry(file, entry, _reader);
}

int MtkFormatReader::go_to_entry(File &file, Entry &entry, int entry_type)
{
    return _seg.go_to_entry(file, entry, entry_type, _reader);
}

int MtkFormatReader::read_data(File &file, void *buf, size_t buf_size,
                               size_t &bytes_read)
{
    return _seg.read_data(file, buf, buf_size, bytes_read, _reader);
}

}

/*!
 * \brief Enable support for MTK boot image format
 *
 * \return
 *   * #RET_OK if the format is successfully enabled
 *   * #RET_WARN if the format is already enabled
 *   * \<= #RET_FAILED if an error occurs
 */
int Reader::enable_format_mtk()
{
    using namespace mtk;

    MB_PRIVATE(Reader);

    std::unique_ptr<FormatReader> format{new MtkFormatReader(*this)};
    return priv->register_format(std::move(format));
}


}
}
