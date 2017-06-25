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
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"


MB_BEGIN_C_DECLS

/*!
 * \brief Find and read Loki boot image header
 *
 * \note The integral fields in the header will be converted to the host's byte
 *       order.
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use mb_file_seek() to return to a known position.
 *
 * \param[in] bir MbBiReader
 * \param[in] file MbFile handle
 * \param[out] header_out Pointer to store header
 * \param[out] offset_out Pointer to store header offset
 *
 * \return
 *   * #MB_BI_OK if the header is found
 *   * #MB_BI_WARN if the header is not found
 *   * #MB_BI_FAILED if any file operation fails non-fatally
 *   * #MB_BI_FATAL if any file operation fails fatally
 */
int find_loki_header(MbBiReader *bir, MbFile *file,
                     LokiHeader *header_out, uint64_t *offset_out)
{
    LokiHeader header;
    size_t n;
    int ret;

    ret = mb_file_seek(file, LOKI_MAGIC_OFFSET, SEEK_SET, nullptr);
    if (ret < 0) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Loki magic not found: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_WARN;
    }

    ret = mb_file_read_fully(file, &header, sizeof(header), &n);
    if (ret < 0) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to read header: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    } else if (n != sizeof(header)) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "Too small to be Loki image");
        return MB_BI_WARN;
    }

    if (memcmp(header.magic, LOKI_MAGIC, LOKI_MAGIC_SIZE) != 0) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "Invalid loki magic");
        return MB_BI_WARN;
    }

    loki_fix_header_byte_order(&header);
    *header_out = header;
    *offset_out = LOKI_MAGIC_OFFSET;

    return MB_BI_OK;
}

/*!
 * \brief Find and read Loki ramdisk address
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use mb_file_seek() to return to a known position.
 *
 * \param[in] bir MbBiReader to set error message
 * \param[in] file MbFile handle
 * \param[in] hdr Android header
 * \param[in] loki_hdr Loki header
 * \param[out] ramdisk_addr_out Pointer to store ramdisk address
 *
 * \return
 *   * #MB_BI_OK if the ramdisk address is found
 *   * #MB_BI_WARN if the ramdisk address is not found
 *   * #MB_BI_FAILED if any file operation fails non-fatally
 *   * #MB_BI_FATAL if any file operation fails fatally
 */
int loki_find_ramdisk_address(MbBiReader *bir, MbFile *file,
                              const AndroidHeader *hdr,
                              const LokiHeader *loki_hdr,
                              uint32_t *ramdisk_addr_out)
{
    // If the boot image was patched with a newer version of loki, find the
    // ramdisk offset in the shell code
    uint32_t ramdisk_addr = 0;
    int ret;
    size_t n;

    if (loki_hdr->ramdisk_addr != 0) {
        uint64_t offset = 0;

        auto result_cb = [](MbFile *file, void *userdata,
                            uint64_t offset) -> int {
            (void) file;
            uint64_t *offset_ptr = static_cast<uint64_t *>(userdata);
            *offset_ptr = offset;
            return MB_FILE_OK;
        };

        ret = mb_file_search(file, -1, -1, 0, LOKI_SHELLCODE,
                             LOKI_SHELLCODE_SIZE - 9, 1, result_cb, &offset);
        if (ret < 0) {
            mb_bi_reader_set_error(bir, mb_file_error(file),
                                   "Failed to search for Loki shellcode: %s",
                                   mb_file_error_string(file));
            return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        }

        if (offset == 0) {
            mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                                   "Loki shellcode not found");
            return MB_FILE_WARN;
        }

        offset += LOKI_SHELLCODE_SIZE - 5;

        ret = mb_file_seek(file, offset, SEEK_SET, nullptr);
        if (ret < 0) {
            mb_bi_reader_set_error(bir, mb_file_error(file),
                                   "Failed to seek to ramdisk address offset: %s",
                                   mb_file_error_string(file));
            return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        }

        ret = mb_file_read_fully(file, &ramdisk_addr,
                                 sizeof(ramdisk_addr), &n);
        if (ret < 0) {
            mb_bi_reader_set_error(bir, mb_file_error(file),
                                   "Failed to read ramdisk address offset: %s",
                                   mb_file_error_string(file));
            return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        } else if (n != sizeof(ramdisk_addr)) {
            mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                                   "Unexpected EOF when reading ramdisk address");
            return MB_BI_WARN;
        }

        ramdisk_addr = mb_le32toh(ramdisk_addr);
    } else {
        // Otherwise, use the default for jflte (- 0x00008000 + 0x02000000)

        if (hdr->kernel_addr > UINT32_MAX - 0x01ff8000) {
            mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                                   "Invalid kernel address: %" PRIu32,
                                   hdr->kernel_addr);
            return MB_BI_WARN;
        }

        ramdisk_addr = hdr->kernel_addr + 0x01ff8000;
    }

    *ramdisk_addr_out = ramdisk_addr;
    return MB_BI_OK;
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
 *       Use mb_file_seek() to return to a known position.
 *
 * \param[in] bir MbBiReader to set error message
 * \param[in] file MbFile handle
 * \param[in] start_offset Starting offset for search
 * \param[out] gzip_offset_out Pointer to store gzip ramdisk offset
 *
 * \return
 *   * #MB_BI_OK if a gzip offset is found
 *   * #MB_BI_WARN if no gzip offsets are found
 *   * #MB_BI_FAILED if any file operation fails non-fatally
 *   * #MB_BI_FATAL if any file operation fails fatally
 */
