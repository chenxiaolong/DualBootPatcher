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
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"


namespace mb
{
namespace bootimg
{
namespace sonyelf
{

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
 * \param[in] bir MbBiReader for setting error messages
 * \param[in] file File handle
 * \param[out] header_out Pointer to store header
 *
 * \return
 *   * #RET_OK if the header is found
 *   * #RET_WARN if the header is not found
 *   * #RET_FAILED if any file operation fails non-fatally
 *   * #RET_FATAL if any file operation fails fatally
 */
int find_sony_elf_header(MbBiReader *bir, File &file,
                         Sony_Elf32_Ehdr &header_out)
{
    Sony_Elf32_Ehdr header;
    size_t n;

    if (!file.seek(0, SEEK_SET, nullptr)) {
        mb_bi_reader_set_error(bir, file.error().value() /* TODO */,
                               "Failed to seek to beginning: %s",
                               file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    if (!file_read_fully(file, &header, sizeof(header), n)) {
        mb_bi_reader_set_error(bir, file.error().value() /* TODO */,
                               "Failed to read header: %s",
                               file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    } else if (n != sizeof(header)) {
        mb_bi_reader_set_error(bir, ERROR_FILE_FORMAT,
                               "Too small to be Sony ELF image");
        return RET_WARN;
    }

    if (memcmp(header.e_ident, SONY_E_IDENT, SONY_EI_NIDENT) != 0) {
        mb_bi_reader_set_error(bir, ERROR_FILE_FORMAT,
                               "Invalid ELF magic");
        return RET_WARN;
    }

    sony_elf_fix_ehdr_byte_order(header);
    header_out = header;

    return RET_OK;
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
int sony_elf_reader_bid(MbBiReader *bir, void *userdata, int best_bid)
{
    SonyElfReaderCtx *const ctx = static_cast<SonyElfReaderCtx *>(userdata);
    int bid = 0;
    int ret;

    if (best_bid >= static_cast<int>(SONY_EI_NIDENT) * 8) {
        // This is a bid we can't win, so bail out
        return RET_WARN;
    }

    // Find the Sony ELF header
    ret = find_sony_elf_header(bir, *bir->file, ctx->hdr);
    if (ret == RET_OK) {
        // Update bid to account for matched bits
        ctx->have_header = true;
        bid += SONY_EI_NIDENT * 8;
    } else if (ret == RET_WARN) {
        // Header not found. This can't be a Sony ELF boot image.
        return 0;
    } else {
        return ret;
    }

    return bid;
}

int sony_elf_reader_read_header(MbBiReader *bir, void *userdata, Header &header)
{
    SonyElfReaderCtx *const ctx = static_cast<SonyElfReaderCtx *>(userdata);
    int ret;

    if (!ctx->have_header) {
        // A bid might not have been performed if the user forced a particular
        // format
        ret = find_sony_elf_header(bir, *bir->file, ctx->hdr);
        if (ret < 0) {
            return ret;
        }
        ctx->have_header = true;
    }

    header.set_supported_fields(SUPPORTED_FIELDS);

    if (!header.set_entrypoint_address(ctx->hdr.e_entry)) {
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
    ctx->seg.entries_clear();

    // Read program segment headers
    for (Elf32_Half i = 0; i < ctx->hdr.e_phnum; ++i) {
        Sony_Elf32_Phdr phdr;
        size_t n;

        if (!bir->file->seek(pos, SEEK_SET, nullptr)) {
            mb_bi_reader_set_error(bir, bir->file->error().value() /* TODO */,
                                   "Failed to seek to segment %" PRIu16
                                   " at %" PRIu64 ": %s", i, pos,
                                   bir->file->error_string().c_str());
            return bir->file->is_fatal() ? RET_FATAL : RET_FAILED;
        }

        if (!file_read_fully(*bir->file, &phdr, sizeof(phdr), n)) {
            mb_bi_reader_set_error(bir, bir->file->error().value() /* TODO */,
                                   "Failed to read segment %" PRIu16 ": %s",
                                   i, bir->file->error_string().c_str());
            return bir->file->is_fatal() ? RET_FATAL : RET_FAILED;
        } else if (n != sizeof(phdr)) {
            mb_bi_reader_set_error(bir, ERROR_FILE_FORMAT,
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
                mb_bi_reader_set_error(bir, ERROR_FILE_FORMAT,
                                       "cmdline too long");
                return RET_WARN;
            }

            if (!bir->file->seek(phdr.p_offset, SEEK_SET, nullptr)) {
                mb_bi_reader_set_error(bir, bir->file->error().value() /* TODO */,
                                       "Failed to seek to cmdline: %s",
                                       bir->file->error_string().c_str());
                return bir->file->is_fatal() ? RET_FATAL : RET_FAILED;
            }

            if (!file_read_fully(*bir->file, cmdline, phdr.p_memsz, n)) {
                mb_bi_reader_set_error(bir, bir->file->error().value() /* TODO */,
                                       "Failed to read cmdline: %s",
                                       bir->file->error_string().c_str());
                return bir->file->is_fatal() ? RET_FATAL : RET_FAILED;
            } else if (n != phdr.p_memsz) {
                mb_bi_reader_set_error(bir, ERROR_FILE_FORMAT,
                                       "Unexpected EOF when reading cmdline");
                return RET_WARN;
            }

            cmdline[n] = '\0';

            if (!header.set_kernel_cmdline({cmdline})) {
                return RET_UNSUPPORTED;
            }
        } else if (phdr.p_type == SONY_E_TYPE_KERNEL
                && phdr.p_flags == SONY_E_FLAGS_KERNEL) {
            ret = ctx->seg.entries_add(ENTRY_TYPE_KERNEL,
                                       phdr.p_offset, phdr.p_memsz, false, bir);
            if (ret != RET_OK) return ret;

            if (!header.set_kernel_address(phdr.p_vaddr)) {
                return RET_UNSUPPORTED;
            }
        } else if (phdr.p_type == SONY_E_TYPE_RAMDISK
                && phdr.p_flags == SONY_E_FLAGS_RAMDISK) {
            ret = ctx->seg.entries_add(ENTRY_TYPE_RAMDISK,
                                       phdr.p_offset, phdr.p_memsz, false, bir);
            if (ret != RET_OK) return ret;

            if (!header.set_ramdisk_address(phdr.p_vaddr)) {
                return RET_UNSUPPORTED;
            }
        } else if (phdr.p_type == SONY_E_TYPE_IPL
                && phdr.p_flags == SONY_E_FLAGS_IPL) {
            ret = ctx->seg.entries_add(ENTRY_TYPE_SONY_IPL,
                                       phdr.p_offset, phdr.p_memsz, false, bir);
            if (ret != RET_OK) return ret;

            if (!header.set_sony_ipl_address(phdr.p_vaddr)) {
                return RET_UNSUPPORTED;
            }
        } else if (phdr.p_type == SONY_E_TYPE_RPM
                && phdr.p_flags == SONY_E_FLAGS_RPM) {
            ret = ctx->seg.entries_add(ENTRY_TYPE_SONY_RPM,
                                       phdr.p_offset, phdr.p_memsz, false, bir);
            if (ret != RET_OK) return ret;

            if (!header.set_sony_rpm_address(phdr.p_vaddr)) {
                return RET_UNSUPPORTED;
            }
        } else if (phdr.p_type == SONY_E_TYPE_APPSBL
                && phdr.p_flags == SONY_E_FLAGS_APPSBL) {
            ret = ctx->seg.entries_add(ENTRY_TYPE_SONY_APPSBL,
                                       phdr.p_offset, phdr.p_memsz, false, bir);
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
            mb_bi_reader_set_error(bir, ERROR_FILE_FORMAT,
                                   "Invalid type (0x%08" PRIx32 ") or flags"
                                   " (0x%08" PRIx32 ") field in segment"
                                   " %" PRIu32, phdr.p_type, phdr.p_flags, i);
            return RET_WARN;
        }
    }

    return RET_OK;
}

int sony_elf_reader_read_entry(MbBiReader *bir, void *userdata, Entry &entry)
{
    SonyElfReaderCtx *const ctx = static_cast<SonyElfReaderCtx *>(userdata);

    return ctx->seg.read_entry(*bir->file, entry, bir);
}

int sony_elf_reader_go_to_entry(MbBiReader *bir, void *userdata, Entry &entry,
                                int entry_type)
{
    SonyElfReaderCtx *const ctx = static_cast<SonyElfReaderCtx *>(userdata);

    return ctx->seg.go_to_entry(*bir->file, entry, entry_type, bir);
}

int sony_elf_reader_read_data(MbBiReader *bir, void *userdata,
                              void *buf, size_t buf_size,
                              size_t &bytes_read)
{
    SonyElfReaderCtx *const ctx = static_cast<SonyElfReaderCtx *>(userdata);

    return ctx->seg.read_data(*bir->file, buf, buf_size, bytes_read, bir);
}

int sony_elf_reader_free(MbBiReader *bir, void *userdata)
{
    (void) bir;
    delete static_cast<SonyElfReaderCtx *>(userdata);
    return RET_OK;
}

}

/*!
 * \brief Enable support for Sony ELF boot image format
 *
 * \param bir MbBiReader
 *
 * \return
 *   * #RET_OK if the format is successfully enabled
 *   * #RET_WARN if the format is already enabled
 *   * \<= #RET_FAILED if an error occurs
 */
int mb_bi_reader_enable_format_sony_elf(MbBiReader *bir)
{
    using namespace sonyelf;

    SonyElfReaderCtx *const ctx = new SonyElfReaderCtx();

    return _mb_bi_reader_register_format(bir,
                                         ctx,
                                         FORMAT_SONY_ELF,
                                         FORMAT_NAME_SONY_ELF,
                                         &sony_elf_reader_bid,
                                         nullptr,
                                         &sony_elf_reader_read_header,
                                         &sony_elf_reader_read_entry,
                                         &sony_elf_reader_go_to_entry,
                                         &sony_elf_reader_read_data,
                                         &sony_elf_reader_free);
}

}
}
