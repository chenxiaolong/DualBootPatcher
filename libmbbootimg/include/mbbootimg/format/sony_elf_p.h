/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#include "mbbootimg/guard_p.h"

#include "mbcommon/common.h"
#include "mbcommon/endian.h"

#include "mbbootimg/format/sony_elf_glibc_p.h"

MB_BEGIN_C_DECLS

static inline void sony_elf_fix_ehdr_byte_order(struct Sony_Elf32_Ehdr *header)
{
    header->e_type = mb_le16toh(header->e_type);
    header->e_machine = mb_le16toh(header->e_machine);
    header->e_version = mb_le32toh(header->e_version);
    header->e_entry = mb_le32toh(header->e_entry);
    header->e_phoff = mb_le32toh(header->e_phoff);
    header->e_shoff = mb_le32toh(header->e_shoff);
    header->e_flags = mb_le32toh(header->e_flags);
    header->e_ehsize = mb_le16toh(header->e_ehsize);
    header->e_phentsize = mb_le16toh(header->e_phentsize);
    header->e_phnum = mb_le16toh(header->e_phnum);
    header->e_shentsize = mb_le16toh(header->e_shentsize);
    header->e_shnum = mb_le16toh(header->e_shnum);
    header->e_shstrndx = mb_le16toh(header->e_shstrndx);
}

static inline void sony_elf_fix_phdr_byte_order(struct Sony_Elf32_Phdr *header)
{
    header->p_type = mb_le32toh(header->p_type);
    header->p_offset = mb_le32toh(header->p_offset);
    header->p_vaddr = mb_le32toh(header->p_vaddr);
    header->p_paddr = mb_le32toh(header->p_paddr);
    header->p_filesz = mb_le32toh(header->p_filesz);
    header->p_memsz = mb_le32toh(header->p_memsz);
    header->p_flags = mb_le32toh(header->p_flags);
    header->p_align = mb_le32toh(header->p_align);
}

MB_END_C_DECLS