int loki_old_find_gzip_offset(MbBiReader *bir, MbFile *file,
                              uint32_t start_offset, uint64_t *gzip_offset_out)
{
    struct SearchResult
    {
        bool have_flag0 = false;
        bool have_flag8 = false;
        uint64_t flag0_offset;
        uint64_t flag8_offset;
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
    int ret;

    // Find first result with flags == 0x00 and flags == 0x08
    auto result_cb = [](MbFile *file, void *userdata, uint64_t offset) -> int {
        SearchResult *result = static_cast<SearchResult *>(userdata);
        uint64_t orig_offset;
        unsigned char flags;
        size_t n;
        int ret;

        // Stop early if possible
        if (result->have_flag0 && result->have_flag8) {
            return MB_FILE_WARN;
        }

        // Save original position
        ret = mb_file_seek(file, 0, SEEK_CUR, &orig_offset);
        if (ret != MB_FILE_OK) {
            return ret;
        }

        // Seek to flags byte
        ret = mb_file_seek(file, offset + 3, SEEK_SET, nullptr);
        if (ret != MB_FILE_OK) {
            return ret;
        }

        // Read next bytes for flags
        ret = mb_file_read_fully(file, &flags, sizeof(flags), &n);
        if (ret != MB_FILE_OK) {
            return ret;
        } else if (n != sizeof(flags)) {
            // EOF
            return MB_FILE_WARN;
        }

        if (!result->have_flag0 && flags == 0x00) {
            result->have_flag0 = true;
            result->flag0_offset = offset;
        } else if (!result->have_flag8 && flags == 0x08) {
            result->have_flag8 = true;
            result->flag8_offset = offset;
        }

        // Restore original position as per contract
        ret = mb_file_seek(file, orig_offset, SEEK_SET, nullptr);
        if (ret != MB_FILE_OK) {
            return ret;
        }

        return MB_FILE_OK;
    };

    ret = mb_file_search(file, start_offset, -1, 0, gzip_deflate_magic,
                         sizeof(gzip_deflate_magic), -1, result_cb, &result);
    if (ret < 0) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to search for gzip magic: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    // Prefer gzip header with original filename flag since most loki'd boot
    // images will have been compressed manually with the gzip tool
    if (result.have_flag8) {
        *gzip_offset_out = result.flag8_offset;
    } else if (result.have_flag0) {
        *gzip_offset_out = result.flag0_offset;
    } else {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "No gzip headers found");
        return MB_BI_WARN;
    }

    return MB_BI_OK;
}

