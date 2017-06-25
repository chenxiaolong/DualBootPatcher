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


MB_BEGIN_C_DECLS

/*!
 * \brief Find and read Sony ELF boot image header
 *
 * \note The integral fields in the header will be converted to the host's byte
 *       order.
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use mb_file_seek() to return to a known position.
 *
 * \param[in] bir MbBiReader for setting error messages
 * \param[in] file MbFile handle
 * \param[out] header_out Pointer to store header
 *
 * \return
 *   * #MB_BI_OK if the header is found
 *   * #MB_BI_WARN if the header is not found
 *   * #MB_BI_FAILED if any file operation fails non-fatally
 *   * #MB_BI_FATAL if any file operation fails fatally
 */
int find_sony_elf_header(MbBiReader *bir, MbFile *file,
                         Sony_Elf32_Ehdr *header_out)
{
    Sony_Elf32_Ehdr header;
    size_t n;
    int ret;

    ret = mb_file_seek(file, 0, SEEK_SET, nullptr);
    if (ret != MB_FILE_OK) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to seek to beginning: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    ret = mb_file_read_fully(file, &header, sizeof(header), &n);
    if (ret != MB_FILE_OK) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to read header: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    } else if (n != sizeof(header)) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "Too small to be Sony ELF image");
        return MB_BI_WARN;
    }

    if (memcmp(header.e_ident, SONY_E_IDENT, SONY_EI_NIDENT) != 0) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "Invalid ELF magic");
        return MB_BI_WARN;
    }

    sony_elf_fix_ehdr_byte_order(&header);
    *header_out = header;

    return MB_BI_OK;
}

/*!
 * \brief Perform a bid
 *
 * \return
 *   * If \>= 0, the number of bits that conform to the Sony ELF format
 *   * #MB_BI_WARN if this is a bid that can't be won
 *   * #MB_BI_FAILED if any file operations fail non-fatally
 *   * #MB_BI_FATAL if any file operations fail fatally
 */
int sony_elf_reader_bid(MbBiReader *bir, void *userdata, int best_bid)
{
    SonyElfReaderCtx *const ctx = static_cast<SonyElfReaderCtx *>(userdata);
    int bid = 0;
    int ret;

    if (best_bid >= SONY_EI_NIDENT * 8) {
        // This is a bid we can't win, so bail out
        return MB_BI_WARN;
    }

    // Find the Sony ELF header
    ret = find_sony_elf_header(bir, bir->file, &ctx->hdr);
    if (ret == MB_BI_OK) {
        // Update bid to account for matched bits
        ctx->have_header = true;
        bid += SONY_EI_NIDENT * 8;
    } else if (ret == MB_BI_WARN) {
        // Header not found. This can't be a Sony ELF boot image.
        return 0;
    } else {
        return ret;
    }

    return bid;
}

