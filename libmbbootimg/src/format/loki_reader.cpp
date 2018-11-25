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

#include "mbbootimg/format/loki_reader_p.h"

#include <algorithm>
#include <optional>

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
#include "mbbootimg/format/loki_error.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"


namespace mb::bootimg
{
namespace loki
{

LokiFormatReader::LokiFormatReader(Reader &reader)
    : FormatReader(reader)
    , m_hdr()
    , m_loki_hdr()
{
}

LokiFormatReader::~LokiFormatReader() = default;

int LokiFormatReader::type()
{
    return FORMAT_LOKI;
}

std::string LokiFormatReader::name()
{
    return FORMAT_NAME_LOKI;
}

/*!
 * \brief Perform a bid
 *
 * \return
 *   * If \>= 0, the number of bits that conform to the Loki format
 *   * If \< 0, the bid cannot be won
 *   * A specific error code
 */
oc::result<int> LokiFormatReader::open(File &file, int best_bid)
{
    int bid = 0;

    if (best_bid >= static_cast<int>(
            android::BOOT_MAGIC_SIZE + LOKI_MAGIC_SIZE) * 8) {
        // This is a bid we can't win, so bail out
        return -1;
    }

    // Find the Loki header
    uint64_t loki_offset;
    auto ret = find_loki_header(m_reader, file, m_loki_hdr, loki_offset);
    if (ret) {
        // Update bid to account for matched bits
        m_loki_offset = loki_offset;
        bid += static_cast<int>(LOKI_MAGIC_SIZE * 8);
    } else if (ret.error().category() == loki_error_category()) {
        // Header not found. This can't be a Loki boot image.
        return 0;
    } else {
        return ret.as_failure();
    }

    // Find the Android header
    uint64_t header_offset;
    ret = android::AndroidFormatReader::find_header(
            m_reader, file, LOKI_MAX_HEADER_OFFSET, m_hdr, header_offset);
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

    m_seg = SegmentReader();

    return bid;
}

oc::result<void> LokiFormatReader::close(File &file)
{
    (void) file;

    m_hdr = {};
    m_loki_hdr = {};
    m_header_offset = {};
    m_loki_offset = {};
    m_seg = {};

    return oc::success();
}

oc::result<void> LokiFormatReader::read_header(File &file, Header &header)
{
    uint64_t kernel_offset;
    uint64_t ramdisk_offset;
    uint64_t dt_offset = 0;
    uint32_t kernel_size;
    uint32_t ramdisk_size;

    // New-style images record the original values of changed fields in the
    // Android header
    if (m_loki_hdr.orig_kernel_size != 0
            && m_loki_hdr.orig_ramdisk_size != 0
            && m_loki_hdr.ramdisk_addr != 0) {
        OUTCOME_TRYV(read_header_new(m_reader, file, m_hdr, m_loki_hdr, header,
                                     kernel_offset, kernel_size,
                                     ramdisk_offset, ramdisk_size,
                                     dt_offset));
    } else {
        OUTCOME_TRYV(read_header_old(m_reader, file, m_hdr, m_loki_hdr, header,
                                     kernel_offset, kernel_size,
                                     ramdisk_offset, ramdisk_size));
    }

    std::vector<SegmentReaderEntry> entries;

    entries.push_back({
        ENTRY_TYPE_KERNEL, kernel_offset, kernel_size, false
    });
    entries.push_back({
        ENTRY_TYPE_RAMDISK, ramdisk_offset, ramdisk_size, false
    });
    if (m_hdr.dt_size > 0 && dt_offset != 0) {
        entries.push_back({
            ENTRY_TYPE_DEVICE_TREE, dt_offset, m_hdr.dt_size, false
        });
    }

    return m_seg->set_entries(std::move(entries));
}

oc::result<void> LokiFormatReader::read_entry(File &file, Entry &entry)
{
    return m_seg->read_entry(file, entry, m_reader);
}

oc::result<void> LokiFormatReader::go_to_entry(File &file, Entry &entry,
                                               int entry_type)
{
    return m_seg->go_to_entry(file, entry, entry_type, m_reader);
}

oc::result<size_t> LokiFormatReader::read_data(File &file, void *buf,
                                               size_t buf_size)
{
    return m_seg->read_data(file, buf, buf_size, m_reader);
}

/*!
 * \brief Find and read Loki boot image header
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
 * \param[out] header_out Pointer to store header
 * \param[out] offset_out Pointer to store header offset
 *
 * \return
 *   * Nothing if the header is found
 *   * A LokiError if the header is not found
 *   * A specified error code if any file operation fails
 */
oc::result<void> LokiFormatReader::find_loki_header(Reader &reader, File &file,
                                                    LokiHeader &header_out,
                                                    uint64_t &offset_out)
{
    LokiHeader header;

    OUTCOME_TRYV(file.seek(LOKI_MAGIC_OFFSET, SEEK_SET));

    auto ret = file_read_exact(file, &header, sizeof(header));
    if (!ret) {
        if (ret.error() == FileError::UnexpectedEof) {
            return LokiError::LokiHeaderTooSmall;
        } else {
            return ret.as_failure();
        }
    }

    if (memcmp(header.magic, LOKI_MAGIC, LOKI_MAGIC_SIZE) != 0) {
        return LokiError::InvalidLokiMagic;
    }

    loki_fix_header_byte_order(header);
    header_out = header;
    offset_out = LOKI_MAGIC_OFFSET;

    return oc::success();
}

/*!
 * \brief Find and read Loki ramdisk address
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use File::seek() to return to a known position.
 *
 * \param[in] reader Reader to set error message
 * \param[in] file File handle
 * \param[in] hdr Android header
 * \param[in] loki_hdr Loki header
 * \param[out] ramdisk_addr_out Pointer to store ramdisk address
 *
 * \return
 *   * Nothing if the ramdisk address is found
 *   * A LokiError if the ramdisk address is not found
 *   * A specific error code if any file operation fails
 */
oc::result<void>
LokiFormatReader::find_ramdisk_address(Reader &reader, File &file,
                                       const android::AndroidHeader &hdr,
                                       const LokiHeader &loki_hdr,
                                       uint32_t &ramdisk_addr_out)
{
    // If the boot image was patched with a newer version of loki, find the
    // ramdisk offset in the shell code
    uint32_t ramdisk_addr = 0;

    if (loki_hdr.ramdisk_addr != 0) {
        uint64_t offset = 0;

        auto result_cb = [&](File &file_, uint64_t offset_)
                -> oc::result<FileSearchAction> {
            (void) file_;
            offset = offset_;
            return FileSearchAction::Continue;
        };

        OUTCOME_TRYV(file_search(file, {}, {}, 0, LOKI_SHELLCODE,
                                 LOKI_SHELLCODE_SIZE - 9, 1, result_cb));

        if (offset == 0) {
            return LokiError::ShellcodeNotFound;
        }

        offset += LOKI_SHELLCODE_SIZE - 5;

        OUTCOME_TRYV(file.seek(static_cast<int64_t>(offset), SEEK_SET));

        OUTCOME_TRYV(file_read_exact(file, &ramdisk_addr, sizeof(ramdisk_addr)));

        ramdisk_addr = mb_le32toh(ramdisk_addr);
    } else {
        // Otherwise, use the default for jflte (- 0x00008000 + 0x02000000)

        if (hdr.kernel_addr > UINT32_MAX - 0x01ff8000) {
            //DEBUG("Invalid kernel address: %" PRIu32, hdr.kernel_addr);
            return LokiError::InvalidKernelAddress;
        }

        ramdisk_addr = hdr.kernel_addr + 0x01ff8000;
    }

    ramdisk_addr_out = ramdisk_addr;
    return oc::success();
}

/*!
 * \brief Find gzip ramdisk offset in old-style Loki image
 *
 * This function will search for gzip headers (`0x1f8b08`) with a flags byte of
 * `0x00` or `0x08`. It will find the first occurrence of either magic string.
 * If both are found, the one with the flags byte set to `0x08` takes precedence
 * as it indiciates that the original filename field is set. This is usually the
 * case for ramdisks packed via the `gzip` command line tool.
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use File::seek() to return to a known position.
 *
 * \param[in] reader Reader to set error message
 * \param[in] file File handle
 * \param[in] start_offset Starting offset for search
 * \param[out] gzip_offset_out Pointer to store gzip ramdisk offset
 *
 * \return
 *   * Nothing if a gzip offset is found
 *   * A LokiError if no gzip offsets are found
 *   * A specific error if any file operation fails
 */
oc::result<void>
LokiFormatReader::find_gzip_offset_old(Reader &reader, File &file,
                                       uint32_t start_offset,
                                       uint64_t &gzip_offset_out)
{
    struct SearchResult
    {
        std::optional<uint64_t> flag0_offset;
        std::optional<uint64_t> flag8_offset;
    };

    // gzip header:
    // byte 0-1 : magic bytes 0x1f, 0x8b
    // byte 2   : compression (0x08 = deflate)
    // byte 3   : flags
    // byte 4-7 : modification timestamp
    // byte 8   : compression flags
    // byte 9   : operating system

    static const unsigned char gzip_deflate_magic[] = { 0x1f, 0x8b, 0x08 };

    SearchResult result = {};

    // Find first result with flags == 0x00 and flags == 0x08
    auto result_cb = [&](File &file_, uint64_t offset)
            -> oc::result<FileSearchAction> {
        unsigned char flags;

        // Stop early if possible
        if (result.flag0_offset && result.flag8_offset) {
            return FileSearchAction::Stop;
        }

        // Save original position
        OUTCOME_TRY(orig_offset, file_.seek(0, SEEK_CUR));

        // Seek to flags byte
        OUTCOME_TRYV(file_.seek(static_cast<int64_t>(offset + 3), SEEK_SET));

        // Read next bytes for flags
        auto ret = file_read_exact(file_, &flags, sizeof(flags));
        if (!ret) {
            if (ret.error() == FileError::UnexpectedEof) {
                return FileSearchAction::Stop;
            } else {
                return ret.as_failure();
            }
        }

        if (!result.flag0_offset && flags == 0x00) {
            result.flag0_offset = offset;
        } else if (!result.flag8_offset && flags == 0x08) {
            result.flag8_offset = offset;
        }

        // Restore original position as per contract
        OUTCOME_TRYV(file_.seek(static_cast<int64_t>(orig_offset), SEEK_SET));

        return FileSearchAction::Continue;
    };

    OUTCOME_TRYV(file_search(file, start_offset, {}, 0, gzip_deflate_magic,
                             sizeof(gzip_deflate_magic), {}, result_cb));

    // Prefer gzip header with original filename flag since most loki'd boot
    // images will have been compressed manually with the gzip tool
    if (result.flag8_offset) {
        gzip_offset_out = *result.flag8_offset;
    } else if (result.flag0_offset) {
        gzip_offset_out = *result.flag0_offset;
    } else {
        return LokiError::NoRamdiskGzipHeaderFound;
    }

    return oc::success();
}

/*!
 * \brief Find ramdisk size in old-style Loki image
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use File::seek() to return to a known position.
 *
 * \param[in] reader Reader to set error message
 * \param[in] file File handle
 * \param[in] hdr Android header
 * \param[in] ramdisk_offset Offset of ramdisk in image
 * \param[out] ramdisk_size_out Pointer to store ramdisk size
 *
 * \return
 *   * Nothing if the ramdisk size is found
 *   * A LokiError if the ramdisk size is not found
 *   * A specific error if any file operation fails
 */
oc::result<void>
LokiFormatReader::find_ramdisk_size_old(Reader &reader, File &file,
                                        const android::AndroidHeader &hdr,
                                        uint32_t ramdisk_offset,
                                        uint32_t &ramdisk_size_out)
{
    int32_t aboot_size;

    // If the boot image was patched with an old version of loki, the ramdisk
    // size is not stored properly. We'll need to guess the size of the archive.

    // The ramdisk is supposed to be from the gzip header to EOF, but loki needs
    // to store a copy of aboot, so it is put in the last 0x200 bytes of the
    // file.
    if (is_lg_ramdisk_address(hdr.ramdisk_addr)) {
        aboot_size = static_cast<int32_t>(hdr.page_size);
    } else {
        aboot_size = 0x200;
    }

    OUTCOME_TRY(aboot_offset, file.seek(-aboot_size, SEEK_END));

    if (ramdisk_offset > aboot_offset) {
        return LokiError::RamdiskOffsetGreaterThanAbootOffset;
    }

    // Ignore zero padding as we might strip away too much
#if 1
    ramdisk_size_out = static_cast<uint32_t>(aboot_offset - ramdisk_offset);
    return oc::success();
#else
    char buf[1024];

    // Search backwards to find non-zero byte
    uint64_t cur_offset = aboot_offset;

    while (cur_offset > ramdisk_offset) {
        size_t to_read = std::min<uint64_t>(
                sizeof(buf), cur_offset - ramdisk_offset);
        cur_offset -= to_read;

        OUTCOME_TRYV(file.seek(cur_offset, SEEK_SET));

        OUTCOME_TRYV(file_read_exact(file, buf, to_read));

        for (size_t i = to_read; i-- > 0; ) {
            if (buf[i] != '\0') {
                ramdisk_size_out = cur_offset - ramdisk_offset + i;
                return oc::success();
            }
        }
    }

    return LokiError::FailedToDetermineRamdiskSize;
#endif
}

/*!
 * \brief Find size of Linux kernel in boot image
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use File::seek() to return to a known position.
 *
 * \param[in] reader Reader to set error message
 * \param[in] file File handle
 * \param[in] kernel_offset Offset of kernel in boot image
 * \param[out] kernel_size_out Pointer to store kernel size
 *
 * \return
 *   * Nothing if the kernel size is found
 *   * FileError::UnexpectedEof if the kernel size cannot be found
 *   * A specific error if any file operation fails
 */
oc::result<void>
LokiFormatReader::find_linux_kernel_size(Reader &reader,File &file,
                                         uint32_t kernel_offset,
                                         uint32_t &kernel_size_out)
{
    uint32_t kernel_size;

    // If the boot image was patched with an early version of loki, the
    // original kernel size is not stored in the loki header properly (or in the
    // shellcode). The size is stored in the kernel image's header though, so
    // we'll use that.
    // http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html#d0e309
    OUTCOME_TRYV(file.seek(kernel_offset + 0x2c, SEEK_SET));

    OUTCOME_TRYV(file_read_exact(file, &kernel_size, sizeof(kernel_size)));

    kernel_size_out = mb_le32toh(kernel_size);
    return oc::success();
}

/*!
 * \brief Read header for old-style Loki image
 *
 * \param[in] reader Reader to set error message
 * \param[in] file File handle
 * \param[in] hdr Android header for image
 * \param[in] loki_hdr Loki header for image
 * \param[out] header Header instance to store header values
 * \param[out] kernel_offset_out Pointer to store kernel offset
 * \param[out] kernel_size_out Pointer to store kernel size
 * \param[out] ramdisk_offset_out Pointer to store ramdisk offset
 * \param[out] ramdisk_size_out Pointer to store ramdisk size
 *
 * \return Nothing if the header is successfully read. Otherwise, a specific
 *         error code.
 */
oc::result<void>
LokiFormatReader::read_header_old(Reader &reader, File &file,
                                  const android::AndroidHeader &hdr,
                                  const LokiHeader &loki_hdr,
                                  Header &header,
                                  uint64_t &kernel_offset_out,
                                  uint32_t &kernel_size_out,
                                  uint64_t &ramdisk_offset_out,
                                  uint32_t &ramdisk_size_out)
{
    uint32_t tags_addr;
    uint32_t kernel_size;
    uint32_t ramdisk_size;
    uint32_t ramdisk_addr;
    uint64_t gzip_offset;

    if (hdr.page_size == 0) {
        return LokiError::PageSizeCannotBeZero;
    }

    // The kernel tags address is invalid in the old loki images, so use the
    // default for jflte
    tags_addr = hdr.kernel_addr - android::DEFAULT_KERNEL_OFFSET
            + android::DEFAULT_TAGS_OFFSET;

    // Try to guess kernel size
    OUTCOME_TRYV(find_linux_kernel_size(reader, file, hdr.page_size,
                                        kernel_size));

    // Look for gzip offset for the ramdisk
    OUTCOME_TRYV(find_gzip_offset_old(
            reader, file, hdr.page_size + kernel_size
                    + align_page_size<uint32_t>(kernel_size, hdr.page_size),
            gzip_offset));

    // Try to guess ramdisk size
    OUTCOME_TRYV(find_ramdisk_size_old(reader, file, hdr,
                                       static_cast<uint32_t>(gzip_offset),
                                       ramdisk_size));

    // Guess original ramdisk address
    OUTCOME_TRYV(find_ramdisk_address(reader, file, hdr, loki_hdr,
                                      ramdisk_addr));

    kernel_size_out = kernel_size;
    ramdisk_size_out = ramdisk_size;

    auto *name_ptr = reinterpret_cast<const char *>(hdr.name);
    auto name_size = strnlen(name_ptr, sizeof(hdr.name));

    auto *cmdline_ptr = reinterpret_cast<const char *>(hdr.cmdline);
    auto cmdline_size = strnlen(cmdline_ptr, sizeof(hdr.cmdline));

    header.set_supported_fields(OLD_SUPPORTED_FIELDS);
    header.set_board_name({{name_ptr, name_size}});
    header.set_kernel_cmdline({{cmdline_ptr, cmdline_size}});
    header.set_page_size(hdr.page_size);
    header.set_kernel_address(hdr.kernel_addr);
    header.set_ramdisk_address(ramdisk_addr);
    header.set_secondboot_address(hdr.second_addr);
    header.set_kernel_tags_address(tags_addr);

    uint64_t pos = 0;

    // pos cannot overflow due to the nature of the operands (adding UINT32_MAX
    // a few times can't overflow a uint64_t). File length overflow is checked
    // during read.

    // Header
    pos += hdr.page_size;

    // Kernel
    kernel_offset_out = pos;
    pos += kernel_size;
    pos += align_page_size<uint64_t>(pos, hdr.page_size);

    // Ramdisk
    ramdisk_offset_out = pos = gzip_offset;
    pos += ramdisk_size;
    pos += align_page_size<uint64_t>(pos, hdr.page_size);

    return oc::success();
}

/*!
 * \brief Read header for new-style Loki image
 *
 * \param[in] reader Reader to set error message
 * \param[in] file File handle
 * \param[in] hdr Android header for image
 * \param[in] loki_hdr Loki header for image
 * \param[out] header Header instance to store header values
 * \param[out] kernel_offset_out Pointer to store kernel offset
 * \param[out] kernel_size_out Pointer to store kernel size
 * \param[out] ramdisk_offset_out Pointer to store ramdisk offset
 * \param[out] ramdisk_size_out Pointer to store ramdisk size
 * \param[out] dt_offset_out Pointer to store device tree offset
 *
 * \return Nothing if the header is successfully read. Otherwise, a specific
 *         error code.
 */
oc::result<void>
LokiFormatReader::read_header_new(Reader &reader, File &file,
                                  const android::AndroidHeader &hdr,
                                  const LokiHeader &loki_hdr,
                                  Header &header,
                                  uint64_t &kernel_offset_out,
                                  uint32_t &kernel_size_out,
                                  uint64_t &ramdisk_offset_out,
                                  uint32_t &ramdisk_size_out,
                                  uint64_t &dt_offset_out)
{
    uint32_t fake_size;
    uint32_t ramdisk_addr;

    if (hdr.page_size == 0) {
        return LokiError::PageSizeCannotBeZero;
    }

    if (is_lg_ramdisk_address(hdr.ramdisk_addr)) {
        fake_size = hdr.page_size;
    } else {
        fake_size = 0x200;
    }

    // Find original ramdisk address
    OUTCOME_TRYV(find_ramdisk_address(reader, file, hdr, loki_hdr,
                                      ramdisk_addr));

    // Restore original values in boot image header
    kernel_size_out = loki_hdr.orig_kernel_size;
    ramdisk_size_out = loki_hdr.orig_ramdisk_size;

    auto *name_ptr = reinterpret_cast<const char *>(hdr.name);
    auto name_size = strnlen(name_ptr, sizeof(hdr.name));

    auto *cmdline_ptr = reinterpret_cast<const char *>(hdr.cmdline);
    auto cmdline_size = strnlen(cmdline_ptr, sizeof(hdr.cmdline));

    header.set_supported_fields(NEW_SUPPORTED_FIELDS);
    header.set_board_name({{name_ptr, name_size}});
    header.set_kernel_cmdline({{cmdline_ptr, cmdline_size}});
    header.set_page_size(hdr.page_size);
    header.set_kernel_address(hdr.kernel_addr);
    header.set_ramdisk_address(ramdisk_addr);
    header.set_secondboot_address(hdr.second_addr);
    header.set_kernel_tags_address(hdr.tags_addr);

    uint64_t pos = 0;

    // pos cannot overflow due to the nature of the operands (adding UINT32_MAX
    // a few times can't overflow a uint64_t). File length overflow is checked
    // during read.

    // Header
    pos += hdr.page_size;

    // Kernel
    kernel_offset_out = pos;
    pos += loki_hdr.orig_kernel_size;
    pos += align_page_size<uint64_t>(pos, hdr.page_size);

    // Ramdisk
    ramdisk_offset_out = pos;
    pos += loki_hdr.orig_ramdisk_size;
    pos += align_page_size<uint64_t>(pos, hdr.page_size);

    // Device tree
    if (hdr.dt_size != 0) {
        pos += fake_size;
    }
    dt_offset_out = pos;
    pos += hdr.dt_size;
    pos += align_page_size<uint64_t>(pos, hdr.page_size);

    return oc::success();
}

}

/*!
 * \brief Enable support for Loki boot image format
 *
 * \return Nothing if the format is successfully enabled. Otherwise, the error
 *         code.
 */
oc::result<void> Reader::enable_format_loki()
{
    return register_format(std::make_unique<loki::LokiFormatReader>(*this));
}

}