/*!
 * \brief Find ramdisk size in old-style Loki image
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use mb_file_seek() to return to a known position.
 *
 * \param[in] bir MbBiReader to set error message
 * \param[in] file MbFile handle
 * \param[in] hdr Android header
 * \param[in] ramdisk_offset Offset of ramdisk in image
 * \param[out] ramdisk_size_out Pointer to store ramdisk size
 *
 * \return
 *   * #MB_BI_OK if the ramdisk size is found
 *   * #MB_BI_WARN if the ramdisk size is not found
 *   * #MB_BI_FAILED if any file operation fails non-fatally
 *   * #MB_BI_FATAL if any file operation fails fatally
 */
int loki_old_find_ramdisk_size(MbBiReader *bir, MbFile *file,
                               const AndroidHeader *hdr,
                               uint32_t ramdisk_offset,
                               uint32_t *ramdisk_size_out)
{
    int32_t aboot_size;
    uint64_t aboot_offset;
#if 0
    uint64_t cur_offset;
    char buf[1024];
    size_t n;
#endif
    int ret;

    // If the boot image was patched with an old version of loki, the ramdisk
    // size is not stored properly. We'll need to guess the size of the archive.

    // The ramdisk is supposed to be from the gzip header to EOF, but loki needs
    // to store a copy of aboot, so it is put in the last 0x200 bytes of the
    // file.
    if (LOKI_IS_LG_RAMDISK_ADDR(hdr->ramdisk_addr)) {
        aboot_size = hdr->page_size;
    } else {
        aboot_size = 0x200;
    }

    ret = mb_file_seek(file, -aboot_size, SEEK_END, &aboot_offset);
    if (ret != MB_FILE_OK) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to seek to end of file: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    if (ramdisk_offset > aboot_offset) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_INTERNAL_ERROR,
                               "Ramdisk offset greater than aboot offset");
        return MB_BI_FAILED;
    }

    // Ignore zero padding as we might strip away too much
#if 1
    *ramdisk_size_out = aboot_offset - ramdisk_offset;
    return MB_BI_OK;
#else
    // Search backwards to find non-zero byte
    cur_offset = aboot_offset;

    while (cur_offset > ramdisk_offset) {
        size_t to_read = std::min<uint64_t>(
                sizeof(buf), cur_offset - ramdisk_offset);
        cur_offset -= to_read;

        ret = mb_file_seek(file, cur_offset, SEEK_SET, nullptr);
        if (ret != MB_FILE_OK) {
            mb_bi_reader_set_error(bir, mb_file_error(file),
                                   "Failed to seek: %s",
                                   mb_file_error_string(file));
            return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        }

        ret = mb_file_read_fully(file, buf, to_read, &n);
        if (ret != MB_FILE_OK) {
            mb_bi_reader_set_error(bir, mb_file_error(file),
                                   "Failed to read: %s",
                                   mb_file_error_string(file));
            return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
        } else if (n != to_read) {
            mb_bi_reader_set_error(bir, MB_BI_ERROR_INTERNAL_ERROR,
                                   "Unexpected file truncation");
            return MB_BI_FAILED;
        }

        for (size_t i = n; i-- > 0; ) {
            if (buf[i] != '\0') {
                *ramdisk_size_out = cur_offset - ramdisk_offset + i;
                return MB_BI_OK;
            }
        }
    }

    mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                           "Could not determine ramdisk size");
    return MB_BI_WARN;
#endif
}

/*!
 * \brief Find size of Linux kernel in boot image
 *
 * \pre The file position can be at any offset prior to calling this function.
 *
 * \post The file pointer position is undefined after this function returns.
 *       Use mb_file_seek() to return to a known position.
 *
 * \param[in] bir MbBiReader to set error message
 * \param[in] file MbFile handle
 * \param[in] kernel_offset Offset of kernel in boot image
 * \param[out] kernel_size_out Pointer to store kernel size
 *
 * \return
 *   * #MB_BI_OK if the kernel size is found
 *   * #MB_BI_WARN if the kernel size cannot be found
 *   * #MB_BI_FAILED if any file operation fails non-fatally
 *   * #MB_BI_FATAL if any file operation fails fatally
 */
