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
#include "mbcommon/finally.h"

#include "mbbootimg/entry.h"
#include "mbbootimg/format/android_error.h"
#include "mbbootimg/format/mtk_error.h"
#include "mbbootimg/header.h"
#include "mbbootimg/writer_p.h"


namespace mb::bootimg::mtk
{

static oc::result<void>
_mtk_header_update_size(File &file, uint64_t offset, uint32_t size)
{
    uint32_t le32_size = mb_htole32(size);

    if (offset > SIZE_MAX - offsetof(MtkHeader, size)) {
        return MtkError::MtkHeaderOffsetTooLarge;
    }

    OUTCOME_TRYV(file.seek(
            static_cast<int64_t>(offset + offsetof(MtkHeader, size)), SEEK_SET));

    OUTCOME_TRYV(file_write_exact(file, &le32_size, sizeof(le32_size)));

    return oc::success();
}

static oc::result<void>
_mtk_compute_sha1(SegmentWriter &seg, File &file,
                  unsigned char digest[SHA_DIGEST_LENGTH])
{
    SHA_CTX sha_ctx;
    char buf[10240];

    uint32_t kernel_mtkhdr_size = 0;
    uint32_t ramdisk_mtkhdr_size = 0;

    if (!SHA1_Init(&sha_ctx)) {
        return android::AndroidError::Sha1InitError;
    }

    for (auto const &entry : seg.entries()) {
        uint64_t remain = *entry.size;

        OUTCOME_TRYV(file.seek(static_cast<int64_t>(entry.offset), SEEK_SET));

        // Update checksum with data
        while (remain > 0) {
            auto to_read = std::min<uint64_t>(remain, sizeof(buf));

            OUTCOME_TRYV(file_read_exact(file, buf, static_cast<size_t>(to_read)));

            if (!SHA1_Update(&sha_ctx, buf, static_cast<size_t>(to_read))) {
                return android::AndroidError::Sha1UpdateError;
            }

            remain -= to_read;
        }

        uint32_t le32_size;

        // Update checksum with size
        switch (entry.type) {
        case EntryType::MtkKernelHeader:
            kernel_mtkhdr_size = *entry.size;
            continue;
        case EntryType::MtkRamdiskHeader:
            ramdisk_mtkhdr_size = *entry.size;
            continue;
        case EntryType::Kernel:
            le32_size = mb_htole32(*entry.size + kernel_mtkhdr_size);
            break;
        case EntryType::Ramdisk:
            le32_size = mb_htole32(*entry.size + ramdisk_mtkhdr_size);
            break;
        case EntryType::SecondBoot:
            le32_size = mb_htole32(*entry.size);
            break;
        case EntryType::DeviceTree:
            if (*entry.size == 0) {
                continue;
            }
            le32_size = mb_htole32(*entry.size);
            break;
        default:
            continue;
        }

        if (!SHA1_Update(&sha_ctx, &le32_size, sizeof(le32_size))) {
            return android::AndroidError::Sha1UpdateError;
        }
    }

    if (!SHA1_Final(digest, &sha_ctx)) {
        return android::AndroidError::Sha1UpdateError;
    }

    return oc::success();
}

MtkFormatWriter::MtkFormatWriter() noexcept
    : FormatWriter()
    , m_hdr()
{
}

MtkFormatWriter::~MtkFormatWriter() noexcept = default;

Format MtkFormatWriter::type()
{
    return Format::Mtk;
}

oc::result<void> MtkFormatWriter::open(File &file)
{
    (void) file;

    m_seg = SegmentWriter();

    return oc::success();
}

oc::result<void> MtkFormatWriter::close(File &file)
{
    auto reset_state = finally([&] {
        m_hdr = {};
        m_seg = {};
    });

    if (m_seg) {
        auto swentry = m_seg->entry();

        // If successful, finish up the boot image
        if (swentry == m_seg->entries().end()) {
            OUTCOME_TRY(file_size, file.seek(0, SEEK_CUR));

            // Truncate to set size
            OUTCOME_TRYV(file.truncate(file_size));

            // Update MTK header sizes
            for (auto const &entry : m_seg->entries()) {
                if (entry.type == EntryType::MtkKernelHeader) {
                    OUTCOME_TRYV(_mtk_header_update_size(
                            file, entry.offset,
                            static_cast<uint32_t>(
                                    m_hdr.kernel_size - sizeof(MtkHeader))));
                } else if (entry.type == EntryType::MtkRamdiskHeader) {
                    OUTCOME_TRYV(_mtk_header_update_size(
                            file, entry.offset,
                            static_cast<uint32_t>(
                                    m_hdr.ramdisk_size - sizeof(MtkHeader))));
                }
            }

            // We need to take the performance hit and compute the SHA1 here.
            // We can't fill in the sizes in the MTK headers when we're writing
            // them. Thus, if we calculated the SHA1sum during write, it would
            // be incorrect.
            OUTCOME_TRYV(_mtk_compute_sha1(
                    *m_seg, file,
                    reinterpret_cast<unsigned char *>(m_hdr.id)));

            // Convert fields back to little-endian
            android_fix_header_byte_order(m_hdr);

            // Seek back to beginning to write header
            OUTCOME_TRYV(file.seek(0, SEEK_SET));

            // Write header
            OUTCOME_TRYV(file_write_exact(file, &m_hdr, sizeof(m_hdr)));
        }
    }

    return oc::success();
}

oc::result<Header> MtkFormatWriter::get_header(File &file)
{
    (void) file;

    Header header;
    header.set_supported_fields(SUPPORTED_FIELDS);

    return std::move(header);
}

oc::result<void> MtkFormatWriter::write_header(File &file, const Header &header)
{
    // Construct header
    m_hdr = {};
    memcpy(m_hdr.magic, android::BOOT_MAGIC, android::BOOT_MAGIC_SIZE);

    if (auto address = header.kernel_address()) {
        m_hdr.kernel_addr = *address;
    }
    if (auto address = header.ramdisk_address()) {
        m_hdr.ramdisk_addr = *address;
    }
    if (auto address = header.secondboot_address()) {
        m_hdr.second_addr = *address;
    }
    if (auto address = header.kernel_tags_address()) {
        m_hdr.tags_addr = *address;
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
            m_hdr.page_size = *page_size;
            break;
        default:
            //DEBUG("Invalid page size: %" PRIu32, *page_size);
            return android::AndroidError::InvalidPageSize;
        }
    } else {
        return android::AndroidError::MissingPageSize;
    }

