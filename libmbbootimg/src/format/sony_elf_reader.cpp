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

#include "mbbootimg/format/sony_elf_reader_p.h"

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
#include "mbbootimg/format/sony_elf_defs.h"
#include "mbbootimg/format/sony_elf_error.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader_p.h"


namespace mb::bootimg::sonyelf
{

SonyElfFormatReader::SonyElfFormatReader() noexcept
    : FormatReader()
    , m_hdr()
{
}

SonyElfFormatReader::~SonyElfFormatReader() noexcept = default;

Format SonyElfFormatReader::type()
{
    return Format::SonyElf;
}

/*!
 * \brief Perform a bid
 *
 * \return
 *   * If \>= 0, the number of bits that conform to the Sony ELF format
 *   * If \< 0, the bid cannot be won
 *   * A specific error code
 */
oc::result<int> SonyElfFormatReader::open(File &file, int best_bid)
{
    int bid = 0;

    if (best_bid >= static_cast<int>(SONY_EI_NIDENT) * 8) {
        // This is a bid we can't win, so bail out
        return -1;
    }

    // Find the Sony ELF header
    auto ehdr = find_sony_elf_header(file);
    if (ehdr) {
        // Update bid to account for matched bits
        m_hdr = std::move(ehdr.value());
        bid += static_cast<int>(SONY_EI_NIDENT * 8);
    } else if (ehdr.error().category() == sony_elf_error_category()) {
        // Header not found. This can't be a Sony ELF boot image.
        return 0;
    } else {
        return ehdr.as_failure();
    }

    m_seg = SegmentReader();

    return bid;
}

oc::result<void> SonyElfFormatReader::close(File &file)
{
    (void) file;

    m_hdr = {};
    m_seg = {};

    return oc::success();
}

oc::result<Header> SonyElfFormatReader::read_header(File &file)
{
    Header header;

    header.set_supported_fields(SUPPORTED_FIELDS);
    header.set_entrypoint_address(m_hdr.e_entry);

    // Calculate offsets for each section

    uint64_t pos = 0;

    // pos cannot overflow due to the nature of the operands (adding UINT32_MAX
    // a few times can't overflow a uint64_t). File length overflow is checked
    // during read.

    // Header
    pos += sizeof(Sony_Elf32_Ehdr);

    std::vector<SegmentReaderEntry> entries;

    // Read program segment headers
    for (Elf32_Half i = 0; i < m_hdr.e_phnum; ++i) {
        Sony_Elf32_Phdr phdr;

        OUTCOME_TRYV(file.seek(static_cast<int64_t>(pos), SEEK_SET));

        OUTCOME_TRYV(file_read_exact(file, &phdr, sizeof(phdr)));

        // Account for program header
        pos += sizeof(phdr);

        // Fix byte order
        sony_elf_fix_phdr_byte_order(phdr);

        if (phdr.p_type == SONY_E_TYPE_CMDLINE
                && phdr.p_flags == SONY_E_FLAGS_CMDLINE) {
            char cmdline[512];

            if (phdr.p_memsz >= sizeof(cmdline)) {
                return SonyElfError::KernelCmdlineTooLong;
            }

            OUTCOME_TRYV(file.seek(phdr.p_offset, SEEK_SET));

            OUTCOME_TRYV(file_read_exact(file, cmdline, phdr.p_memsz));

            // cmdline section may or may not contain null bytes
            cmdline[phdr.p_memsz] = '\0';

            header.set_kernel_cmdline({cmdline});
        } else if (phdr.p_type == SONY_E_TYPE_KERNEL
                && phdr.p_flags == SONY_E_FLAGS_KERNEL) {
            entries.push_back({
                EntryType::Kernel, phdr.p_offset, phdr.p_memsz, false,
            });

            header.set_kernel_address(phdr.p_vaddr);
        } else if (phdr.p_type == SONY_E_TYPE_RAMDISK
                && phdr.p_flags == SONY_E_FLAGS_RAMDISK) {
            entries.push_back({
                EntryType::Ramdisk, phdr.p_offset, phdr.p_memsz, false,
            });

            header.set_ramdisk_address(phdr.p_vaddr);
        } else if (phdr.p_type == SONY_E_TYPE_IPL
                && phdr.p_flags == SONY_E_FLAGS_IPL) {
            entries.push_back({
                EntryType::SonyIpl, phdr.p_offset, phdr.p_memsz, false,
            });

            header.set_sony_ipl_address(phdr.p_vaddr);
        } else if (phdr.p_type == SONY_E_TYPE_RPM
                && phdr.p_flags == SONY_E_FLAGS_RPM) {
            entries.push_back({
                EntryType::SonyRpm, phdr.p_offset, phdr.p_memsz, false,
            });

            header.set_sony_rpm_address(phdr.p_vaddr);
        } else if (phdr.p_type == SONY_E_TYPE_APPSBL
                && phdr.p_flags == SONY_E_FLAGS_APPSBL) {
            entries.push_back({
                EntryType::SonyAppsbl, phdr.p_offset, phdr.p_memsz, false,
            });

            header.set_sony_appsbl_address(phdr.p_vaddr);
        } else if (phdr.p_type == SONY_E_TYPE_SIN) {
            // Skip SIN entry. It contains an RSA signature that we can't
            // recreate (without the private key), so there's no point in
            // dumping this segment.
            continue;
        } else {
            //DEBUG("Invalid type (0x%08" PRIx32 ") or flags"
            //      " (0x%08" PRIx32 ") field in segment %" PRIu32,
            //      phdr.p_type, phdr.p_flags, i);
            return SonyElfError::InvalidTypeOrFlagsField;
        }
    }

    OUTCOME_TRYV(m_seg->set_entries(std::move(entries)));

    return std::move(header);
}

oc::result<Entry> SonyElfFormatReader::read_entry(File &file)
{
    return m_seg->read_entry(file);
}

oc::result<Entry>
SonyElfFormatReader::go_to_entry(File &file, std::optional<EntryType> entry_type)
{
    return m_seg->go_to_entry(file, entry_type);
}

oc::result<size_t>
SonyElfFormatReader::read_data(File &file, void *buf, size_t buf_size)
{
    return m_seg->read_data(file, buf, buf_size);
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
 * \param file File handle
 *
 * \return
 *   * The Sony ELF ehdr if it is found
 *   * A SonyElfError if the header is not found
 *   * A specific error code if any file operation fails
 */
oc::result<Sony_Elf32_Ehdr>
SonyElfFormatReader::find_sony_elf_header(File &file)
{
    OUTCOME_TRYV(file.seek(0, SEEK_SET));

    Sony_Elf32_Ehdr ehdr;

    auto ret = file_read_exact(file, &ehdr, sizeof(ehdr));
    if (!ret) {
        if (ret.error() == FileError::UnexpectedEof) {
            return SonyElfError::SonyElfHeaderTooSmall;
        } else {
            return ret.as_failure();
        }
    }

    if (memcmp(ehdr.e_ident, SONY_E_IDENT, SONY_EI_NIDENT) != 0) {
        return SonyElfError::InvalidElfMagic;
    }

    sony_elf_fix_ehdr_byte_order(ehdr);

    return std::move(ehdr);
}

}
