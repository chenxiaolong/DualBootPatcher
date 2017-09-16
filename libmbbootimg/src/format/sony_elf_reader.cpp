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

#include "mbbootimg/format/sony_elf_reader_p.h"

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
#include "mbbootimg/format/sony_elf_defs.h"
#include "mbbootimg/format/sony_elf_error.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"


namespace mb
{
namespace bootimg
{
namespace sonyelf
{

SonyElfFormatReader::SonyElfFormatReader(Reader &reader)
    : FormatReader(reader)
    , _hdr()
    , _have_header()
{
}

SonyElfFormatReader::~SonyElfFormatReader() = default;

int SonyElfFormatReader::type()
{
    return FORMAT_SONY_ELF;
}

std::string SonyElfFormatReader::name()
{
    return FORMAT_NAME_SONY_ELF;
}

/*!
 * \brief Perform a bid
 *
 * \return
 *   * If \>= 0, the number of bits that conform to the Sony ELF format
 *   * #RET_WARN if this is a bid that can't be won
 *   * #RET_FAILED if any file operations fail non-fatally
 *   * #RET_FATAL if any file operations fail fatally
 */
int SonyElfFormatReader::bid(File &file, int best_bid)
{
    int bid = 0;
    int ret;

    if (best_bid >= static_cast<int>(SONY_EI_NIDENT) * 8) {
        // This is a bid we can't win, so bail out
        return RET_WARN;
    }

    // Find the Sony ELF header
    ret = find_sony_elf_header(_reader, file, _hdr);
    if (ret == RET_OK) {
        // Update bid to account for matched bits
        _have_header = true;
        bid += SONY_EI_NIDENT * 8;
    } else if (ret == RET_WARN) {
        // Header not found. This can't be a Sony ELF boot image.
        return 0;
    } else {
        return ret;
    }

    return bid;
}

int SonyElfFormatReader::read_header(File &file, Header &header)
{
    int ret;

    if (!_have_header) {
        // A bid might not have been performed if the user forced a particular
        // format
        ret = find_sony_elf_header(_reader, file, _hdr);
        if (ret < 0) {
            return ret;
        }
        _have_header = true;
    }

    header.set_supported_fields(SUPPORTED_FIELDS);

    if (!header.set_entrypoint_address(_hdr.e_entry)) {
        return RET_UNSUPPORTED;
    }

    // Calculate offsets for each section

    uint64_t pos = 0;

    // pos cannot overflow due to the nature of the operands (adding UINT32_MAX
    // a few times can't overflow a uint64_t). File length overflow is checked
    // during read.

    // Header
    pos += sizeof(Sony_Elf32_Ehdr);

    // Reset entries
    _seg.entries_clear();

    // Read program segment headers
    for (Elf32_Half i = 0; i < _hdr.e_phnum; ++i) {
        Sony_Elf32_Phdr phdr;
        size_t n;

        if (!file.seek(pos, SEEK_SET, nullptr)) {
            _reader.set_error(file.error(),
                              "Failed to seek to segment %" PRIu16
                              " at %" PRIu64 ": %s", i, pos,
                              file.error_string().c_str());
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        }

        if (!file_read_fully(file, &phdr, sizeof(phdr), n)) {
            _reader.set_error(file.error(),
                              "Failed to read segment %" PRIu16 ": %s",
                              i, file.error_string().c_str());
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        } else if (n != sizeof(phdr)) {
            _reader.set_error(make_error_code(SonyElfError::UnexpectedEndOfFile),
                              "Unexpected EOF when reading segment"
                              " header %" PRIu16, i);
            return RET_WARN;
        }

        // Account for program header
        pos += sizeof(phdr);

        // Fix byte order
        sony_elf_fix_phdr_byte_order(phdr);

        if (phdr.p_type == SONY_E_TYPE_CMDLINE
                && phdr.p_flags == SONY_E_FLAGS_CMDLINE) {
            char cmdline[512];

            if (phdr.p_memsz >= sizeof(cmdline)) {
                _reader.set_error(make_error_code(
                        SonyElfError::KernelCmdlineTooLong));
                return RET_WARN;
            }

            if (!file.seek(phdr.p_offset, SEEK_SET, nullptr)) {
                _reader.set_error(file.error(),
                                  "Failed to seek to cmdline: %s",
                                  file.error_string().c_str());
                return file.is_fatal() ? RET_FATAL : RET_FAILED;
            }

            if (!file_read_fully(file, cmdline, phdr.p_memsz, n)) {
                _reader.set_error(file.error(),
                                  "Failed to read cmdline: %s",
                                  file.error_string().c_str());
                return file.is_fatal() ? RET_FATAL : RET_FAILED;
            } else if (n != phdr.p_memsz) {
                _reader.set_error(make_error_code(SonyElfError::UnexpectedEndOfFile),
                                  "Unexpected EOF when reading cmdline");
                return RET_WARN;
            }

            cmdline[n] = '\0';

            if (!header.set_kernel_cmdline({cmdline})) {
                return RET_UNSUPPORTED;
            }
        } else if (phdr.p_type == SONY_E_TYPE_KERNEL
                && phdr.p_flags == SONY_E_FLAGS_KERNEL) {
            ret = _seg.entries_add(ENTRY_TYPE_KERNEL,
                                   phdr.p_offset, phdr.p_memsz, false, _reader);
            if (ret != RET_OK) return ret;

            if (!header.set_kernel_address(phdr.p_vaddr)) {
                return RET_UNSUPPORTED;
            }
        } else if (phdr.p_type == SONY_E_TYPE_RAMDISK
                && phdr.p_flags == SONY_E_FLAGS_RAMDISK) {
            ret = _seg.entries_add(ENTRY_TYPE_RAMDISK,
                                   phdr.p_offset, phdr.p_memsz, false, _reader);
            if (ret != RET_OK) return ret;

            if (!header.set_ramdisk_address(phdr.p_vaddr)) {
                return RET_UNSUPPORTED;
            }
        } else if (phdr.p_type == SONY_E_TYPE_IPL
                && phdr.p_flags == SONY_E_FLAGS_IPL) {
            ret = _seg.entries_add(ENTRY_TYPE_SONY_IPL,
                                   phdr.p_offset, phdr.p_memsz, false, _reader);
            if (ret != RET_OK) return ret;

            if (!header.set_sony_ipl_address(phdr.p_vaddr)) {
                return RET_UNSUPPORTED;
            }
        } else if (phdr.p_type == SONY_E_TYPE_RPM
                && phdr.p_flags == SONY_E_FLAGS_RPM) {
            ret = _seg.entries_add(ENTRY_TYPE_SONY_RPM,
                                   phdr.p_offset, phdr.p_memsz, false, _reader);
            if (ret != RET_OK) return ret;

            if (!header.set_sony_rpm_address(phdr.p_vaddr)) {
                return RET_UNSUPPORTED;
            }
        } else if (phdr.p_type == SONY_E_TYPE_APPSBL
                && phdr.p_flags == SONY_E_FLAGS_APPSBL) {
            ret = _seg.entries_add(ENTRY_TYPE_SONY_APPSBL,
                                   phdr.p_offset, phdr.p_memsz, false, _reader);
            if (ret != RET_OK) return ret;

            if (!header.set_sony_appsbl_address(phdr.p_vaddr)) {
                return RET_UNSUPPORTED;
            }
        } else if (phdr.p_type == SONY_E_TYPE_SIN) {
            // Skip SIN entry. It contains an RSA signature that we can't
            // recreate (without the private key), so there's no point in
            // dumping this segment.
            continue;
        } else {
            _reader.set_error(make_error_code(SonyElfError::InvalidTypeOrFlagsField),
                              "Invalid type (0x%08" PRIx32 ") or flags"
                              " (0x%08" PRIx32 ") field in segment"
                              " %" PRIu32, phdr.p_type, phdr.p_flags, i);
            return RET_WARN;
        }
    }

    return RET_OK;
}

int SonyElfFormatReader::read_entry(File &file, Entry &entry)
{
    return _seg.read_entry(file, entry, _reader);
}

int SonyElfFormatReader::go_to_entry(File &file, Entry &entry, int entry_type)
{
    return _seg.go_to_entry(file, entry, entry_type, _reader);
}

int SonyElfFormatReader::read_data(File &file, void *buf, size_t buf_size,
                                   size_t &bytes_read)
{
    return _seg.read_data(file, buf, buf_size, bytes_read, _reader);
}

/*!
 * \brief Find and read Sony ELF boot image header
 *
 * \note The integral fields in the header will be converted to the host's byte
 *       order.
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use File::seek() to return to a known position.
 *
 * \param[in] reader Reader for setting error messages
 * \param[in] file File handle
 * \param[out] header_out Pointer to store header
 *
 * \return
 *   * #RET_OK if the header is found
 *   * #RET_WARN if the header is not found
 *   * #RET_FAILED if any file operation fails non-fatally
 *   * #RET_FATAL if any file operation fails fatally
 */
int SonyElfFormatReader::find_sony_elf_header(Reader &reader, File &file,
                                              Sony_Elf32_Ehdr &header_out)
{
    Sony_Elf32_Ehdr header;
    size_t n;

    if (!file.seek(0, SEEK_SET, nullptr)) {
        reader.set_error(file.error(),
                         "Failed to seek to beginning: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    if (!file_read_fully(file, &header, sizeof(header), n)) {
        reader.set_error(file.error(),
                         "Failed to read header: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    } else if (n != sizeof(header)) {
        reader.set_error(make_error_code(SonyElfError::SonyElfHeaderTooSmall));
        return RET_WARN;
    }

    if (memcmp(header.e_ident, SONY_E_IDENT, SONY_EI_NIDENT) != 0) {
        reader.set_error(make_error_code(SonyElfError::InvalidElfMagic));
        return RET_WARN;
    }

    sony_elf_fix_ehdr_byte_order(header);
    header_out = header;

    return RET_OK;
}

}

/*!
 * \brief Enable support for Sony ELF boot image format
 *
 * \return
 *   * #RET_OK if the format is successfully enabled
 *   * #RET_WARN if the format is already enabled
 *   * \<= #RET_FAILED if an error occurs
 */
int Reader::enable_format_sony_elf()
{
    using namespace sonyelf;

    MB_PRIVATE(Reader);

    std::unique_ptr<FormatReader> format{new SonyElfFormatReader(*this)};
    return priv->register_format(std::move(format));
}

}
}