int find_linux_kernel_size(MbBiReader *bir, MbFile *file,
                           uint32_t kernel_offset, uint32_t *kernel_size_out)
{
    int ret;
    size_t n;
    uint32_t kernel_size;

    // If the boot image was patched with an early version of loki, the
    // original kernel size is not stored in the loki header properly (or in the
    // shellcode). The size is stored in the kernel image's header though, so
    // we'll use that.
    // http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html#d0e309
    ret = mb_file_seek(file, kernel_offset + 0x2c, SEEK_SET, nullptr);
    if (ret != MB_FILE_OK) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to seek to kernel header: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    }

    ret = mb_file_read_fully(file, &kernel_size, sizeof(kernel_size), &n);
    if (ret != MB_FILE_OK) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Failed to read size from kernel header: %s",
                               mb_file_error_string(file));
        return ret == MB_FILE_FATAL ? MB_BI_FATAL : MB_BI_FAILED;
    } else if (n != sizeof(kernel_size)) {
        mb_bi_reader_set_error(bir, mb_file_error(file),
                               "Unexpected EOF when reading kernel header");
        return MB_BI_WARN;
    }

    *kernel_size_out = mb_le32toh(kernel_size);
    return MB_BI_OK;
}

/*!
 * \brief Read header for old-style Loki image
 *
 * \param[in] bir MbBiReader to set error message
 * \param[in] file MbFile handle
 * \param[in] hdr Android header for image
 * \param[in] loki_hdr Loki header for image
 * \param[out] header MbBiHeader instance to store header values
 * \param[out] kernel_offset_out Pointer to store kernel offset
 * \param[out] kernel_size_out Pointer to store kernel size
 * \param[out] ramdisk_offset_out Pointer to store ramdisk offset
 * \param[out] ramdisk_size_out Pointer to store ramdisk size
 *
 * \return
 *   * #MB_BI_OK if the header is successfully read
 *   * #MB_BI_WARN if parts of the header are missing or invalid
 *   * #MB_BI_FAILED if any file operation fails non-fatally
 *   * #MB_BI_FATAL if any file operation fails fatally
 */
int loki_read_old_header(MbBiReader *bir, MbFile *file,
                         AndroidHeader *hdr, LokiHeader *loki_hdr,
                         MbBiHeader *header,
                         uint64_t *kernel_offset_out,
                         uint32_t *kernel_size_out,
                         uint64_t *ramdisk_offset_out,
                         uint32_t *ramdisk_size_out)
{
    uint32_t tags_addr;
    uint32_t kernel_size;
    uint32_t ramdisk_size;
    uint32_t ramdisk_addr;
    uint64_t gzip_offset;
    int ret;

    if (hdr->page_size == 0) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "Page size cannot be 0");
        return MB_BI_WARN;
    }

    // The kernel tags address is invalid in the old loki images, so use the
    // default for jflte
    tags_addr = hdr->kernel_addr - ANDROID_DEFAULT_KERNEL_OFFSET
            + ANDROID_DEFAULT_TAGS_OFFSET;

    // Try to guess kernel size
    ret = find_linux_kernel_size(bir, file, hdr->page_size, &kernel_size);
    if (ret != MB_BI_OK) {
        return ret;
    }

    // Look for gzip offset for the ramdisk
    ret = loki_old_find_gzip_offset(
            bir, file, hdr->page_size + kernel_size
            + align_page_size<uint64_t>(kernel_size, hdr->page_size),
            &gzip_offset);
    if (ret != MB_BI_OK) {
        return ret;
    }

    // Try to guess ramdisk size
    ret = loki_old_find_ramdisk_size(bir, file, hdr, gzip_offset,
                                     &ramdisk_size);
    if (ret != MB_BI_OK) {
        return ret;
    }

    // Guess original ramdisk address
    ret = loki_find_ramdisk_address(bir, file, hdr, loki_hdr, &ramdisk_addr);
    if (ret != MB_BI_OK) {
        return ret;
    }

    *kernel_size_out = kernel_size;
    *ramdisk_size_out = ramdisk_size;

    char board_name[sizeof(hdr->name) + 1];
    char cmdline[sizeof(hdr->cmdline) + 1];

    strncpy(board_name, reinterpret_cast<char *>(hdr->name),
            sizeof(hdr->name));
    strncpy(cmdline, reinterpret_cast<char *>(hdr->cmdline),
            sizeof(hdr->cmdline));
    board_name[sizeof(hdr->name)] = '\0';
    cmdline[sizeof(hdr->cmdline)] = '\0';

    mb_bi_header_set_supported_fields(header, LOKI_OLD_SUPPORTED_FIELDS);

    ret = mb_bi_header_set_board_name(header, board_name);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_kernel_cmdline(header, cmdline);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_page_size(header, hdr->page_size);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_kernel_address(header, hdr->kernel_addr);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_ramdisk_address(header, ramdisk_addr);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_secondboot_address(header, hdr->second_addr);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_kernel_tags_address(header, tags_addr);
    if (ret != MB_BI_OK) return ret;

    uint64_t pos = 0;

    // pos cannot overflow due to the nature of the operands (adding UINT32_MAX
    // a few times can't overflow a uint64_t). File length overflow is checked
    // during read.

    // Header
    pos += hdr->page_size;

    // Kernel
    *kernel_offset_out = pos;
    pos += kernel_size;
    pos += align_page_size<uint64_t>(pos, hdr->page_size);

    // Ramdisk
    *ramdisk_offset_out = pos = gzip_offset;
    pos += ramdisk_size;
    pos += align_page_size<uint64_t>(pos, hdr->page_size);

    return MB_BI_OK;
}

