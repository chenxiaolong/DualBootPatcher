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
#include "mbcommon/string.h"

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
    , _hdr()
    , _hdr_kernel()
    , _hdr_ramdisk()
    , _hdr_cmdline()
    , _hdr_ipl()
    , _hdr_rpm()
    , _hdr_appsbl()
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

int SonyElfFormatWriter::get_header(File &file, Header &header)
{
    (void) file;

    header.set_supported_fields(SUPPORTED_FIELDS);

    return RET_OK;
}

int SonyElfFormatWriter::write_header(File &file, const Header &header)
{
    int ret;

    _cmdline.clear();

    _hdr = {};
    _hdr_kernel = {};
    _hdr_ramdisk = {};
    _hdr_cmdline = {};
    _hdr_ipl = {};
    _hdr_rpm = {};
    _hdr_appsbl = {};

    // Construct ELF header
    memcpy(&_hdr.e_ident, SONY_E_IDENT, SONY_EI_NIDENT);
    _hdr.e_type = 2;
    _hdr.e_machine = 40;
    _hdr.e_version = 1;
    _hdr.e_phoff = 52;
    _hdr.e_shoff = 0;
    _hdr.e_flags = 0;
    _hdr.e_ehsize = sizeof(Sony_Elf32_Ehdr);
    _hdr.e_phentsize = sizeof(Sony_Elf32_Phdr);
    _hdr.e_shentsize = 0;
    _hdr.e_shnum = 0;
    _hdr.e_shstrndx = 0;

    if (auto entrypoint_address = header.entrypoint_address()) {
        _hdr.e_entry = *entrypoint_address;
    } else if (auto kernel_address = header.kernel_address()) {
        _hdr.e_entry = *kernel_address;
    }

    // Construct kernel program header
    _hdr_kernel.p_type = SONY_E_TYPE_KERNEL;
    _hdr_kernel.p_flags = SONY_E_FLAGS_KERNEL;
    _hdr_kernel.p_align = 0;

    if (auto address = header.kernel_address()) {
        _hdr_kernel.p_vaddr = *address;
        _hdr_kernel.p_paddr = *address;
    }

    // Construct ramdisk program header
    _hdr_ramdisk.p_type = SONY_E_TYPE_RAMDISK;
    _hdr_ramdisk.p_flags = SONY_E_FLAGS_RAMDISK;
    _hdr_ramdisk.p_align = 0;

    if (auto address = header.ramdisk_address()) {
        _hdr_ramdisk.p_vaddr = *address;
        _hdr_ramdisk.p_paddr = *address;
    }

    // Construct cmdline program header
    _hdr_cmdline.p_type = SONY_E_TYPE_CMDLINE;
    _hdr_cmdline.p_vaddr = 0;
    _hdr_cmdline.p_paddr = 0;
    _hdr_cmdline.p_flags = SONY_E_FLAGS_CMDLINE;
    _hdr_cmdline.p_align = 0;

    if (auto cmdline = header.kernel_cmdline()) {
        _cmdline = *cmdline;
    }

    // Construct IPL program header
    _hdr_ipl.p_type = SONY_E_TYPE_IPL;
    _hdr_ipl.p_flags = SONY_E_FLAGS_IPL;
    _hdr_ipl.p_align = 0;

    if (auto address = header.sony_ipl_address()) {
        _hdr_ipl.p_vaddr = *address;
        _hdr_ipl.p_paddr = *address;
    }

    // Construct RPM program header
    _hdr_rpm.p_type = SONY_E_TYPE_RPM;
    _hdr_rpm.p_flags = SONY_E_FLAGS_RPM;
    _hdr_rpm.p_align = 0;

    if (auto address = header.sony_rpm_address()) {
        _hdr_rpm.p_vaddr = *address;
        _hdr_rpm.p_paddr = *address;
    }

    // Construct APPSBL program header
    _hdr_appsbl.p_type = SONY_E_TYPE_APPSBL;
    _hdr_appsbl.p_flags = SONY_E_FLAGS_APPSBL;
    _hdr_appsbl.p_align = 0;

    if (auto address = header.sony_appsbl_address()) {
        _hdr_appsbl.p_vaddr = *address;
        _hdr_appsbl.p_paddr = *address;
    }

    // Clear existing entries (none should exist unless this function fails and
    // the user reattempts to call it)
    _seg.entries_clear();

    ret = _seg.entries_add(ENTRY_TYPE_KERNEL, 0, false, 0, _writer);
    if (ret != RET_OK) { return ret; }

    ret = _seg.entries_add(ENTRY_TYPE_RAMDISK, 0, false, 0, _writer);
    if (ret != RET_OK) { return ret; }

    ret = _seg.entries_add(SONY_ELF_ENTRY_CMDLINE, 0, false, 0, _writer);
    if (ret != RET_OK) { return ret; }

    ret = _seg.entries_add(ENTRY_TYPE_SONY_IPL, 0, false, 0, _writer);
    if (ret != RET_OK) { return ret; }

    ret = _seg.entries_add(ENTRY_TYPE_SONY_RPM, 0, false, 0, _writer);
    if (ret != RET_OK) { return ret; }

    ret = _seg.entries_add(ENTRY_TYPE_SONY_APPSBL, 0, false, 0, _writer);
    if (ret != RET_OK) { return ret; }

    // Start writing at offset 4096
    if (!file.seek(4096, SEEK_SET, nullptr)) {
        _writer.set_error(file.error(),
                          "Failed to seek to first page: %s",
                          file.error_string().c_str());
        return file.is_fatal() ? RET_FATAL : RET_FAILED;
    }

    return RET_OK;
}

