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

int sony_elf_writer_get_header(MbBiWriter *biw, void *userdata,
                               MbBiHeader *header)
{
    (void) biw;
    (void) userdata;

    mb_bi_header_set_supported_fields(header, SUPPORTED_FIELDS);

    return MB_BI_OK;
}

int sony_elf_writer_write_header(MbBiWriter *biw, void *userdata,
                                 MbBiHeader *header)
{
    SonyElfWriterCtx *const ctx = static_cast<SonyElfWriterCtx *>(userdata);
    int ret;

    ctx->cmdline.clear();

    memset(&ctx->hdr, 0, sizeof(ctx->hdr));
    memset(&ctx->hdr_kernel, 0, sizeof(ctx->hdr_kernel));
    memset(&ctx->hdr_ramdisk, 0, sizeof(ctx->hdr_ramdisk));
    memset(&ctx->hdr_cmdline, 0, sizeof(ctx->hdr_cmdline));
    memset(&ctx->hdr_ipl, 0, sizeof(ctx->hdr_ipl));
    memset(&ctx->hdr_rpm, 0, sizeof(ctx->hdr_rpm));
    memset(&ctx->hdr_appsbl, 0, sizeof(ctx->hdr_appsbl));

    // Construct ELF header
    memcpy(&ctx->hdr.e_ident, SONY_E_IDENT, SONY_EI_NIDENT);
    ctx->hdr.e_type = 2;
    ctx->hdr.e_machine = 40;
    ctx->hdr.e_version = 1;
    ctx->hdr.e_phoff = 52;
    ctx->hdr.e_shoff = 0;
    ctx->hdr.e_flags = 0;
    ctx->hdr.e_ehsize = sizeof(Sony_Elf32_Ehdr);
    ctx->hdr.e_phentsize = sizeof(Sony_Elf32_Phdr);
    ctx->hdr.e_shentsize = 0;
    ctx->hdr.e_shnum = 0;
    ctx->hdr.e_shstrndx = 0;

    if (mb_bi_header_entrypoint_address_is_set(header)) {
        ctx->hdr.e_entry = mb_bi_header_entrypoint_address(header);
    } else if (mb_bi_header_kernel_address_is_set(header)) {
        ctx->hdr.e_entry = mb_bi_header_kernel_address(header);
    }

    // Construct kernel program header
    ctx->hdr_kernel.p_type = SONY_E_TYPE_KERNEL;
    ctx->hdr_kernel.p_flags = SONY_E_FLAGS_KERNEL;
    ctx->hdr_kernel.p_align = 0;

    if (mb_bi_header_kernel_address_is_set(header)) {
        ctx->hdr_kernel.p_vaddr = mb_bi_header_kernel_address(header);
        ctx->hdr_kernel.p_paddr = mb_bi_header_kernel_address(header);
    }

    // Construct ramdisk program header
    ctx->hdr_ramdisk.p_type = SONY_E_TYPE_RAMDISK;
    ctx->hdr_ramdisk.p_flags = SONY_E_FLAGS_RAMDISK;
    ctx->hdr_ramdisk.p_align = 0;

    if (mb_bi_header_ramdisk_address_is_set(header)) {
        ctx->hdr_ramdisk.p_vaddr = mb_bi_header_ramdisk_address(header);
        ctx->hdr_ramdisk.p_paddr = mb_bi_header_ramdisk_address(header);
    }

    // Construct cmdline program header
    ctx->hdr_cmdline.p_type = SONY_E_TYPE_CMDLINE;
    ctx->hdr_cmdline.p_vaddr = 0;
    ctx->hdr_cmdline.p_paddr = 0;
    ctx->hdr_cmdline.p_flags = SONY_E_FLAGS_CMDLINE;
    ctx->hdr_cmdline.p_align = 0;

    const char *cmdline = mb_bi_header_kernel_cmdline(header);
    if (cmdline) {
        ctx->cmdline = cmdline;
    }

    // Construct IPL program header
    ctx->hdr_ipl.p_type = SONY_E_TYPE_IPL;
    ctx->hdr_ipl.p_flags = SONY_E_FLAGS_IPL;
    ctx->hdr_ipl.p_align = 0;

    if (mb_bi_header_sony_ipl_address_is_set(header)) {
        ctx->hdr_ipl.p_vaddr = mb_bi_header_sony_ipl_address(header);
        ctx->hdr_ipl.p_paddr = mb_bi_header_sony_ipl_address(header);
    }

    // Construct RPM program header
    ctx->hdr_rpm.p_type = SONY_E_TYPE_RPM;
    ctx->hdr_rpm.p_flags = SONY_E_FLAGS_RPM;
    ctx->hdr_rpm.p_align = 0;

    if (mb_bi_header_sony_rpm_address_is_set(header)) {
        ctx->hdr_rpm.p_vaddr = mb_bi_header_sony_rpm_address(header);
        ctx->hdr_rpm.p_paddr = mb_bi_header_sony_rpm_address(header);
    }

    // Construct APPSBL program header
    ctx->hdr_appsbl.p_type = SONY_E_TYPE_APPSBL;
    ctx->hdr_appsbl.p_flags = SONY_E_FLAGS_APPSBL;
    ctx->hdr_appsbl.p_align = 0;

    if (mb_bi_header_sony_appsbl_address_is_set(header)) {
        ctx->hdr_appsbl.p_vaddr = mb_bi_header_sony_appsbl_address(header);
        ctx->hdr_appsbl.p_paddr = mb_bi_header_sony_appsbl_address(header);
    }

    // Clear existing entries (none should exist unless this function fails and
    // the user reattempts to call it)
    ctx->seg.entries_clear();

    ret = ctx->seg.entries_add(MB_BI_ENTRY_KERNEL, 0, false, 0, biw);
    if (ret != MB_BI_OK) return ret;

    ret = ctx->seg.entries_add(MB_BI_ENTRY_RAMDISK, 0, false, 0, biw);
    if (ret != MB_BI_OK) return ret;

    ret = ctx->seg.entries_add(SONY_ELF_ENTRY_CMDLINE, 0, false, 0, biw);
    if (ret != MB_BI_OK) return ret;

    ret = ctx->seg.entries_add(MB_BI_ENTRY_SONY_IPL, 0, false, 0, biw);
    if (ret != MB_BI_OK) return ret;

    ret = ctx->seg.entries_add(MB_BI_ENTRY_SONY_RPM, 0, false, 0, biw);
    if (ret != MB_BI_OK) return ret;

    ret = ctx->seg.entries_add(MB_BI_ENTRY_SONY_APPSBL, 0, false, 0, biw);
    if (ret != MB_BI_OK) return ret;

    // Start writing at offset 4096
    if (!biw->file->seek(4096, SEEK_SET, nullptr)) {
        mb_bi_writer_set_error(biw, biw->file->error().value() /* TODO */,
                               "Failed to seek to first page: %s",
                               biw->file->error_string().c_str());
        return biw->file->is_fatal() ? MB_BI_FATAL : MB_BI_FAILED;
    }

    return MB_BI_OK;
}