/*!
 * \brief Read header for new-style Loki image
 *
 * \param[in] bir MbBiReader to set error message
 * \param[in] file MbFile handle
 * \param[in] hdr Android header for image
 * \param[in] loki_hdr Loki header for image
 * \param[out] header MbBiHeader instance to store header values
 * \param[out] kernel_offset_out Pointer to store kernel offset
 * \param[out] kernel_size_out Pointer to store kernel size
 * \param[out] ramdisk_offset_out Pointer to store ramdisk offset
 * \param[out] ramdisk_size_out Pointer to store ramdisk size
 * \param[out] dt_offset_out Pointer to store device tree offset
 *
 * \return
 *   * #MB_BI_OK if the header is successfully read
 *   * #MB_BI_WARN if parts of the header are missing or invalid
 *   * #MB_BI_FAILED if any file operation fails non-fatally
 *   * #MB_BI_FATAL if any file operation fails fatally
 */
int loki_read_new_header(MbBiReader *bir, MbFile *file,
                         AndroidHeader *hdr, LokiHeader *loki_hdr,
                         MbBiHeader *header,
                         uint64_t *kernel_offset_out,
                         uint32_t *kernel_size_out,
                         uint64_t *ramdisk_offset_out,
                         uint32_t *ramdisk_size_out,
                         uint64_t *dt_offset_out)
{
    uint32_t fake_size;
    uint32_t ramdisk_addr;
    int ret;

    if (hdr->page_size == 0) {
        mb_bi_reader_set_error(bir, MB_BI_ERROR_FILE_FORMAT,
                               "Page size cannot be 0");
        return MB_BI_WARN;
    }

    if (LOKI_IS_LG_RAMDISK_ADDR(hdr->ramdisk_addr)) {
        fake_size = hdr->page_size;
    } else {
        fake_size = 0x200;
    }

    // Find original ramdisk address
    ret = loki_find_ramdisk_address(bir, file, hdr, loki_hdr, &ramdisk_addr);
    if (ret != MB_BI_OK) {
        return ret;
    }

    // Restore original values in boot image header
    *kernel_size_out = loki_hdr->orig_kernel_size;
    *ramdisk_size_out = loki_hdr->orig_ramdisk_size;

    char board_name[sizeof(hdr->name) + 1];
    char cmdline[sizeof(hdr->cmdline) + 1];

    strncpy(board_name, reinterpret_cast<char *>(hdr->name),
            sizeof(hdr->name));
    strncpy(cmdline, reinterpret_cast<char *>(hdr->cmdline),
            sizeof(hdr->cmdline));
    board_name[sizeof(hdr->name)] = '\0';
    cmdline[sizeof(hdr->cmdline)] = '\0';

    mb_bi_header_set_supported_fields(header, LOKI_NEW_SUPPORTED_FIELDS);

    ret = mb_bi_header_set_board_name(header, board_name);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_kernel_cmdline(header, cmdline);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_page_size(header, hdr->page_size);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_kernel_address(header, hdr->kernel_addr);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_ramdisk_address(header, ramdisk_addr);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_secondboot_address(header, hdr->second_addr);
    if (ret != MB_BI_OK) return ret;

    ret = mb_bi_header_set_kernel_tags_address(header, hdr->tags_addr);
    if (ret != MB_BI_OK) return ret;

    uint64_t pos = 0;

    // pos cannot overflow due to the nature of the operands (adding UINT32_MAX
    // a few times can't overflow a uint64_t). File length overflow is checked
    // during read.

    // Header
    pos += hdr->page_size;

    // Kernel
    *kernel_offset_out = pos;
    pos += loki_hdr->orig_kernel_size;
    pos += align_page_size<uint64_t>(pos, hdr->page_size);

    // Ramdisk
    *ramdisk_offset_out = pos;
    pos += loki_hdr->orig_ramdisk_size;
    pos += align_page_size<uint64_t>(pos, hdr->page_size);

    // Device tree
    if (hdr->dt_size != 0) {
        pos += fake_size;
    }
    *dt_offset_out = pos;
    pos += hdr->dt_size;
    pos += align_page_size<uint64_t>(pos, hdr->page_size);

    return MB_BI_OK;
}