int sony_elf_reader_read_header(MbBiReader *bir, void *userdata,
                                MbBiHeader *header)
{
    SonyElfReaderCtx *const ctx = static_cast<SonyElfReaderCtx *>(userdata);
    int ret;

    if (!ctx->have_header) {
        // A bid might not have been performed if the user forced a particular
        // format
        ret = find_sony_elf_header(bir, bir->file, &ctx->hdr);
        if (ret < 0) {
            return ret;
        }
        ctx->have_header = true;
    }

    mb_bi_header_set_supported_fields(header, SONY_ELF_SUPPORTED_FIELDS);

    ret = mb_bi_header_set_entrypoint_address(header, ctx->hdr.e_entry);
    if (ret != MB_BI_OK) return ret;

    // Calculate offsets for each section

    uint64_t pos = 0;

    // pos cannot overflow due to the nature of the operands (adding UINT32_MAX
    // a few times can't overflow a uint64_t). File length overflow is checked
    // during read.

    // Header
    pos += sizeof(Sony_Elf32_Ehdr);

    // Reset entries
    _segment_reader_entries_clear(&ctx->segctx);

    // Read program segment headers
    for (Elf32_Half i = 0; i < ctx->hdr.e_phnum; ++i) {
        Sony_Elf32_Phdr phdr;
        size_t n;

        ret = mb_file_seek(bir->file, pos, SEEK_SET, nullptr);
        if (ret < 0) {
            mb_bi_reader_set_error(bir, mb_file_error(bir->file),
                                   "Failed to seek to segment %" PRIu16
                                   " at %" PRIu64 ": %s", i, pos,
                                   mb_file_error_string(bir->file));
            return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        }

        ret = mb_file_read_fully(bir->file, &phdr, sizeof(phdr), &n);
        if (ret < 0) {
            mb_bi_reader_set_error(bir, mb_file_error(bir->file),
                                   "Failed to read segment %" PRIu16 ": %s",
                                   i, mb_file_error_string(bir->file));
            return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        } else if (n != sizeof(phdr)) {
            mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                                   "Unexpected EOF when reading segment"
                                   " header %" PRIu16, i);
            return MB_BI_WARN;
        }

        // Account for program header
        pos += sizeof(phdr);

        // Fix byte order
        sony_elf_fix_phdr_byte_order(&phdr);

        if (phdr.p_type == SONY_E_TYPE_CMDLINE
                && phdr.p_flags == SONY_E_FLAGS_CMDLINE) {
            char cmdline[512];

            if (phdr.p_memsz >= sizeof(cmdline)) {
                mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                                       "cmdline too long");
                return MB_BI_WARN;
            }

            ret = mb_file_seek(bir->file, phdr.p_offset, SEEK_SET, nullptr);
            if (ret < 0) {
                mb_bi_reader_set_error(bir, mb_file_error(bir->file),
                                       "Failed to seek to cmdline: %s",
                                       mb_file_error_string(bir->file));
                return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
            }

            ret = mb_file_read_fully(bir->file, cmdline, phdr.p_memsz, &n);
            if (ret < 0) {
                mb_bi_reader_set_error(bir, mb_file_error(bir->file),
                                       "Failed to read cmdline: %s",
                                       mb_file_error_string(bir->file));
                return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
            } else if (n != phdr.p_memsz) {
                mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                                       "Unexpected EOF when reading cmdline");
                return MB_BI_WARN;
            }

            cmdline[n] = '\0';

            ret = mb_bi_header_set_kernel_cmdline(header, cmdline);
            if (ret != MB_BI_OK) return ret;
        } else if (phdr.p_type == SONY_E_TYPE_KERNEL
                && phdr.p_flags == SONY_E_FLAGS_KERNEL) {
            ret = _segment_reader_entries_add(&ctx->segctx, MB_BI_ENTRY_KERNEL,
                                              phdr.p_offset, phdr.p_memsz,
                                              false, bir);
            if (ret != MB_BI_OK) return ret;

            ret = mb_bi_header_set_kernel_address(header, phdr.p_vaddr);
            if (ret != MB_BI_OK) return ret;
        } else if (phdr.p_type == SONY_E_TYPE_RAMDISK
                && phdr.p_flags == SONY_E_FLAGS_RAMDISK) {
            ret = _segment_reader_entries_add(&ctx->segctx, MB_BI_ENTRY_RAMDISK,
                                              phdr.p_offset, phdr.p_memsz,
                                              false, bir);
            if (ret != MB_BI_OK) return ret;

            ret = mb_bi_header_set_ramdisk_address(header, phdr.p_vaddr);
            if (ret != MB_BI_OK) return ret;
        } else if (phdr.p_type == SONY_E_TYPE_IPL
                && phdr.p_flags == SONY_E_FLAGS_IPL) {
            ret = _segment_reader_entries_add(&ctx->segctx,
                                              MB_BI_ENTRY_SONY_IPL,
                                              phdr.p_offset, phdr.p_memsz,
                                              false, bir);
            if (ret != MB_BI_OK) return ret;

            ret = mb_bi_header_set_sony_ipl_address(header, phdr.p_vaddr);
            if (ret != MB_BI_OK) return ret;
        } else if (phdr.p_type == SONY_E_TYPE_RPM
                && phdr.p_flags == SONY_E_FLAGS_RPM) {
            ret = _segment_reader_entries_add(&ctx->segctx,
                                              MB_BI_ENTRY_SONY_RPM,
                                              phdr.p_offset, phdr.p_memsz,
                                              false, bir);
            if (ret != MB_BI_OK) return ret;

            ret = mb_bi_header_set_sony_rpm_address(header, phdr.p_vaddr);
            if (ret != MB_BI_OK) return ret;
        } else if (phdr.p_type == SONY_E_TYPE_APPSBL
                && phdr.p_flags == SONY_E_FLAGS_APPSBL) {
            ret = _segment_reader_entries_add(&ctx->segctx,
                                              MB_BI_ENTRY_SONY_APPSBL,
                                              phdr.p_offset, phdr.p_memsz,
                                              false, bir);
            if (ret != MB_BI_OK) return ret;

            ret = mb_bi_header_set_sony_appsbl_address(header, phdr.p_vaddr);
            if (ret != MB_BI_OK) return ret;
        } else if (phdr.p_type == SONY_E_TYPE_SIN) {
            // Skip SIN entry. It contains an RSA signature that we can't
            // recreate (without the private key), so there's no point in
            // dumping this segment.
            continue;
        } else {
            mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                                   "Invalid type (0x%08" PRIx32 ") or flags"
                                   " (0x%08" PRIx32 ") field in segment"
                                   " %" PRIu32, phdr.p_type, phdr.p_flags, i);
            return MB_BI_WARN;
        }
    }

    return MB_BI_OK;
}