int sony_elf_writer_get_entry(MbBiWriter *biw, void *userdata,
                              MbBiEntry *entry)
{
    SonyElfWriterCtx *const ctx = static_cast<SonyElfWriterCtx *>(userdata);
    int ret;

    ret = ctx->seg.get_entry(*biw->file, entry, biw);
    if (ret != MB_BI_OK) {
        return ret;
    }

    auto const *swentry = ctx->seg.entry();

    // Silently handle cmdline entry
    if (swentry->type == SONY_ELF_ENTRY_CMDLINE) {
        size_t n;

        mb_bi_entry_clear(entry);

        ret = mb_bi_entry_set_size(entry, ctx->cmdline.size());
        if (ret != MB_BI_OK) return MB_BI_FATAL;

        ret = sony_elf_writer_write_entry(biw, userdata, entry);
        if (ret != MB_BI_OK) return MB_BI_FATAL;

        ret = sony_elf_writer_write_data(biw, userdata, ctx->cmdline.data(),
                                         ctx->cmdline.size(), n);
        if (ret != MB_BI_OK) return MB_BI_FATAL;

        ret = sony_elf_writer_finish_entry(biw, userdata);
        if (ret != MB_BI_OK) return MB_BI_FATAL;

        ret = ctx->seg.get_entry(*biw->file, entry, biw);
        if (ret != MB_BI_OK) return MB_BI_FATAL;
    }

    return MB_BI_OK;
}

int sony_elf_writer_write_entry(MbBiWriter *biw, void *userdata,
                                MbBiEntry *entry)
{
    SonyElfWriterCtx *const ctx = static_cast<SonyElfWriterCtx *>(userdata);

    return ctx->seg.write_entry(*biw->file, entry, biw);
}

int sony_elf_writer_write_data(MbBiWriter *biw, void *userdata,
                               const void *buf, size_t buf_size,
                               size_t &bytes_written)
{
    SonyElfWriterCtx *const ctx = static_cast<SonyElfWriterCtx *>(userdata);

    return ctx->seg.write_data(*biw->file, buf, buf_size, bytes_written, biw);
}