    if (auto board_name = header.board_name()) {
        if (board_name->size() >= sizeof(m_hdr.name)) {
            return android::AndroidError::BoardNameTooLong;
        }

        strncpy(reinterpret_cast<char *>(m_hdr.name), board_name->c_str(),
                sizeof(m_hdr.name) - 1);
        m_hdr.name[sizeof(m_hdr.name) - 1] = '\0';
    }
    if (auto cmdline = header.kernel_cmdline()) {
        if (cmdline->size() >= sizeof(m_hdr.cmdline)) {
            return android::AndroidError::KernelCmdlineTooLong;
        }

        strncpy(reinterpret_cast<char *>(m_hdr.cmdline), cmdline->c_str(),
                sizeof(m_hdr.cmdline) - 1);
        m_hdr.cmdline[sizeof(m_hdr.cmdline) - 1] = '\0';
    }

    // TODO: UNUSED
    // TODO: ID

    std::vector<SegmentWriterEntry> entries;

    entries.push_back({ EntryType::MtkKernelHeader, 0, {}, 0 });
    entries.push_back({ EntryType::Kernel, 0, {}, m_hdr.page_size });
    entries.push_back({ EntryType::MtkRamdiskHeader, 0, {}, 0 });
    entries.push_back({ EntryType::Ramdisk, 0, {}, m_hdr.page_size });
    entries.push_back({ EntryType::SecondBoot, 0, {}, m_hdr.page_size });
    entries.push_back({ EntryType::DeviceTree, 0, {}, m_hdr.page_size });

    OUTCOME_TRYV(m_seg->set_entries(std::move(entries)));

    // Start writing after first page
    OUTCOME_TRYV(file.seek(m_hdr.page_size, SEEK_SET));

    return oc::success();
}

oc::result<Entry> MtkFormatWriter::get_entry(File &file)
{
    return m_seg->get_entry(file);
}

oc::result<void> MtkFormatWriter::write_entry(File &file, const Entry &entry)
{
    return m_seg->write_entry(file, entry);
}

oc::result<size_t>
MtkFormatWriter::write_data(File &file, const void *buf, size_t buf_size)
{
    return m_seg->write_data(file, buf, buf_size);
}

oc::result<void> MtkFormatWriter::finish_entry(File &file)
{
    OUTCOME_TRYV(m_seg->finish_entry(file));

    auto swentry = m_seg->entry();

    if ((swentry->type == EntryType::Kernel
            || swentry->type == EntryType::Ramdisk)
            && *swentry->size == UINT32_MAX - sizeof(MtkHeader)) {
        return MtkError::EntryTooLargeToFitMtkHeader;
    } else if ((swentry->type == EntryType::MtkKernelHeader
            || swentry->type == EntryType::MtkRamdiskHeader)
            && *swentry->size != sizeof(MtkHeader)) {
        return MtkError::InvalidEntrySizeForMtkHeader;
    }

    switch (swentry->type) {
    case EntryType::Kernel:
        m_hdr.kernel_size = static_cast<uint32_t>(
                *swentry->size + sizeof(MtkHeader));
        break;
    case EntryType::Ramdisk:
        m_hdr.ramdisk_size = static_cast<uint32_t>(
                *swentry->size + sizeof(MtkHeader));
        break;
    case EntryType::SecondBoot:
        m_hdr.second_size = *swentry->size;
        break;
    case EntryType::DeviceTree:
        m_hdr.dt_size = *swentry->size;
        break;
    default:
        break;
    }

    return oc::success();
}

}
