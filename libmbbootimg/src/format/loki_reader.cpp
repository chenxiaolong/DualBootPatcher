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
#include "mbbootimg/format/loki_error.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"
#include "mbbootimg/reader_p.h"


namespace mb
{
namespace bootimg
{
namespace loki
{

LokiFormatReader::LokiFormatReader(Reader &reader)
    : FormatReader(reader)
    , _hdr()
    , _loki_hdr()
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
 *   * #RET_WARN if this is a bid that can't be won
 *   * #RET_FAILED if any file operations fail non-fatally
 *   * #RET_FATAL if any file operations fail fatally
 */
int LokiFormatReader::bid(File &file, int best_bid)
{
    int bid = 0;
    int ret;

    if (best_bid >= static_cast<int>(
            android::BOOT_MAGIC_SIZE + LOKI_MAGIC_SIZE) * 8) {
        // This is a bid we can't win, so bail out
        return RET_WARN;
    }

    // Find the Loki header
    uint64_t loki_offset;
    ret = find_loki_header(_reader, file, _loki_hdr, loki_offset);
    if (ret == RET_OK) {
        // Update bid to account for matched bits
        _loki_offset = loki_offset;
        bid += static_cast<int>(LOKI_MAGIC_SIZE * 8);
    } else if (ret == RET_WARN) {
        // Header not found. This can't be a Loki boot image.
        return 0;
    } else {
        return ret;
    }

    // Find the Android header
    uint64_t header_offset;
    ret = android::AndroidFormatReader::find_header(
            _reader, file, LOKI_MAX_HEADER_OFFSET, _hdr, header_offset);
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

    return bid;
}

int LokiFormatReader::read_header(File &file, Header &header)
{
    int ret;
    uint64_t kernel_offset;
    uint64_t ramdisk_offset;
    uint64_t dt_offset = 0;
    uint32_t kernel_size;
    uint32_t ramdisk_size;

    // A bid might not have been performed if the user forced a particular
    // format
    if (!_loki_offset) {
        uint64_t loki_offset;
        ret = find_loki_header(_reader, file, _loki_hdr, loki_offset);
        if (ret < 0) {
            return ret;
        }
        _loki_offset = loki_offset;
    }
    if (!_header_offset) {
        uint64_t header_offset;
        ret = android::AndroidFormatReader::find_header(
                _reader, file, android::MAX_HEADER_OFFSET, _hdr,
                header_offset);
        if (ret < 0) {
            return ret;
        }
        _header_offset = header_offset;
    }

    // New-style images record the original values of changed fields in the
    // Android header
    if (_loki_hdr.orig_kernel_size != 0
            && _loki_hdr.orig_ramdisk_size != 0
            && _loki_hdr.ramdisk_addr != 0) {
        ret =  read_header_new(_reader, file, _hdr, _loki_hdr, header,
                               kernel_offset, kernel_size,
                               ramdisk_offset, ramdisk_size,
                               dt_offset);
    } else {
        ret =  read_header_old(_reader, file, _hdr, _loki_hdr, header,
                               kernel_offset, kernel_size,
                               ramdisk_offset, ramdisk_size);
    }
    if (ret < 0) {
        return ret;
    }

    _seg.entries_clear();

    ret = _seg.entries_add(ENTRY_TYPE_KERNEL,
                           kernel_offset, kernel_size, false, _reader);
    if (ret != RET_OK) { return ret; }

    ret = _seg.entries_add(ENTRY_TYPE_RAMDISK,
                           ramdisk_offset, ramdisk_size, false, _reader);
    if (ret != RET_OK) { return ret; }

    if (_hdr.dt_size > 0 && dt_offset != 0) {
        ret = _seg.entries_add(ENTRY_TYPE_DEVICE_TREE,
                               dt_offset, _hdr.dt_size, false, _reader);
        if (ret != RET_OK) { return ret; }
    }

    return RET_OK;
}

int LokiFormatReader::read_entry(File &file, Entry &entry)
{
    return _seg.read_entry(file, entry, _reader);
}

int LokiFormatReader::go_to_entry(File &file, Entry &entry, int entry_type)
{
    return _seg.go_to_entry(file, entry, entry_type, _reader);
}

int LokiFormatReader::read_data(File &file, void *buf, size_t buf_size,
                                size_t &bytes_read)
{
    return _seg.read_data(file, buf, buf_size, bytes_read, _reader);
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
 *   * #RET_OK if the header is found
 *   * #RET_WARN if the header is not found
 *   * #RET_FAILED if any file operation fails non-fatally
 *   * #RET_FATAL if any file operation fails fatally
 */
int LokiFormatReader::find_loki_header(Reader &reader, File &file,
                                       LokiHeader &header_out,
                                       uint64_t &offset_out)
{
    LokiHeader header;
    size_t n;

    if (!file.seek(LOKI_MAGIC_OFFSET, SEEK_SET, nullptr)) {
        reader.set_error(file.error(),
                         "Loki magic not found: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_WARN;
    }

    if (!file_read_fully(file, &header, sizeof(header), n)) {
        reader.set_error(file.error(),
                         "Failed to read header: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    } else if (n != sizeof(header)) {
        reader.set_error(make_error_code(LokiError::LokiHeaderTooSmall),
                         "Too small to be Loki image");
        return RET_WARN;
    }

    if (memcmp(header.magic, LOKI_MAGIC, LOKI_MAGIC_SIZE) != 0) {
        reader.set_error(make_error_code(LokiError::InvalidLokiMagic));
        return RET_WARN;
    }

    loki_fix_header_byte_order(header);
    header_out = header;
    offset_out = LOKI_MAGIC_OFFSET;

    return RET_OK;
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
 *   * #RET_OK if the ramdisk address is found
 *   * #RET_WARN if the ramdisk address is not found
 *   * #RET_FAILED if any file operation fails non-fatally
 *   * #RET_FATAL if any file operation fails fatally
 */
int LokiFormatReader::find_ramdisk_address(Reader &reader, File &file,
                                           const android::AndroidHeader &hdr,
                                           const LokiHeader &loki_hdr,
                                           uint32_t &ramdisk_addr_out)
{
    // If the boot image was patched with a newer version of loki, find the
    // ramdisk offset in the shell code
    uint32_t ramdisk_addr = 0;
    size_t n;

    if (loki_hdr.ramdisk_addr != 0) {
        uint64_t offset = 0;

        auto result_cb = [](File &file_, void *userdata, uint64_t offset_)
                -> FileSearchAction {
            (void) file_;
            auto offset_ptr = static_cast<uint64_t *>(userdata);
            *offset_ptr = offset_;
            return FileSearchAction::Continue;
        };

        if (!file_search(file, -1, -1, 0, LOKI_SHELLCODE,
                         LOKI_SHELLCODE_SIZE - 9, 1, result_cb, &offset)) {
            reader.set_error(file.error(),
                             "Failed to search for Loki shellcode: %s",
                             file.error_string().c_str());
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        }

        if (offset == 0) {
            reader.set_error(make_error_code(LokiError::ShellcodeNotFound));
            return RET_WARN;
        }

        offset += LOKI_SHELLCODE_SIZE - 5;

        if (!file.seek(static_cast<int64_t>(offset), SEEK_SET, nullptr)) {
            reader.set_error(file.error(),
                             "Failed to seek to ramdisk address offset: %s",
                             file.error_string().c_str());
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        }

        if (!file_read_fully(file, &ramdisk_addr, sizeof(ramdisk_addr), n)) {
            reader.set_error(file.error(),
                             "Failed to read ramdisk address offset: %s",
                             file.error_string().c_str());
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        } else if (n != sizeof(ramdisk_addr)) {
            reader.set_error(make_error_code(LokiError::UnexpectedEndOfFile),
                             "Unexpected EOF when reading ramdisk address");
            return RET_WARN;
        }

        ramdisk_addr = mb_le32toh(ramdisk_addr);
    } else {
        // Otherwise, use the default for jflte (- 0x00008000 + 0x02000000)

        if (hdr.kernel_addr > UINT32_MAX - 0x01ff8000) {
            reader.set_error(make_error_code(LokiError::InvalidKernelAddress),
                             "Invalid kernel address: %" PRIu32,
                             hdr.kernel_addr);
            return RET_WARN;
        }

        ramdisk_addr = hdr.kernel_addr + 0x01ff8000;
    }

    ramdisk_addr_out = ramdisk_addr;
    return RET_OK;
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
 *   * #RET_OK if a gzip offset is found
 *   * #RET_WARN if no gzip offsets are found
 *   * #RET_FAILED if any file operation fails non-fatally
 *   * #RET_FATAL if any file operation fails fatally
 */
int LokiFormatReader::find_gzip_offset_old(Reader &reader, File &file,
                                           uint32_t start_offset,
                                           uint64_t &gzip_offset_out)
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

    // Find first result with flags == 0x00 and flags == 0x08
    auto result_cb = [](File &file_, void *userdata, uint64_t offset)
            -> FileSearchAction {
        auto result_ = static_cast<SearchResult *>(userdata);
        uint64_t orig_offset;
        unsigned char flags;
        size_t n;

        // Stop early if possible
        if (result_->have_flag0 && result_->have_flag8) {
            return FileSearchAction::Stop;
        }

        // Save original position
        if (!file_.seek(0, SEEK_CUR, &orig_offset)) {
            return FileSearchAction::Fail;
        }

        // Seek to flags byte
        if (!file_.seek(static_cast<int64_t>(offset + 3), SEEK_SET, nullptr)) {
            return FileSearchAction::Fail;
        }

        // Read next bytes for flags
        if (!file_read_fully(file_, &flags, sizeof(flags), n)) {
            return FileSearchAction::Fail;
        } else if (n != sizeof(flags)) {
            // EOF
            return FileSearchAction::Stop;
        }

        if (!result_->have_flag0 && flags == 0x00) {
            result_->have_flag0 = true;
            result_->flag0_offset = offset;
        } else if (!result_->have_flag8 && flags == 0x08) {
            result_->have_flag8 = true;
            result_->flag8_offset = offset;
        }

        // Restore original position as per contract
        if (!file_.seek(static_cast<int64_t>(orig_offset), SEEK_SET, nullptr)) {
            return FileSearchAction::Fail;
        }

        return FileSearchAction::Continue;
    };

    if (!file_search(file, start_offset, -1, 0, gzip_deflate_magic,
                     sizeof(gzip_deflate_magic), -1, result_cb, &result)) {
        reader.set_error(file.error(),
                         "Failed to search for gzip magic: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    // Prefer gzip header with original filename flag since most loki'd boot
    // images will have been compressed manually with the gzip tool
    if (result.have_flag8) {
        gzip_offset_out = result.flag8_offset;
    } else if (result.have_flag0) {
        gzip_offset_out = result.flag0_offset;
    } else {
        reader.set_error(make_error_code(LokiError::NoRamdiskGzipHeaderFound));
        return RET_WARN;
    }

    return RET_OK;
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
 *   * #RET_OK if the ramdisk size is found
 *   * #RET_WARN if the ramdisk size is not found
 *   * #RET_FAILED if any file operation fails non-fatally
 *   * #RET_FATAL if any file operation fails fatally
 */
int LokiFormatReader::find_ramdisk_size_old(Reader &reader, File &file,
                                            const android::AndroidHeader &hdr,
                                            uint32_t ramdisk_offset,
                                            uint32_t &ramdisk_size_out)
{
    int32_t aboot_size;
    uint64_t aboot_offset;
#if 0
    uint64_t cur_offset;
    char buf[1024];
    size_t n;
#endif

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

    if (!file.seek(-aboot_size, SEEK_END, &aboot_offset)) {
        reader.set_error(file.error(),
                         "Failed to seek to end of file: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    if (ramdisk_offset > aboot_offset) {
        reader.set_error(make_error_code(
                LokiError::RamdiskOffsetGreaterThanAbootOffset));
        return RET_FAILED;
    }

    // Ignore zero padding as we might strip away too much
#if 1
    ramdisk_size_out = static_cast<uint32_t>(aboot_offset - ramdisk_offset);
    return RET_OK;
#else
    // Search backwards to find non-zero byte
    cur_offset = aboot_offset;

    while (cur_offset > ramdisk_offset) {
        size_t to_read = std::min<uint64_t>(
                sizeof(buf), cur_offset - ramdisk_offset);
        cur_offset -= to_read;

        if (!file.seek(cur_offset, SEEK_SET, nullptr)) {
            reader.set_error(file.error(),
                             "Failed to seek: %s",
                             file.error_string().c_str());
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        }

        if (!file_read_fully(file, buf, to_read, n)) {
            reader.set_error(file.error(),
                             "Failed to read: %s",
                             file.error_string().c_str());
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        } else if (n != to_read) {
            reader.set_error(make_error_code(
                    LokiError::UnexpectedFileTruncation));
            return RET_FAILED;
        }

        for (size_t i = n; i-- > 0; ) {
            if (buf[i] != '\0') {
                ramdisk_size_out = cur_offset - ramdisk_offset + i;
                return RET_OK;
            }
        }
    }

    reader.set_error(make_error_code(LokiError::FailedToDetermineRamdiskSize));
    return RET_WARN;
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
 *   * #RET_OK if the kernel size is found
 *   * #RET_WARN if the kernel size cannot be found
 *   * #RET_FAILED if any file operation fails non-fatally
 *   * #RET_FATAL if any file operation fails fatally
 */
int LokiFormatReader::find_linux_kernel_size(Reader &reader, File &file,
                                             uint32_t kernel_offset,
                                             uint32_t &kernel_size_out)
{
    size_t n;
    uint32_t kernel_size;

    // If the boot image was patched with an early version of loki, the
    // original kernel size is not stored in the loki header properly (or in the
    // shellcode). The size is stored in the kernel image's header though, so
    // we'll use that.
    // http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html#d0e309
    if (!file.seek(kernel_offset + 0x2c, SEEK_SET, nullptr)) {
        reader.set_error(file.error(),
                         "Failed to seek to kernel header: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    if (!file_read_fully(file, &kernel_size, sizeof(kernel_size), n)) {
        reader.set_error(file.error(),
                         "Failed to read size from kernel header: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    } else if (n != sizeof(kernel_size)) {
        reader.set_error(make_error_code(LokiError::UnexpectedEndOfFile),
                         "Unexpected EOF when reading kernel header");
        return RET_WARN;
    }

    kernel_size_out = mb_le32toh(kernel_size);
    return RET_OK;
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
 * \return
 *   * #RET_OK if the header is successfully read
 *   * #RET_WARN if parts of the header are missing or invalid
 *   * #RET_FAILED if any file operation fails non-fatally
 *   * #RET_FATAL if any file operation fails fatally
 */
int LokiFormatReader::read_header_old(Reader &reader, File &file,
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
    int ret;

    if (hdr.page_size == 0) {
        reader.set_error(make_error_code(LokiError::PageSizeCannotBeZero));
        return RET_WARN;
    }

    // The kernel tags address is invalid in the old loki images, so use the
    // default for jflte
    tags_addr = hdr.kernel_addr - android::DEFAULT_KERNEL_OFFSET
            + android::DEFAULT_TAGS_OFFSET;

    // Try to guess kernel size
    ret = find_linux_kernel_size(reader, file, hdr.page_size, kernel_size);
    if (ret != RET_OK) {
        return ret;
    }

    // Look for gzip offset for the ramdisk
    ret = find_gzip_offset_old(
            reader, file, hdr.page_size + kernel_size
                    + align_page_size<uint32_t>(kernel_size, hdr.page_size),
            gzip_offset);
    if (ret != RET_OK) {
        return ret;
    }

    // Try to guess ramdisk size
    ret = find_ramdisk_size_old(reader, file, hdr,
                                static_cast<uint32_t>(gzip_offset),
                                ramdisk_size);
    if (ret != RET_OK) {
        return ret;
    }

    // Guess original ramdisk address
    ret = find_ramdisk_address(reader, file, hdr, loki_hdr, ramdisk_addr);
    if (ret != RET_OK) {
        return ret;
    }

    kernel_size_out = kernel_size;
    ramdisk_size_out = ramdisk_size;

    char board_name[sizeof(hdr.name) + 1];
    char cmdline[sizeof(hdr.cmdline) + 1];

    strncpy(board_name, reinterpret_cast<const char *>(hdr.name),
            sizeof(hdr.name));
    strncpy(cmdline, reinterpret_cast<const char *>(hdr.cmdline),
            sizeof(hdr.cmdline));
    board_name[sizeof(hdr.name)] = '\0';
    cmdline[sizeof(hdr.cmdline)] = '\0';

    header.set_supported_fields(OLD_SUPPORTED_FIELDS);

    if (!header.set_board_name({board_name})
            || !header.set_kernel_cmdline({cmdline})
            || !header.set_page_size(hdr.page_size)
            || !header.set_kernel_address(hdr.kernel_addr)
            || !header.set_ramdisk_address(ramdisk_addr)
            || !header.set_secondboot_address(hdr.second_addr)
            || !header.set_kernel_tags_address(tags_addr)) {
        return RET_UNSUPPORTED;
    }

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

    return RET_OK;
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
 * \return
 *   * #RET_OK if the header is successfully read
 *   * #RET_WARN if parts of the header are missing or invalid
 *   * #RET_FAILED if any file operation fails non-fatally
 *   * #RET_FATAL if any file operation fails fatally
 */
int LokiFormatReader::read_header_new(Reader &reader, File &file,
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
    int ret;

    if (hdr.page_size == 0) {
        reader.set_error(make_error_code(LokiError::PageSizeCannotBeZero));
        return RET_WARN;
    }

    if (is_lg_ramdisk_address(hdr.ramdisk_addr)) {
        fake_size = hdr.page_size;
    } else {
        fake_size = 0x200;
    }

    // Find original ramdisk address
    ret = find_ramdisk_address(reader, file, hdr, loki_hdr, ramdisk_addr);
    if (ret != RET_OK) {
        return ret;
    }

    // Restore original values in boot image header
    kernel_size_out = loki_hdr.orig_kernel_size;
    ramdisk_size_out = loki_hdr.orig_ramdisk_size;

    char board_name[sizeof(hdr.name) + 1];
    char cmdline[sizeof(hdr.cmdline) + 1];

    strncpy(board_name, reinterpret_cast<const char *>(hdr.name),
            sizeof(hdr.name));
    strncpy(cmdline, reinterpret_cast<const char *>(hdr.cmdline),
            sizeof(hdr.cmdline));
    board_name[sizeof(hdr.name)] = '\0';
    cmdline[sizeof(hdr.cmdline)] = '\0';

    header.set_supported_fields(NEW_SUPPORTED_FIELDS);

    if (!header.set_board_name({board_name})
            || !header.set_kernel_cmdline({cmdline})
            || !header.set_page_size(hdr.page_size)
            || !header.set_kernel_address(hdr.kernel_addr)
            || !header.set_ramdisk_address(ramdisk_addr)
            || !header.set_secondboot_address(hdr.second_addr)
            || !header.set_kernel_tags_address(hdr.tags_addr)) {
        return RET_UNSUPPORTED;
    }

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

    return RET_OK;
}

}

/*!
 * \brief Enable support for Loki boot image format
 *
 * \return
 *   * #RET_OK if the format is successfully enabled
 *   * #RET_WARN if the format is already enabled
 *   * \<= #RET_FAILED if an error occurs
 */
int Reader::enable_format_loki()
{
    using namespace loki;

    MB_PRIVATE(Reader);

    std::unique_ptr<FormatReader> format{new LokiFormatReader(*this)};
    return priv->register_format(std::move(format));
}

}
}