int SonyElfFormatWriter::get_entry(File &file, Entry &entry)
{
    int ret;

    ret = _seg.get_entry(file, entry, _writer);
    if (ret != RET_OK) {
        return ret;
    }

    auto const *swentry = _seg.entry();

    // Silently handle cmdline entry
    if (swentry->type == SONY_ELF_ENTRY_CMDLINE) {
        size_t n;

        entry.clear();

        entry.set_size(_cmdline.size());

        ret = write_entry(file, entry);
        if (ret != RET_OK) { return RET_FATAL; }

        ret = write_data(file, _cmdline.data(), _cmdline.size(), n);
        if (ret != RET_OK) { return RET_FATAL; }

        ret = finish_entry(file);
        if (ret != RET_OK) { return RET_FATAL; }

        ret = get_entry(file, entry);
        if (ret != RET_OK) { return RET_FATAL; }
    }

    return RET_OK;
}

int SonyElfFormatWriter::write_entry(File &file, const Entry &entry)
{
    return _seg.write_entry(file, entry, _writer);
}

int SonyElfFormatWriter::write_data(File &file, const void *buf,
                                    size_t buf_size, size_t &bytes_written)
{
    return _seg.write_data(file, buf, buf_size, bytes_written, _writer);
}

int SonyElfFormatWriter::finish_entry(File &file)
{
    int ret;

    ret = _seg.finish_entry(file, _writer);
    if (ret != RET_OK) {
        return ret;
    }

    auto const *swentry = _seg.entry();

    switch (swentry->type) {
    case ENTRY_TYPE_KERNEL:
        _hdr_kernel.p_offset = static_cast<Elf32_Off>(swentry->offset);
        _hdr_kernel.p_filesz = swentry->size;
        _hdr_kernel.p_memsz = swentry->size;
        break;
    case ENTRY_TYPE_RAMDISK:
        _hdr_ramdisk.p_offset = static_cast<Elf32_Off>(swentry->offset);
        _hdr_ramdisk.p_filesz = swentry->size;
        _hdr_ramdisk.p_memsz = swentry->size;
        break;
    case ENTRY_TYPE_SONY_IPL:
        _hdr_ipl.p_offset = static_cast<Elf32_Off>(swentry->offset);
        _hdr_ipl.p_filesz = swentry->size;
        _hdr_ipl.p_memsz = swentry->size;
        break;
    case ENTRY_TYPE_SONY_RPM:
        _hdr_rpm.p_offset = static_cast<Elf32_Off>(swentry->offset);
        _hdr_rpm.p_filesz = swentry->size;
        _hdr_rpm.p_memsz = swentry->size;
        break;
    case ENTRY_TYPE_SONY_APPSBL:
        _hdr_appsbl.p_offset = static_cast<Elf32_Off>(swentry->offset);
        _hdr_appsbl.p_filesz = swentry->size;
        _hdr_appsbl.p_memsz = swentry->size;
        break;
    case SONY_ELF_ENTRY_CMDLINE:
        _hdr_cmdline.p_offset = static_cast<Elf32_Off>(swentry->offset);
        _hdr_cmdline.p_filesz = swentry->size;
        _hdr_cmdline.p_memsz = swentry->size;
        break;
    }

    if (swentry->size > 0) {
        ++_hdr.e_phnum;
    }

    return RET_OK;
}