int sony_elf_writer_finish_entry(MbBiWriter *biw, void *userdata)
{
    SonyElfWriterCtx *const ctx = static_cast<SonyElfWriterCtx *>(userdata);
    int ret;

    ret = ctx->seg.finish_entry(*biw->file, biw);
    if (ret != MB_BI_OK) {
        return ret;
    }

    auto const *swentry = ctx->seg.entry();

    switch (swentry->type) {
    case MB_BI_ENTRY_KERNEL:
        ctx->hdr_kernel.p_offset = swentry->offset;
        ctx->hdr_kernel.p_filesz = swentry->size;
        ctx->hdr_kernel.p_memsz = swentry->size;
        break;
    case MB_BI_ENTRY_RAMDISK:
        ctx->hdr_ramdisk.p_offset = swentry->offset;
        ctx->hdr_ramdisk.p_filesz = swentry->size;
        ctx->hdr_ramdisk.p_memsz = swentry->size;
        break;
    case MB_BI_ENTRY_SONY_IPL:
        ctx->hdr_ipl.p_offset = swentry->offset;
        ctx->hdr_ipl.p_filesz = swentry->size;
        ctx->hdr_ipl.p_memsz = swentry->size;
        break;
    case MB_BI_ENTRY_SONY_RPM:
        ctx->hdr_rpm.p_offset = swentry->offset;
        ctx->hdr_rpm.p_filesz = swentry->size;
        ctx->hdr_rpm.p_memsz = swentry->size;
        break;
    case MB_BI_ENTRY_SONY_APPSBL:
        ctx->hdr_appsbl.p_offset = swentry->offset;
        ctx->hdr_appsbl.p_filesz = swentry->size;
        ctx->hdr_appsbl.p_memsz = swentry->size;
        break;
    case SONY_ELF_ENTRY_CMDLINE:
        ctx->hdr_cmdline.p_offset = swentry->offset;
        ctx->hdr_cmdline.p_filesz = swentry->size;
        ctx->hdr_cmdline.p_memsz = swentry->size;
        break;
    }

    if (swentry->size > 0) {
        ++ctx->hdr.e_phnum;
    }

    return MB_BI_OK;
}

int sony_elf_writer_close(MbBiWriter *biw, void *userdata)
{
    SonyElfWriterCtx *const ctx = static_cast<SonyElfWriterCtx *>(userdata);
    size_t n;

    if (!ctx->have_file_size) {
        if (!biw->file->seek(0, SEEK_CUR, &ctx->file_size)) {
            mb_bi_writer_set_error(biw, biw->file->error().value() /* TODO */,
                                   "Failed to get file offset: %s",
                                   biw->file->error_string().c_str());
            return biw->file->is_fatal() ? MB_BI_FATAL : MB_BI_FAILED;
        }

        ctx->have_file_size = true;
    }

    auto const *swentry = ctx->seg.entry();

    // If successful, finish up the boot image
    if (!swentry) {
        // Write headers
        Sony_Elf32_Ehdr hdr = ctx->hdr;
        Sony_Elf32_Phdr hdr_kernel = ctx->hdr_kernel;
        Sony_Elf32_Phdr hdr_ramdisk = ctx->hdr_ramdisk;
        Sony_Elf32_Phdr hdr_cmdline = ctx->hdr_cmdline;
        Sony_Elf32_Phdr hdr_ipl = ctx->hdr_ipl;
        Sony_Elf32_Phdr hdr_rpm = ctx->hdr_rpm;
        Sony_Elf32_Phdr hdr_appsbl = ctx->hdr_appsbl;

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
        if (!biw->file->seek(0, SEEK_SET, nullptr)) {
            mb_bi_writer_set_error(biw, biw->file->error().value() /* TODO */,
                                   "Failed to seek to beginning: %s",
                                   biw->file->error_string().c_str());
            return biw->file->is_fatal() ? MB_BI_FATAL : MB_BI_FAILED;
        }

        // Write headers
        for (auto it = headers; it->ptr && it->can_write; ++it) {
            if (!mb::file_write_fully(*biw->file, it->ptr, it->size, n)
                    || n != it->size) {
                mb_bi_writer_set_error(biw, biw->file->error().value() /* TODO */,
                                       "Failed to write header: %s",
                                       biw->file->error_string().c_str());
                return biw->file->is_fatal() ? MB_BI_FATAL : MB_BI_FAILED;
            }
        }
    }

    return MB_BI_OK;
}

int sony_elf_writer_free(MbBiWriter *bir, void *userdata)
{
    (void) bir;
    delete static_cast<SonyElfWriterCtx *>(userdata);
    return MB_BI_OK;
}

}
}
}

/*!
 * \brief Set Sony ELF boot image output format
 *
 * \param biw MbBiWriter
 *
 * \return
 *   * #MB_BI_OK if the format is successfully enabled
 *   * #MB_BI_WARN if the format is already enabled
 *   * \<= #MB_BI_FAILED if an error occurs
 */
int mb_bi_writer_set_format_sony_elf(MbBiWriter *biw)
{
    using namespace mb::bootimg::sonyelf;

    SonyElfWriterCtx *const ctx = new SonyElfWriterCtx();

    return _mb_bi_writer_register_format(biw,
                                         ctx,
                                         MB_BI_FORMAT_SONY_ELF,
                                         MB_BI_FORMAT_NAME_SONY_ELF,
                                         nullptr,
                                         &sony_elf_writer_get_header,
                                         &sony_elf_writer_write_header,
                                         &sony_elf_writer_get_entry,
                                         &sony_elf_writer_write_entry,
                                         &sony_elf_writer_write_data,
                                         &sony_elf_writer_finish_entry,
                                         &sony_elf_writer_close,
                                         &sony_elf_writer_free);
}