/*!
 * \brief Perform a bid
 *
 * \return
 *   * If \>= 0, the number of bits that conform to the Loki format
 *   * #MB_BI_WARN if this is a bid that can't be won
 *   * #MB_BI_FAILED if any file operations fail non-fatally
 *   * #MB_BI_FATAL if any file operations fail fatally
 */
int loki_reader_bid(MbBiReader *bir, void *userdata, int best_bid)
{
    LokiReaderCtx *const ctx = static_cast<LokiReaderCtx *>(userdata);
    int bid = 0;
    int ret;

    if (best_bid >= (ANDROID_BOOT_MAGIC_SIZE + LOKI_MAGIC_SIZE) * 8) {
        // This is a bid we can't win, so bail out
        return MB_BI_WARN;
    }

    // Find the Loki header
    ret = find_loki_header(bir, bir->file, &ctx->loki_hdr, &ctx->loki_offset);
    if (ret == MB_BI_OK) {
        // Update bid to account for matched bits
        ctx->have_loki_offset = true;
        bid += LOKI_MAGIC_SIZE * 8;
    } else if (ret == MB_BI_WARN) {
        // Header not found. This can't be a Loki boot image.
        return 0;
    } else {
        return ret;
    }

    // Find the Android header
    ret = find_android_header(bir, bir->file, LOKI_MAX_HEADER_OFFSET,
                              &ctx->hdr, &ctx->header_offset);
    if (ret == MB_BI_OK) {
        // Update bid to account for matched bits
        ctx->have_header_offset = true;
        bid += ANDROID_BOOT_MAGIC_SIZE * 8;
    } else if (ret == MB_BI_WARN) {
        // Header not found. This can't be an Android boot image.
        return 0;
    } else {
        return ret;
    }

    return bid;
}

