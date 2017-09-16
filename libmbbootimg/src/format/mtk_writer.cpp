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

#include "mbbootimg/format/mtk_writer_p.h"

#include <algorithm>

#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstring>

#include <openssl/sha.h>

#include "mbcommon/endian.h"
#include "mbcommon/file.h"
#include "mbcommon/file_util.h"
#include "mbcommon/string.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/format/android_error.h"
#include "mbbootimg/format/mtk_error.h"
#include "mbbootimg/header.h"
#include "mbbootimg/writer.h"
#include "mbbootimg/writer_p.h"


namespace mb
{
namespace bootimg
{
namespace mtk
{

static int _mtk_header_update_size(Writer &writer, File &file,
                                   uint64_t offset, uint32_t size)
{
    uint32_t le32_size = mb_htole32(size);
    size_t n;

    if (offset > SIZE_MAX - offsetof(MtkHeader, size)) {
        writer.set_error(make_error_code(MtkError::MtkHeaderOffsetTooLarge));
        return RET_FATAL;
    }

    if (!file.seek(static_cast<int64_t>(offset + offsetof(MtkHeader, size)),
                   SEEK_SET, nullptr)) {
        writer.set_error(file.error(),
                         "Failed to seek to MTK size field: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    if (!file_write_fully(file, &le32_size, sizeof(le32_size), n)) {
        writer.set_error(file.error(),
                         "Failed to write MTK size field: %s",
                         file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    } else if (n != sizeof(le32_size)) {
        writer.set_error(file.error(),
                         "Unexpected EOF when writing MTK size field");
        return RET_FAILED;
    }

    return RET_OK;
}

static int _mtk_compute_sha1(Writer &writer, SegmentWriter &seg,
                             File &file,
                             unsigned char digest[SHA_DIGEST_LENGTH])
{
    SHA_CTX sha_ctx;
    char buf[10240];
    size_t n;

    uint32_t kernel_mtkhdr_size = 0;
    uint32_t ramdisk_mtkhdr_size = 0;

    if (!SHA1_Init(&sha_ctx)) {
        writer.set_error(make_error_code(
                android::AndroidError::Sha1InitError));
        return RET_FAILED;
    }

    for (size_t i = 0; i < seg.entries_size(); ++i) {
        auto const *entry = seg.entries_get(i);
        uint64_t remain = entry->size;

        if (!file.seek(static_cast<int64_t>(entry->offset), SEEK_SET,
                       nullptr)) {
            writer.set_error(file.error(),
                             "Failed to seek to entry %" MB_PRIzu ": %s",
                             i, file.error_string().c_str());
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        }

        // Update checksum with data
        while (remain > 0) {
            size_t to_read = std::min<uint64_t>(remain, sizeof(buf));

            if (!file_read_fully(file, buf, to_read, n)) {
                writer.set_error(file.error(),
                                 "Failed to read entry %" MB_PRIzu ": %s",
                                 i, file.error_string().c_str());
                return file.is_fatal() ? RET_FATAL : RET_FAILED;
            } else if (n != to_read) {
                writer.set_error(file.error(),
                                 "Unexpected EOF when reading entry");
                return RET_FAILED;
            }

            if (!SHA1_Update(&sha_ctx, buf, n)) {
                writer.set_error(make_error_code(
                        android::AndroidError::Sha1UpdateError));
                return RET_FAILED;
            }

            remain -= to_read;
        }

        uint32_t le32_size;

        // Update checksum with size
        switch (entry->type) {
        case ENTRY_TYPE_MTK_KERNEL_HEADER:
            kernel_mtkhdr_size = entry->size;
            continue;
        case ENTRY_TYPE_MTK_RAMDISK_HEADER:
            ramdisk_mtkhdr_size = entry->size;
            continue;
        case ENTRY_TYPE_KERNEL:
            le32_size = mb_htole32(entry->size + kernel_mtkhdr_size);
            break;
        case ENTRY_TYPE_RAMDISK:
            le32_size = mb_htole32(entry->size + ramdisk_mtkhdr_size);
            break;
        case ENTRY_TYPE_SECONDBOOT:
            le32_size = mb_htole32(entry->size);
            break;
        case ENTRY_TYPE_DEVICE_TREE:
            if (entry->size == 0) {
                continue;
            }
            le32_size = mb_htole32(entry->size);
            break;
        default:
            continue;
        }

        if (!SHA1_Update(&sha_ctx, &le32_size, sizeof(le32_size))) {
            writer.set_error(make_error_code(
                    android::AndroidError::Sha1UpdateError));
            return RET_FAILED;
        }
    }

    if (!SHA1_Final(digest, &sha_ctx)) {
        writer.set_error(make_error_code(
                android::AndroidError::Sha1UpdateError));
        return RET_FATAL;
    }

    return RET_OK;
}

MtkFormatWriter::MtkFormatWriter(Writer &writer)
    : FormatWriter(writer)
    , _hdr()
{
}

MtkFormatWriter::~MtkFormatWriter() = default;

int MtkFormatWriter::type()
{
    return FORMAT_MTK;
}

std::string MtkFormatWriter::name()
{
    return FORMAT_NAME_MTK;
}

int MtkFormatWriter::get_header(File &file, Header &header)
{
    (void) file;

    header.set_supported_fields(SUPPORTED_FIELDS);

    return RET_OK;
}

int MtkFormatWriter::write_header(File &file, const Header &header)
{
    int ret;

    // Construct header
    memset(&_hdr, 0, sizeof(_hdr));
    memcpy(_hdr.magic, android::BOOT_MAGIC, android::BOOT_MAGIC_SIZE);

    if (auto address = header.kernel_address()) {
        _hdr.kernel_addr = *address;
    }
    if (auto address = header.ramdisk_address()) {
        _hdr.ramdisk_addr = *address;
    }
    if (auto address = header.secondboot_address()) {
        _hdr.second_addr = *address;
    }
    if (auto address = header.kernel_tags_address()) {
        _hdr.tags_addr = *address;
    }
    if (auto page_size = header.page_size()) {
        switch (*page_size) {
        case 2048:
        case 4096:
        case 8192:
        case 16384:
        case 32768:
        case 65536:
        case 131072:
            _hdr.page_size = *page_size;
            break;
        default:
            _writer.set_error(make_error_code(android::AndroidError::InvalidPageSize),
                              "Invalid page size: %" PRIu32, *page_size);
            return RET_FAILED;
        }
    } else {
        _writer.set_error(make_error_code(
                android::AndroidError::MissingPageSize));
        return RET_FAILED;
    }

    if (auto board_name = header.board_name()) {
        if (board_name->size() >= sizeof(_hdr.name)) {
            _writer.set_error(make_error_code(
                    android::AndroidError::BoardNameTooLong));
            return RET_FAILED;
        }

        strncpy(reinterpret_cast<char *>(_hdr.name), board_name->c_str(),
                sizeof(_hdr.name) - 1);
        _hdr.name[sizeof(_hdr.name) - 1] = '\0';
    }
    if (auto cmdline = header.kernel_cmdline()) {
        if (cmdline->size() >= sizeof(_hdr.cmdline)) {
            _writer.set_error(make_error_code(
                    android::AndroidError::KernelCmdlineTooLong));
            return RET_FAILED;
        }

        strncpy(reinterpret_cast<char *>(_hdr.cmdline), cmdline->c_str(),
                sizeof(_hdr.cmdline) - 1);
        _hdr.cmdline[sizeof(_hdr.cmdline) - 1] = '\0';
    }

    // TODO: UNUSED
    // TODO: ID

    // Clear existing entries (none should exist unless this function fails and
    // the user reattempts to call it)
    _seg.entries_clear();

    ret = _seg.entries_add(ENTRY_TYPE_MTK_KERNEL_HEADER,
                           0, false, 0, _writer);
    if (ret != RET_OK) { return ret; }

    ret = _seg.entries_add(ENTRY_TYPE_KERNEL,
                           0, false, _hdr.page_size, _writer);
    if (ret != RET_OK) { return ret; }

    ret = _seg.entries_add(ENTRY_TYPE_MTK_RAMDISK_HEADER,
                           0, false, 0, _writer);
    if (ret != RET_OK) { return ret; }

    ret = _seg.entries_add(ENTRY_TYPE_RAMDISK,
                           0, false, _hdr.page_size, _writer);
    if (ret != RET_OK) { return ret; }

    ret = _seg.entries_add(ENTRY_TYPE_SECONDBOOT,
                           0, false, _hdr.page_size, _writer);
    if (ret != RET_OK) { return ret; }

    ret = _seg.entries_add(ENTRY_TYPE_DEVICE_TREE,
                           0, false, _hdr.page_size, _writer);
    if (ret != RET_OK) { return ret; }

    // Start writing after first page
    if (!file.seek(_hdr.page_size, SEEK_SET, nullptr)) {
        _writer.set_error(file.error(),
                          "Failed to seek to first page: %s",
                          file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    return RET_OK;
}

int MtkFormatWriter::get_entry(File &file, Entry &entry)
{
    return _seg.get_entry(file, entry, _writer);
}

int MtkFormatWriter::write_entry(File &file, const Entry &entry)
{
    return _seg.write_entry(file, entry, _writer);
}

int MtkFormatWriter::write_data(File &file, const void *buf, size_t buf_size,
                                size_t &bytes_written)
{
    return _seg.write_data(file, buf, buf_size, bytes_written, _writer);
}

int MtkFormatWriter::finish_entry(File &file)
{
    int ret;

    ret = _seg.finish_entry(file, _writer);
    if (ret != RET_OK) {
        return ret;
    }

    auto const *swentry = _seg.entry();

    if ((swentry->type == ENTRY_TYPE_KERNEL
            || swentry->type == ENTRY_TYPE_RAMDISK)
            && swentry->size == UINT32_MAX - sizeof(MtkHeader)) {
        _writer.set_error(make_error_code(
                MtkError::EntryTooLargeToFitMtkHeader));
        return RET_FATAL;
    } else if ((swentry->type == ENTRY_TYPE_MTK_KERNEL_HEADER
            || swentry->type == ENTRY_TYPE_MTK_RAMDISK_HEADER)
            && swentry->size != sizeof(MtkHeader)) {
        _writer.set_error(make_error_code(
                MtkError::InvalidEntrySizeForMtkHeader));
        return RET_FATAL;
    }

    switch (swentry->type) {
    case ENTRY_TYPE_KERNEL:
        _hdr.kernel_size = static_cast<uint32_t>(
                swentry->size + sizeof(MtkHeader));
        break;
    case ENTRY_TYPE_RAMDISK:
        _hdr.ramdisk_size = static_cast<uint32_t>(
                swentry->size + sizeof(MtkHeader));
        break;
    case ENTRY_TYPE_SECONDBOOT:
        _hdr.second_size = swentry->size;
        break;
    case ENTRY_TYPE_DEVICE_TREE:
        _hdr.dt_size = swentry->size;
        break;
    }

    return RET_OK;
}

int MtkFormatWriter::close(File &file)
{
    int ret;
    size_t n;

    if (!_file_size) {
        uint64_t file_size;
        if (!file.seek(0, SEEK_CUR, &file_size)) {
            _writer.set_error(file.error(),
                              "Failed to get file offset: %s",
                              file.error_string().c_str());
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        }

        _file_size = file_size;
    }

    auto const *swentry = _seg.entry();

    // If successful, finish up the boot image
    if (!swentry) {
        // Truncate to set size
        if (!file.truncate(*_file_size)) {
            _writer.set_error(file.error(),
                              "Failed to truncate file: %s",
                              file.error_string().c_str());
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        }

        // Update MTK header sizes
        for (size_t i = 0; i < _seg.entries_size(); ++i) {
            auto const *entry = _seg.entries_get(i);
            switch (entry->type) {
            case ENTRY_TYPE_MTK_KERNEL_HEADER:
                ret = _mtk_header_update_size(
                        _writer, file, entry->offset,
                        static_cast<uint32_t>(
                                _hdr.kernel_size - sizeof(MtkHeader)));
                break;
            case ENTRY_TYPE_MTK_RAMDISK_HEADER:
                ret = _mtk_header_update_size(
                        _writer, file, entry->offset,
                        static_cast<uint32_t>(
                                _hdr.ramdisk_size - sizeof(MtkHeader)));
                break;
            default:
                continue;
            }

            if (ret != RET_OK) {
                return ret;
            }
        }

        // We need to take the performance hit and compute the SHA1 here.
        // We can't fill in the sizes in the MTK headers when we're writing
        // them. Thus, if we calculated the SHA1sum during write, it would be
        // incorrect.
        ret = _mtk_compute_sha1(_writer, _seg, file,
                                reinterpret_cast<unsigned char *>(_hdr.id));
        if (ret != RET_OK) {
            return ret;
        }

        // Convert fields back to little-endian
        android::AndroidHeader hdr = _hdr;
        android_fix_header_byte_order(hdr);

        // Seek back to beginning to write header
        if (!file.seek(0, SEEK_SET, nullptr)) {
            _writer.set_error(file.error(),
                              "Failed to seek to beginning: %s",
                              file.error_string().c_str());
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        }

        // Write header
        if (!file_write_fully(file, &hdr, sizeof(hdr), n) || n != sizeof(hdr)) {
            _writer.set_error(file.error(),
                              "Failed to write header: %s",
                              file.error_string().c_str());
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        }
    }

    return RET_OK;
}

}

/*!
 * \brief Set MTK boot image output format
 *
 * \return
 *   * #RET_OK if the format is successfully set
 *   * \<= #RET_WARN if an error occurs
 */
int Writer::set_format_mtk()
{
    using namespace mtk;

    MB_PRIVATE(Writer);

    std::unique_ptr<FormatWriter> format{new MtkFormatWriter(*this)};
    return priv->register_format(std::move(format));
}

}
}
