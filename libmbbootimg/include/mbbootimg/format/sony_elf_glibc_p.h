/*
 * This file defines standard ELF types, structures, and macros.
 * Copyright (C) 1995-2015 Free Software Foundation, Inc.
 * This file is part of the GNU C Library.
 *
 * The GNU C Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * The GNU C Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the GNU C Library; if not, see
 * <http://www.gnu.org/licenses/>.
 */

/*
 * This file is based on elf.h from glibc 2.21 and is licensed under the same
 * terms as the original source code. It can be used standalone (outside of
 * DualBootPatcher) under the terms above.
 */

#pragma once

#include "mbbootimg/guard_p.h"

#include <cstddef>
#include <cstdint>

namespace mb::bootimg::sonyelf
{

using Elf32_Half    = uint16_t;
using Elf32_Word    = uint32_t;
using Elf32_Sword   = int32_t;
using Elf32_Xword   = uint64_t;
using Elf32_Sxword  = int64_t;
using Elf32_Addr    = uint32_t;
using Elf32_Off     = uint32_t;
using Elf32_Section = uint16_t;
using Elf32_Versym  = Elf32_Half;

/* Legal values for p_type (segment type).  */

constexpr Elf32_Word SONY_PT_NULL         = 0;          // Program header table entry unused
constexpr Elf32_Word SONY_PT_LOAD         = 1;          // Loadable program segment
constexpr Elf32_Word SONY_PT_DYNAMIC      = 2;          // Dynamic linking information
constexpr Elf32_Word SONY_PT_INTERP       = 3;          // Program interpreter
constexpr Elf32_Word SONY_PT_NOTE         = 4;          // Auxiliary information
constexpr Elf32_Word SONY_PT_SHLIB        = 5;          // Reserved
constexpr Elf32_Word SONY_PT_PHDR         = 6;          // Entry for header table itself
constexpr Elf32_Word SONY_PT_TLS          = 7;          // Thread-local storage segment
constexpr Elf32_Word SONY_PT_NUM          = 8;          // Number of defined types
constexpr Elf32_Word SONY_PT_LOOS         = 0x60000000; // Start of OS-specific
constexpr Elf32_Word SONY_PT_GNU_EH_FRAME = 0x6474e550; // GCC .eh_frame_hdr segment
constexpr Elf32_Word SONY_PT_GNU_STACK    = 0x6474e551; // Indicates stack executability
constexpr Elf32_Word SONY_PT_GNU_RELRO    = 0x6474e552; // Read-only after relocation
constexpr Elf32_Word SONY_PT_LOSUNW       = 0x6ffffffa;
constexpr Elf32_Word SONY_PT_SUNWBSS      = 0x6ffffffa; // Sun Specific segment
constexpr Elf32_Word SONY_PT_SUNWSTACK    = 0x6ffffffb; // Stack segment
constexpr Elf32_Word SONY_PT_HISUNW       = 0x6fffffff;
constexpr Elf32_Word SONY_PT_HIOS         = 0x6fffffff; // End of OS-specific
constexpr Elf32_Word SONY_PT_LOPROC       = 0x70000000; // Start of processor-specific
constexpr Elf32_Word SONY_PT_HIPROC       = 0x7fffffff; // End of processor-specific

constexpr Elf32_Word SONY_E_FLAGS_KERNEL  = 0x00000000;
constexpr Elf32_Word SONY_E_FLAGS_RAMDISK = 0x80000000;
constexpr Elf32_Word SONY_E_FLAGS_IPL     = 0x40000000;
constexpr Elf32_Word SONY_E_FLAGS_CMDLINE = 0x20000000;
constexpr Elf32_Word SONY_E_FLAGS_RPM     = 0x01000000;
constexpr Elf32_Word SONY_E_FLAGS_APPSBL  = 0x02000000;

constexpr Elf32_Word SONY_E_TYPE_KERNEL   = SONY_PT_LOAD;
constexpr Elf32_Word SONY_E_TYPE_RAMDISK  = SONY_PT_LOAD;
constexpr Elf32_Word SONY_E_TYPE_IPL      = SONY_PT_LOAD;
constexpr Elf32_Word SONY_E_TYPE_CMDLINE  = SONY_PT_NOTE;
constexpr Elf32_Word SONY_E_TYPE_RPM      = SONY_PT_LOAD;
constexpr Elf32_Word SONY_E_TYPE_APPSBL   = SONY_PT_LOAD;
constexpr Elf32_Word SONY_E_TYPE_SIN      = 0x214e4953; // "SIN!"

constexpr unsigned char SONY_E_IDENT[]    = "\x7f" "ELF" "\x01\x01\x01\x61";
constexpr Elf32_Half    SONY_E_TYPE       = 2;
constexpr Elf32_Half    SONY_E_MACHINE    = 40;
constexpr Elf32_Word    SONY_E_VERSION    = 1;
constexpr Elf32_Off     SONY_E_PHOFF      = 52;
constexpr Elf32_Off     SONY_E_SHOFF      = 0;
constexpr Elf32_Word    SONY_E_FLAGS      = 0;
constexpr Elf32_Half    SONY_E_EHSIZE     = 52;
constexpr Elf32_Half    SONY_E_PHENTSIZE  = 32;
constexpr Elf32_Half    SONY_E_SHENTSIZE  = 0;
constexpr Elf32_Half    SONY_E_SHNUM      = 0;
constexpr Elf32_Half    SONY_E_SHSTRNDX   = 0;

constexpr size_t        SONY_EI_NIDENT    = 8;
constexpr size_t        SONY_PADDING      = 8;

struct Sony_Elf32_Ehdr
{
    unsigned char e_ident[SONY_EI_NIDENT];  // Magic number and other info
    unsigned char e_unused[SONY_PADDING];   // Unused padding bytes
    Elf32_Half    e_type;                   // Object file type
    Elf32_Half    e_machine;                // Architecture
    Elf32_Word    e_version;                // Object file version
    Elf32_Addr    e_entry;                  // Entry point virtual address
    Elf32_Off     e_phoff;                  // Program header table file offset
    Elf32_Off     e_shoff;                  // Section header table file offset
    Elf32_Word    e_flags;                  // Processor-specific flags
    Elf32_Half    e_ehsize;                 // ELF header size in bytes
    Elf32_Half    e_phentsize;              // Program header table entry size
    Elf32_Half    e_phnum;                  // Program header table entry count
    Elf32_Half    e_shentsize;              // Section header table entry size
    Elf32_Half    e_shnum;                  // Section header table entry count
    Elf32_Half    e_shstrndx;               // Section header string table index
};

struct Sony_Elf32_Phdr
{
    Elf32_Word    p_type;                   // Segment type
    Elf32_Off     p_offset;                 // Segment file offset
    Elf32_Addr    p_vaddr;                  // Segment virtual address
    Elf32_Addr    p_paddr;                  // Segment physical address
    Elf32_Word    p_filesz;                 // Segment size in file
    Elf32_Word    p_memsz;                  // Segment size in memory
    Elf32_Word    p_flags;                  // Segment flags
    Elf32_Word    p_align;                  // Segment alignment
};

}
