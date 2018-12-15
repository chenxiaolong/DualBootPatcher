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

#include "mbbootimg/format/loki_reader_p.h"

#include <algorithm>
#include <optional>

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
#include "mbbootimg/format/loki_error.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader_p.h"


namespace mb::bootimg::loki
{

LokiFormatReader::LokiFormatReader() noexcept
    : FormatReader()
    , m_ahdr()
    , m_lhdr()
{
}

LokiFormatReader::~LokiFormatReader() noexcept = default;

Format LokiFormatReader::type()
{
    return Format::Loki;
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
    auto lhdr = find_loki_header(file);
    if (lhdr) {
        // Update bid to account for matched bits
        m_lhdr = std::move(lhdr.value().first);
        m_lhdr_offset = lhdr.value().second;
        bid += static_cast<int>(LOKI_MAGIC_SIZE * 8);
    } else if (lhdr.error().category() == loki_error_category()) {
        // Header not found. This can't be a Loki boot image.
        return 0;
    } else {
        return lhdr.as_failure();
    }

    // Find the Android header
    auto ahdr = android::AndroidFormatReader::find_header(
            file, LOKI_MAX_HEADER_OFFSET);
    if (ahdr) {
        // Update bid to account for matched bits
        m_ahdr = std::move(ahdr.value().first);
        m_ahdr_offset = ahdr.value().second;
        bid += static_cast<int>(android::BOOT_MAGIC_SIZE * 8);
    } else if (ahdr.error() == android::AndroidError::HeaderNotFound
            || ahdr.error() == android::AndroidError::HeaderOutOfBounds) {
        // Header not found. This can't be an Android boot image.
        return 0;
    } else {
        return ahdr.as_failure();
    }

    m_seg = SegmentReader();

    return bid;
}

oc::result<void> LokiFormatReader::close(File &file)
{
    (void) file;

    m_ahdr = {};
    m_lhdr = {};
    m_ahdr_offset = {};
    m_lhdr_offset = {};
    m_seg = {};

    return oc::success();
}

oc::result<Header> LokiFormatReader::read_header(File &file)
{
    // New-style images record the original values of changed fields in the
    // Android header
    auto read_header_func =
            (m_lhdr.orig_kernel_size != 0
                    && m_lhdr.orig_ramdisk_size != 0
                    && m_lhdr.ramdisk_addr != 0)
            ? &read_header_new : &read_header_old;

    OUTCOME_TRY(result, read_header_func(file, m_ahdr, m_lhdr));

    std::vector<SegmentReaderEntry> entries;

    entries.push_back({
        EntryType::Kernel, result.kernel_offset, result.kernel_size, false,
    });
    entries.push_back({
        EntryType::Ramdisk, result.ramdisk_offset, result.ramdisk_size, false,
    });
    if (m_ahdr.dt_size > 0 && result.dt_offset != 0) {
        entries.push_back({
            EntryType::DeviceTree, result.dt_offset, m_ahdr.dt_size, false,
        });
    }

    OUTCOME_TRYV(m_seg->set_entries(std::move(entries)));

    return std::move(result.header);
}

oc::result<Entry> LokiFormatReader::read_entry(File &file)
{
    return m_seg->read_entry(file);
}

oc::result<Entry>
LokiFormatReader::go_to_entry(File &file, std::optional<EntryType> entry_type)
{
    return m_seg->go_to_entry(file, entry_type);
}

oc::result<size_t>
LokiFormatReader::read_data(File &file, void *buf, size_t buf_size)
{
    return m_seg->read_data(file, buf, buf_size);
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
 * \param file File handle
 *
 * \return
 *   * If the header is found, the header and its offset
 *   * A LokiError if the header is not found
 *   * A specified error code if any file operation fails
 */
oc::result<std::pair<LokiHeader, uint64_t>>
LokiFormatReader::find_loki_header(File &file)
{
    OUTCOME_TRYV(file.seek(LOKI_MAGIC_OFFSET, SEEK_SET));

    LokiHeader header;

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

    return {std::move(header), LOKI_MAGIC_OFFSET};
}

/*!
 * \brief Find and read Loki ramdisk address
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use File::seek() to return to a known position.
 *
 * \param file File handle
 * \param ahdr Android header
 * \param lhdr Loki header
 *
 * \return
 *   * The ramdisk address if it is found
 *   * A LokiError if the ramdisk address is not found
 *   * A specific error code if any file operation fails
 */
oc::result<uint32_t>
LokiFormatReader::find_ramdisk_address(File &file,
                                       const android::AndroidHeader &ahdr,
                                       const LokiHeader &lhdr)
{
    // If the boot image was patched with a newer version of loki, find the
    // ramdisk offset in the shell code
    uint32_t ramdisk_addr;

    if (lhdr.ramdisk_addr != 0) {
        OUTCOME_TRYV(file.seek(0, SEEK_SET));

        FileSearcher searcher(&file, LOKI_SHELLCODE, LOKI_SHELLCODE_SIZE - 9);

        OUTCOME_TRY(offset, searcher.next());
        if (!offset) {
            return LokiError::ShellcodeNotFound;
        }

        *offset += LOKI_SHELLCODE_SIZE - 5;

        OUTCOME_TRYV(file.seek(static_cast<int64_t>(*offset), SEEK_SET));

        OUTCOME_TRYV(file_read_exact(file, &ramdisk_addr, sizeof(ramdisk_addr)));

        ramdisk_addr = mb_le32toh(ramdisk_addr);
    } else {
        // Otherwise, use the default for jflte (- 0x00008000 + 0x02000000)

        if (ahdr.kernel_addr > UINT32_MAX - 0x01ff8000) {
            //DEBUG("Invalid kernel address: %" PRIu32, ahdr.kernel_addr);
            return LokiError::InvalidKernelAddress;
        }

        ramdisk_addr = ahdr.kernel_addr + 0x01ff8000;
    }

    return ramdisk_addr;
}

/*!
 * \brief Find gzip ramdisk offset in old-style Loki image
 *
 * This function will search for gzip headers (`0x1f8b08`) with a flags byte of
 * `0x00` or `0x08`. It will find the first occurrence of either magic string.
 * If both are found, the one with the flags byte set to `0x08` takes precedence
 * as it indicates that the original filename field is set. This is usually the
 * case for ramdisks packed via the `gzip` command line tool.
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use File::seek() to return to a known position.
 *
 * \param file File handle
 * \param start_offset Starting offset for search
 *
 * \return
 *   * The ramdisk gzip offset if it is found
 *   * A LokiError if no gzip offsets are found
 *   * A specific error if any file operation fails
 */
oc::result<uint64_t>
LokiFormatReader::find_gzip_offset_old(File &file, uint32_t start_offset)
{
    // gzip header:
    // byte 0-1 : magic bytes 0x1f, 0x8b
    // byte 2   : compression (0x08 = deflate)
    // byte 3   : flags
    // byte 4-7 : modification timestamp
    // byte 8   : compression flags
    // byte 9   : operating system

    static constexpr unsigned char gzip_deflate_magic[] = { 0x1f, 0x8b, 0x08 };

    OUTCOME_TRYV(file.seek(static_cast<int64_t>(start_offset), SEEK_SET));

    FileSearcher searcher(&file, gzip_deflate_magic,
                          sizeof(gzip_deflate_magic));
    std::optional<uint64_t> flag0_offset;
    std::optional<uint64_t> flag8_offset;

    // Find first result with flags == 0x00 and flags == 0x08
    while (true) {
        OUTCOME_TRY(offset, searcher.next());
        if (!offset) {
            break;
        }

        // Offset is relative to starting position
        *offset += start_offset;

        // Save original position
        OUTCOME_TRY(orig_offset, file.seek(0, SEEK_CUR));

        // Seek to flags byte
        OUTCOME_TRYV(file.seek(static_cast<int64_t>(*offset + 3), SEEK_SET));

        // Read next bytes for flags
        unsigned char flags;
        if (auto r = file_read_exact(file, &flags, sizeof(flags)); !r) {
            if (r.error() == FileError::UnexpectedEof) {
                break;
            } else {
                return r.as_failure();
            }
        }

        if (!flag0_offset && flags == 0x00) {
            flag0_offset = *offset;
        } else if (!flag8_offset && flags == 0x08) {
            flag8_offset = *offset;
        }

        // Restore original position as per contract
        OUTCOME_TRYV(file.seek(static_cast<int64_t>(orig_offset), SEEK_SET));

        // Stop early if possible
        if (flag0_offset && flag8_offset) {
            break;
        }
    }

    // Prefer gzip header with original filename flag since most loki'd boot
    // images will have been compressed manually with the gzip tool
    if (flag8_offset) {
        return *flag8_offset;
    } else if (flag0_offset) {
        return *flag0_offset;
    } else {
        return LokiError::NoRamdiskGzipHeaderFound;
    }
}

/*!
 * \brief Find ramdisk size in old-style Loki image
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use File::seek() to return to a known position.
 *
 * \param file File handle
 * \param ahdr Android header
 * \param ramdisk_offset Offset of ramdisk in image
 *
 * \return
 *   * The estimated ramdisk size if it is found
 *   * A LokiError if the ramdisk size is not found
 *   * A specific error if any file operation fails
 */
oc::result<uint32_t>
LokiFormatReader::find_ramdisk_size_old(File &file,
                                        const android::AndroidHeader &ahdr,
                                        uint32_t ramdisk_offset)
{
    // If the boot image was patched with an old version of loki, the ramdisk
    // size is not stored properly. We'll need to guess the size of the archive.

    // The ramdisk is supposed to be from the gzip header to EOF, but loki needs
    // to store a copy of aboot, so it is put in the last 0x200 bytes of the
    // file.

    int32_t aboot_size;

    if (is_lg_ramdisk_address(ahdr.ramdisk_addr)) {
        aboot_size = static_cast<int32_t>(ahdr.page_size);
    } else {
        aboot_size = 0x200;
    }

    OUTCOME_TRY(aboot_offset, file.seek(-aboot_size, SEEK_END));

    if (ramdisk_offset > aboot_offset) {
        return LokiError::RamdiskOffsetGreaterThanAbootOffset;
    }

    // Ignore zero padding as we might strip away too much
#if 1
    return static_cast<uint32_t>(aboot_offset - ramdisk_offset);
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
                return cur_offset - ramdisk_offset + i;
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
 * \param file File handle
 * \param kernel_offset Offset of kernel in boot image
 *
 * \return
 *   * The kernel size if it is found
 *   * FileError::UnexpectedEof if the kernel size cannot be found
 *   * A specific error if any file operation fails
 */
oc::result<uint32_t>
LokiFormatReader::find_linux_kernel_size(File &file, uint32_t kernel_offset)
{
    // If the boot image was patched with an early version of loki, the
    // original kernel size is not stored in the loki header properly (or in the
    // shellcode). The size is stored in the kernel image's header though, so
    // we'll use that.
    // http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html#d0e309
    OUTCOME_TRYV(file.seek(kernel_offset + 0x2c, SEEK_SET));

    uint32_t kernel_size;
    OUTCOME_TRYV(file_read_exact(file, &kernel_size, sizeof(kernel_size)));

    return mb_le32toh(kernel_size);
}

/*!
 * \brief Read header for old-style Loki image
 *
 * \param file File handle
 * \param ahdr Android header for image
 * \param lhdr Loki header for image
 *
 * \return ReadHeaderResult containing header fields if the header from an
 *         old-style Loki image is successfully read. Otherwise, a specific
 *         error code.
 */
oc::result<ReadHeaderResult>
LokiFormatReader::read_header_old(File &file,
                                  const android::AndroidHeader &ahdr,
                                  const LokiHeader &lhdr)
{
    if (ahdr.page_size == 0) {
        return LokiError::PageSizeCannotBeZero;
    }

    // The kernel tags address is invalid in the old loki images, so use the
    // default for jflte
    uint32_t tags_addr = ahdr.kernel_addr - android::DEFAULT_KERNEL_OFFSET
            + android::DEFAULT_TAGS_OFFSET;

    // Try to guess kernel size
    OUTCOME_TRY(kernel_size, find_linux_kernel_size(file, ahdr.page_size));

    // Look for gzip offset for the ramdisk
    OUTCOME_TRY(gzip_offset, find_gzip_offset_old(
            file, ahdr.page_size + kernel_size
                    + align_page_size<uint32_t>(kernel_size, ahdr.page_size)));

    // Try to guess ramdisk size
    OUTCOME_TRY(ramdisk_size, find_ramdisk_size_old(
            file, ahdr, static_cast<uint32_t>(gzip_offset)));

    // Guess original ramdisk address
    OUTCOME_TRY(ramdisk_addr, find_ramdisk_address(file, ahdr, lhdr));

    Header header;

    auto *name_ptr = reinterpret_cast<const char *>(ahdr.name);
    auto name_size = strnlen(name_ptr, sizeof(ahdr.name));

    auto *cmdline_ptr = reinterpret_cast<const char *>(ahdr.cmdline);
    auto cmdline_size = strnlen(cmdline_ptr, sizeof(ahdr.cmdline));

    header.set_supported_fields(OLD_SUPPORTED_FIELDS);
    header.set_board_name({{name_ptr, name_size}});
    header.set_kernel_cmdline({{cmdline_ptr, cmdline_size}});
    header.set_page_size(ahdr.page_size);
    header.set_kernel_address(ahdr.kernel_addr);
    header.set_ramdisk_address(ramdisk_addr);
    header.set_secondboot_address(ahdr.second_addr);
    header.set_kernel_tags_address(tags_addr);

    return ReadHeaderResult{
        std::move(header),
        ahdr.page_size,
        gzip_offset,
        0,
        kernel_size,
        ramdisk_size,
    };
}

/*!
 * \brief Read header for new-style Loki image
 *
 * \param file File handle
 * \param ahdr Android header for image
 * \param lhdr Loki header for image
 *
 * \return ReadHeaderResult containing header fields if the header from an
 *         new-style Loki image is successfully read. Otherwise, a specific
 *         error code.
 */
oc::result<ReadHeaderResult>
LokiFormatReader::read_header_new(File &file,
                                  const android::AndroidHeader &ahdr,
                                  const LokiHeader &lhdr)
{
    if (ahdr.page_size == 0) {
        return LokiError::PageSizeCannotBeZero;
    }

    uint32_t fake_size;

    if (is_lg_ramdisk_address(ahdr.ramdisk_addr)) {
        fake_size = ahdr.page_size;
    } else {
        fake_size = 0x200;
    }

    // Find original ramdisk address
    OUTCOME_TRY(ramdisk_addr, find_ramdisk_address(file, ahdr, lhdr));

    Header header;

    auto *name_ptr = reinterpret_cast<const char *>(ahdr.name);
    auto name_size = strnlen(name_ptr, sizeof(ahdr.name));

    auto *cmdline_ptr = reinterpret_cast<const char *>(ahdr.cmdline);
    auto cmdline_size = strnlen(cmdline_ptr, sizeof(ahdr.cmdline));

    header.set_supported_fields(NEW_SUPPORTED_FIELDS);
    header.set_board_name({{name_ptr, name_size}});
    header.set_kernel_cmdline({{cmdline_ptr, cmdline_size}});
    header.set_page_size(ahdr.page_size);
    header.set_kernel_address(ahdr.kernel_addr);
    header.set_ramdisk_address(ramdisk_addr);
    header.set_secondboot_address(ahdr.second_addr);
    header.set_kernel_tags_address(ahdr.tags_addr);

    // Calculate offsets for each section

    uint64_t pos = 0;

    // pos cannot overflow due to the nature of the operands (adding UINT32_MAX
    // a few times can't overflow a uint64_t). File length overflow is checked
    // during read.

    // Header
    pos += ahdr.page_size;

    // Kernel
    auto kernel_offset = pos;
    pos += lhdr.orig_kernel_size;
    pos += align_page_size<uint64_t>(pos, ahdr.page_size);

    // Ramdisk
    auto ramdisk_offset = pos;
    pos += lhdr.orig_ramdisk_size;
    pos += align_page_size<uint64_t>(pos, ahdr.page_size);

    // Device tree
    if (ahdr.dt_size != 0) {
        pos += fake_size;
    }
    auto dt_offset = pos;

    return ReadHeaderResult{
        std::move(header),
        kernel_offset,
        ramdisk_offset,
        dt_offset,
        lhdr.orig_kernel_size,
        lhdr.orig_ramdisk_size,
    };
}

}