int sony_elf_reader_read_entry(MbBiReader *bir, void *userdata,
                               MbBiEntry *entry)
{
    SonyElfReaderCtx *const ctx = static_cast<SonyElfReaderCtx *>(userdata);

    return _segment_reader_read_entry(&ctx->segctx, bir->file, entry, bir);
}

int sony_elf_reader_go_to_entry(MbBiReader *bir, void *userdata,
                                MbBiEntry *entry, int entry_type)
{
    SonyElfReaderCtx *const ctx = static_cast<SonyElfReaderCtx *>(userdata);

    return _segment_reader_go_to_entry(&ctx->segctx, bir->file, entry,
                                       entry_type, bir);
}

int sony_elf_reader_read_data(MbBiReader *bir, void *userdata,
                              void *buf, size_t buf_size,
                              size_t *bytes_read)
{
    SonyElfReaderCtx *const ctx = static_cast<SonyElfReaderCtx *>(userdata);

    return _segment_reader_read_data(&ctx->segctx, bir->file, buf, buf_size,
                                     bytes_read, bir);
}

int sony_elf_reader_free(MbBiReader *bir, void *userdata)
{
    (void) bir;
    SonyElfReaderCtx *const ctx = static_cast<SonyElfReaderCtx *>(userdata);
    _segment_reader_deinit(&ctx->segctx);
    free(ctx);
    return MB_BI_OK;
}

/*!
 * \brief Enable support for Sony ELF boot image format
 *
 * \param bir MbBiReader
 *
 * \return
 *   * #MB_BI_OK if the format is successfully enabled
 *   * #MB_BI_WARN if the format is already enabled
 *   * \<= #MB_BI_FAILED if an error occurs
 */
int mb_bi_reader_enable_format_sony_elf(MbBiReader *bir)
{
    SonyElfReaderCtx *const ctx = static_cast<SonyElfReaderCtx *>(
            calloc(1, sizeof(SonyElfReaderCtx)));
    if (!ctx) {
        mb_bi_reader_set_error(bir, -errno,
                               "Failed to allocate SonyElfReaderCtx: %s",
                               strerror(errno));
        return MB_BI_FAILED;
    }

    _segment_reader_init(&ctx->segctx);

    return _mb_bi_reader_register_format(bir,
                                         ctx,
                                         MB_BI_FORMAT_SONY_ELF,
                                         MB_BI_FORMAT_NAME_SONY_ELF,
                                         &sony_elf_reader_bid,
                                         nullptr,
                                         &sony_elf_reader_read_header,
                                         &sony_elf_reader_read_entry,
                                         &sony_elf_reader_go_to_entry,
                                         &sony_elf_reader_read_data,
                                         &sony_elf_reader_free);
}

MB_END_C_DECLS
