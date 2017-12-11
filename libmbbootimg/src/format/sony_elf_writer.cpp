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

#include "mbbootimg/format/sony_elf_writer_p.h"

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
#include "mbbootimg/format/sony_elf_defs.h"
#include "mbbootimg/header.h"
#include "mbbootimg/writer.h"
#include "mbbootimg/writer_p.h"

namespace mb
{
namespace bootimg
{
namespace sonyelf
{

constexpr int SONY_ELF_ENTRY_CMDLINE = -1;

SonyElfFormatWriter::SonyElfFormatWriter(Writer &writer)
    : FormatWriter(writer)
    , m_hdr()
    , m_hdr_kernel()
    , m_hdr_ramdisk()
    , m_hdr_cmdline()
    , m_hdr_ipl()
    , m_hdr_rpm()
    , m_hdr_appsbl()
{
}

SonyElfFormatWriter::~SonyElfFormatWriter() = default;

int SonyElfFormatWriter::type()
{
    return FORMAT_SONY_ELF;
}

std::string SonyElfFormatWriter::name()
{
    return FORMAT_NAME_SONY_ELF;
}

oc::result<void> SonyElfFormatWriter::open(File &file)
{
    (void) file;

    m_seg = SegmentWriter();

    return oc::success();
}

oc::result<void> SonyElfFormatWriter::close(File &file)
{
    auto reset_state = finally([&] {
        m_hdr = {};
        m_hdr_kernel = {};
        m_hdr_ramdisk = {};
        m_hdr_cmdline = {};
        m_hdr_ipl = {};
        m_hdr_rpm = {};
        m_hdr_appsbl = {};
        m_cmdline.clear();
        m_seg = {};
    });

    if (m_writer.is_open()) {
        auto swentry = m_seg->entry();

        // If successful, finish up the boot image
        if (swentry == m_seg->entries().end()) {
            // Write headers
            struct {
                const void *ptr;
                size_t size;
                bool can_write;
            } headers[] = {
                { &m_hdr, sizeof(m_hdr), true },
                { &m_hdr_kernel, sizeof(m_hdr_kernel), m_hdr_kernel.p_filesz > 0 },
                { &m_hdr_ramdisk, sizeof(m_hdr_ramdisk), m_hdr_ramdisk.p_filesz > 0 },
                { &m_hdr_cmdline, sizeof(m_hdr_cmdline), m_hdr_cmdline.p_filesz > 0 },
                { &m_hdr_ipl, sizeof(m_hdr_ipl), m_hdr_ipl.p_filesz > 0 },
                { &m_hdr_rpm, sizeof(m_hdr_rpm), m_hdr_rpm.p_filesz > 0 },
                { &m_hdr_appsbl, sizeof(m_hdr_appsbl), m_hdr_appsbl.p_filesz > 0 },
            };

            sony_elf_fix_ehdr_byte_order(m_hdr);
            sony_elf_fix_phdr_byte_order(m_hdr_kernel);
            sony_elf_fix_phdr_byte_order(m_hdr_ramdisk);
            sony_elf_fix_phdr_byte_order(m_hdr_cmdline);
            sony_elf_fix_phdr_byte_order(m_hdr_ipl);
            sony_elf_fix_phdr_byte_order(m_hdr_rpm);
            sony_elf_fix_phdr_byte_order(m_hdr_appsbl);

            // Seek back to beginning to write headers
            auto seek_ret = file.seek(0, SEEK_SET);
            if (!seek_ret) {
                if (file.is_fatal()) { m_writer.set_fatal(); }
                return seek_ret.as_failure();
            }

            // Write headers
            for (auto const &header : headers) {
                if (header.can_write) {
                    auto ret = file_write_exact(file, header.ptr, header.size);
                    if (!ret) {
                        if (file.is_fatal()) { m_writer.set_fatal(); }
                        return ret.as_failure();
                    }
                }
            }
        }
    }

    return oc::success();
}

oc::result<void> SonyElfFormatWriter::get_header(File &file, Header &header)
{
    (void) file;

    header.set_supported_fields(SUPPORTED_FIELDS);

    return oc::success();
}

oc::result<void> SonyElfFormatWriter::write_header(File &file,
                                                   const Header &header)
{
    m_cmdline.clear();

    m_hdr = {};
    m_hdr_kernel = {};
    m_hdr_ramdisk = {};
    m_hdr_cmdline = {};
    m_hdr_ipl = {};
    m_hdr_rpm = {};
    m_hdr_appsbl = {};

    // Construct ELF header
    memcpy(&m_hdr.e_ident, SONY_E_IDENT, SONY_EI_NIDENT);
    m_hdr.e_type = 2;
    m_hdr.e_machine = 40;
    m_hdr.e_version = 1;
    m_hdr.e_phoff = 52;
    m_hdr.e_shoff = 0;
    m_hdr.e_flags = 0;
    m_hdr.e_ehsize = sizeof(Sony_Elf32_Ehdr);
    m_hdr.e_phentsize = sizeof(Sony_Elf32_Phdr);
    m_hdr.e_shentsize = 0;
    m_hdr.e_shnum = 0;
    m_hdr.e_shstrndx = 0;

    if (auto entrypoint_address = header.entrypoint_address()) {
        m_hdr.e_entry = *entrypoint_address;
    } else if (auto kernel_address = header.kernel_address()) {
        m_hdr.e_entry = *kernel_address;
    }

    // Construct kernel program header
    m_hdr_kernel.p_type = SONY_E_TYPE_KERNEL;
    m_hdr_kernel.p_flags = SONY_E_FLAGS_KERNEL;
    m_hdr_kernel.p_align = 0;

    if (auto address = header.kernel_address()) {
        m_hdr_kernel.p_vaddr = *address;
        m_hdr_kernel.p_paddr = *address;
    }

    // Construct ramdisk program header
    m_hdr_ramdisk.p_type = SONY_E_TYPE_RAMDISK;
    m_hdr_ramdisk.p_flags = SONY_E_FLAGS_RAMDISK;
    m_hdr_ramdisk.p_align = 0;

    if (auto address = header.ramdisk_address()) {
        m_hdr_ramdisk.p_vaddr = *address;
        m_hdr_ramdisk.p_paddr = *address;
    }

    // Construct cmdline program header
    m_hdr_cmdline.p_type = SONY_E_TYPE_CMDLINE;
    m_hdr_cmdline.p_vaddr = 0;
    m_hdr_cmdline.p_paddr = 0;
    m_hdr_cmdline.p_flags = SONY_E_FLAGS_CMDLINE;
    m_hdr_cmdline.p_align = 0;

    if (auto cmdline = header.kernel_cmdline()) {
        m_cmdline = *cmdline;
    }

    // Construct IPL program header
    m_hdr_ipl.p_type = SONY_E_TYPE_IPL;
    m_hdr_ipl.p_flags = SONY_E_FLAGS_IPL;
    m_hdr_ipl.p_align = 0;

    if (auto address = header.sony_ipl_address()) {
        m_hdr_ipl.p_vaddr = *address;
        m_hdr_ipl.p_paddr = *address;
    }

    // Construct RPM program header
    m_hdr_rpm.p_type = SONY_E_TYPE_RPM;
    m_hdr_rpm.p_flags = SONY_E_FLAGS_RPM;
    m_hdr_rpm.p_align = 0;

    if (auto address = header.sony_rpm_address()) {
        m_hdr_rpm.p_vaddr = *address;
        m_hdr_rpm.p_paddr = *address;
    }

    // Construct APPSBL program header
    m_hdr_appsbl.p_type = SONY_E_TYPE_APPSBL;
    m_hdr_appsbl.p_flags = SONY_E_FLAGS_APPSBL;
    m_hdr_appsbl.p_align = 0;

    if (auto address = header.sony_appsbl_address()) {
        m_hdr_appsbl.p_vaddr = *address;
        m_hdr_appsbl.p_paddr = *address;
    }

    std::vector<SegmentWriterEntry> entries;

    entries.push_back({ ENTRY_TYPE_KERNEL, 0, {}, 0 });
    entries.push_back({ ENTRY_TYPE_RAMDISK, 0, {}, 0 });
    entries.push_back({ SONY_ELF_ENTRY_CMDLINE, 0, {}, 0 });
    entries.push_back({ ENTRY_TYPE_SONY_IPL, 0, {}, 0 });
    entries.push_back({ ENTRY_TYPE_SONY_RPM, 0, {}, 0 });
    entries.push_back({ ENTRY_TYPE_SONY_APPSBL, 0, {}, 0 });

    OUTCOME_TRYV(m_seg->set_entries(std::move(entries)));

    // Start writing at offset 4096
    auto seek_ret = file.seek(4096, SEEK_SET);
    if (!seek_ret) {
        if (file.is_fatal()) { m_writer.set_fatal(); }
        return seek_ret.as_failure();
    }

    return oc::success();
}

oc::result<void> SonyElfFormatWriter::get_entry(File &file, Entry &entry)
{
    OUTCOME_TRYV(m_seg->get_entry(file, entry, m_writer));

    auto swentry = m_seg->entry();

    // Silently handle cmdline entry
    if (swentry->type == SONY_ELF_ENTRY_CMDLINE) {
        entry.clear();

        entry.set_size(m_cmdline.size());

        auto set_as_fatal = finally([&] {
            m_writer.set_fatal();
        });

        OUTCOME_TRYV(write_entry(file, entry));
        OUTCOME_TRYV(write_data(file, m_cmdline.data(), m_cmdline.size()));
        OUTCOME_TRYV(finish_entry(file));
        OUTCOME_TRYV(get_entry(file, entry));

        set_as_fatal.dismiss();
    }

    return oc::success();
}

oc::result<void> SonyElfFormatWriter::write_entry(File &file,
                                                  const Entry &entry)
{
    return m_seg->write_entry(file, entry, m_writer);
}

oc::result<size_t> SonyElfFormatWriter::write_data(File &file, const void *buf,
                                                   size_t buf_size)
{
    return m_seg->write_data(file, buf, buf_size, m_writer);
}

oc::result<void> SonyElfFormatWriter::finish_entry(File &file)
{
    OUTCOME_TRYV(m_seg->finish_entry(file, m_writer));

    auto swentry = m_seg->entry();

    switch (swentry->type) {
    case ENTRY_TYPE_KERNEL:
        m_hdr_kernel.p_offset = static_cast<Elf32_Off>(swentry->offset);
        m_hdr_kernel.p_filesz = *swentry->size;
        m_hdr_kernel.p_memsz = *swentry->size;
        break;
    case ENTRY_TYPE_RAMDISK:
        m_hdr_ramdisk.p_offset = static_cast<Elf32_Off>(swentry->offset);
        m_hdr_ramdisk.p_filesz = *swentry->size;
        m_hdr_ramdisk.p_memsz = *swentry->size;
        break;
    case ENTRY_TYPE_SONY_IPL:
        m_hdr_ipl.p_offset = static_cast<Elf32_Off>(swentry->offset);
        m_hdr_ipl.p_filesz = *swentry->size;
        m_hdr_ipl.p_memsz = *swentry->size;
        break;
    case ENTRY_TYPE_SONY_RPM:
        m_hdr_rpm.p_offset = static_cast<Elf32_Off>(swentry->offset);
        m_hdr_rpm.p_filesz = *swentry->size;
        m_hdr_rpm.p_memsz = *swentry->size;
        break;
    case ENTRY_TYPE_SONY_APPSBL:
        m_hdr_appsbl.p_offset = static_cast<Elf32_Off>(swentry->offset);
        m_hdr_appsbl.p_filesz = *swentry->size;
        m_hdr_appsbl.p_memsz = *swentry->size;
        break;
    case SONY_ELF_ENTRY_CMDLINE:
        m_hdr_cmdline.p_offset = static_cast<Elf32_Off>(swentry->offset);
        m_hdr_cmdline.p_filesz = *swentry->size;
        m_hdr_cmdline.p_memsz = *swentry->size;
        break;
    }

    if (*swentry->size > 0) {
        ++m_hdr.e_phnum;
    }

    return oc::success();
}

}

/*!
 * \brief Set Sony ELF boot image output format
 *
 * \return Nothing if the format is successfully set. Otherwise, the error code.
 */
oc::result<void> Writer::set_format_sony_elf()
{
    return register_format(
            std::make_unique<sonyelf::SonyElfFormatWriter>(*this));
}

}
}