int SonyElfFormatWriter::close(File &file)
{
    size_t n;

    auto const *swentry = _seg.entry();

    // If successful, finish up the boot image
    if (!swentry) {
        // Write headers
        Sony_Elf32_Ehdr hdr = _hdr;
        Sony_Elf32_Phdr hdr_kernel = _hdr_kernel;
        Sony_Elf32_Phdr hdr_ramdisk = _hdr_ramdisk;
        Sony_Elf32_Phdr hdr_cmdline = _hdr_cmdline;
        Sony_Elf32_Phdr hdr_ipl = _hdr_ipl;
        Sony_Elf32_Phdr hdr_rpm = _hdr_rpm;
        Sony_Elf32_Phdr hdr_appsbl = _hdr_appsbl;

        struct {
            const void *ptr;
            size_t size;
            bool can_write;
        } headers[] = {
            { &hdr, sizeof(hdr), true },
            { &hdr_kernel, sizeof(hdr_kernel), hdr_kernel.p_filesz > 0 },
            { &hdr_ramdisk, sizeof(hdr_ramdisk), hdr_ramdisk.p_filesz > 0 },
            { &hdr_cmdline, sizeof(hdr_cmdline), hdr_cmdline.p_filesz > 0 },
            { &hdr_ipl, sizeof(hdr_ipl), hdr_ipl.p_filesz > 0 },
            { &hdr_rpm, sizeof(hdr_rpm), hdr_rpm.p_filesz > 0 },
            { &hdr_appsbl, sizeof(hdr_appsbl), hdr_appsbl.p_filesz > 0 },
            { nullptr, 0, false },
        };

        sony_elf_fix_ehdr_byte_order(hdr);
        sony_elf_fix_phdr_byte_order(hdr_kernel);
        sony_elf_fix_phdr_byte_order(hdr_ramdisk);
        sony_elf_fix_phdr_byte_order(hdr_cmdline);
        sony_elf_fix_phdr_byte_order(hdr_ipl);
        sony_elf_fix_phdr_byte_order(hdr_rpm);
        sony_elf_fix_phdr_byte_order(hdr_appsbl);

        // Seek back to beginning to write headers
        if (!file.seek(0, SEEK_SET, nullptr)) {
            _writer.set_error(file.error(),
                              "Failed to seek to beginning: %s",
                              file.error_string().c_str());
            return file.is_fatal() ? RET_FATAL : RET_FAILED;
        }

        // Write headers
        for (auto it = headers; it->ptr && it->can_write; ++it) {
            if (!file_write_fully(file, it->ptr, it->size, n)
                    || n != it->size) {
                _writer.set_error(file.error(),
                                  "Failed to write header: %s",
                                  file.error_string().c_str());
                return file.is_fatal() ? RET_FATAL : RET_FAILED;
            }
        }
    }

    return RET_OK;
}

}

/*!
 * \brief Set Sony ELF boot image output format
 *
 * \return
 *   * #RET_OK if the format is successfully set
 *   * \<= #RET_WARN if an error occurs
 */
int Writer::set_format_sony_elf()
{
    using namespace sonyelf;

    MB_PRIVATE(Writer);

    std::unique_ptr<FormatWriter> format{new SonyElfFormatWriter(*this)};
    return priv->register_format(std::move(format));
}

}
}