int loki_reader_read_header(MbBiReader *bir, void *userdata,
                            MbBiHeader *header)
{
    LokiReaderCtx *const ctx = static_cast<LokiReaderCtx *>(userdata);
    int ret;
    uint64_t kernel_offset;
    uint64_t ramdisk_offset;
    uint64_t dt_offset = 0;
    uint32_t kernel_size;
    uint32_t ramdisk_size;

    // A bid might not have been performed if the user forced a particular
    // format
    if (!ctx->have_loki_offset) {
        ret = find_loki_header(bir, bir->file, &ctx->loki_hdr,
                               &ctx->header_offset);
        if (ret < 0) {
            return ret;
        }
        ctx->have_loki_offset = true;
    }
    if (!ctx->have_header_offset) {
        ret = find_android_header(bir, bir->file, ANDROID_MAX_HEADER_OFFSET,
                                  &ctx->hdr, &ctx->header_offset);
        if (ret < 0) {
            return ret;
        }
        ctx->have_header_offset = true;
    }

    // New-style images record the original values of changed fields in the
    // Android header
    if (ctx->loki_hdr.orig_kernel_size != 0
            && ctx->loki_hdr.orig_ramdisk_size != 0
            && ctx->loki_hdr.ramdisk_addr != 0) {
        ret =  loki_read_new_header(bir, bir->file,
                                    &ctx->hdr, &ctx->loki_hdr, header,
                                    &kernel_offset, &kernel_size,
                                    &ramdisk_offset, &ramdisk_size,
                                    &dt_offset);
    } else {
        ret =  loki_read_old_header(bir, bir->file,
                                    &ctx->hdr, &ctx->loki_hdr, header,
                                    &kernel_offset, &kernel_size,
                                    &ramdisk_offset, &ramdisk_size);
    }
    if (ret < 0) {
        return ret;
    }

    _segment_reader_entries_clear(&ctx->segctx);

    ret = _segment_reader_entries_add(&ctx->segctx, MB_BI_ENTRY_KERNEL,
                                      kernel_offset, kernel_size, false, bir);
    if (ret != MB_BI_OK) return ret;

    ret = _segment_reader_entries_add(&ctx->segctx, MB_BI_ENTRY_RAMDISK,
                                      ramdisk_offset, ramdisk_size, false, bir);
    if (ret != MB_BI_OK) return ret;

    if (ctx->hdr.dt_size > 0 && dt_offset != 0) {
        ret = _segment_reader_entries_add(&ctx->segctx, MB_BI_ENTRY_DEVICE_TREE,
                                          dt_offset, ctx->hdr.dt_size, false,
                                          bir);
        if (ret != MB_BI_OK) return ret;
    }

    return MB_BI_OK;
}

int loki_reader_read_entry(MbBiReader *bir, void *userdata,
                           MbBiEntry *entry)
{
    LokiReaderCtx *const ctx = static_cast<LokiReaderCtx *>(userdata);

    return _segment_reader_read_entry(&ctx->segctx, bir->file, entry, bir);
}

int loki_reader_go_to_entry(MbBiReader *bir, void *userdata, MbBiEntry *entry,
                            int entry_type)
{
    LokiReaderCtx *const ctx = static_cast<LokiReaderCtx *>(userdata);

    return _segment_reader_go_to_entry(&ctx->segctx, bir->file, entry,
                                       entry_type, bir);
}

int loki_reader_read_data(MbBiReader *bir, void *userdata,
                          void *buf, size_t buf_size,
                          size_t *bytes_read)
{
    LokiReaderCtx *const ctx = static_cast<LokiReaderCtx *>(userdata);

    return _segment_reader_read_data(&ctx->segctx, bir->file, buf, buf_size,
                                     bytes_read, bir);
}

int loki_reader_free(MbBiReader *bir, void *userdata)
{
    (void) bir;
    LokiReaderCtx *const ctx = static_cast<LokiReaderCtx *>(userdata);
    _segment_reader_deinit(&ctx->segctx);
    free(ctx);
    return MB_BI_OK;
}

/*!
 * \brief Enable support for Loki boot image format
 *
 * \param bir MbBiReader
 *
 * \return
 *   * #MB_BI_OK if the format is successfully enabled
 *   * #MB_BI_WARN if the format is already enabled
 *   * \<= #MB_BI_FAILED if an error occurs
 */
int mb_bi_reader_enable_format_loki(MbBiReader *bir)
{
    LokiReaderCtx *const ctx = static_cast<LokiReaderCtx *>(
            calloc(1, sizeof(LokiReaderCtx)));
    if (!ctx) {
        mb_bi_reader_set_error(bir, -errno,
                               "Failed to allocate LokiReaderCtx: %s",
                               strerror(errno));
        return MB_BI_FAILED;
    }

    _segment_reader_init(&ctx->segctx);

    return _mb_bi_reader_register_format(bir,
                                         ctx,
                                         MB_BI_FORMAT_LOKI,
                                         MB_BI_FORMAT_NAME_LOKI,
                                         &loki_reader_bid,
                                         nullptr,
                                         &loki_reader_read_header,
                                         &loki_reader_read_entry,
                                         &loki_reader_go_to_entry,
                                         &loki_reader_read_data,
                                         &loki_reader_free);
}

MB_END_C_DECLS
