/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "bootimage/sonyelfformat.h"

#define __STDC_FORMAT_MACROS
#include <cinttypes>
#include <cstring>

#include "bootimage/sonyelf.h"
#include "private/logging.h"

namespace mbp
{

SonyElfFormat::SonyElfFormat(BootImageIntermediate *i10e)
    : BootImageFormat(i10e)
{
}

SonyElfFormat::~SonyElfFormat()
{
}

bool SonyElfFormat::isValid(const unsigned char *data, std::size_t size)
{
    return size >= sizeof(Sony_Elf32_Ehdr)
            && memcmp(data, SONY_E_IDENT, SONY_EI_NIDENT) == 0;
}

static void dumpEhdr(const Sony_Elf32_Ehdr *hdr)
{
    LOGD("ELF32 header:");
    FLOGD("- e_ident:          %s",
          StringUtils::toPrintable(hdr->e_ident, SONY_EI_NIDENT).c_str());
    FLOGD("- e_unused:         %s",
          StringUtils::toPrintable(hdr->e_unused, SONY_PADDING).c_str());
    FLOGD("- e_type:           %" PRIu16, hdr->e_type);
    FLOGD("- e_machine:        %" PRIu16, hdr->e_machine);
    FLOGD("- e_version:        %" PRIu32, hdr->e_version);
    FLOGD("- e_entry:          0x%08x",   hdr->e_entry);
    FLOGD("- e_phoff:          0x%08x",   hdr->e_phoff);
    FLOGD("- e_shoff:          0x%08x",   hdr->e_shoff);
    FLOGD("- e_flags:          %" PRIu32, hdr->e_flags);
    FLOGD("- e_ehsize:         %" PRIu16, hdr->e_ehsize);
    FLOGD("- e_phentsize:      %" PRIu16, hdr->e_phentsize);
    FLOGD("- e_phnum:          %" PRIu16, hdr->e_phnum);
    FLOGD("- e_shentsize:      %" PRIu16, hdr->e_shentsize);
    FLOGD("- e_shnum:          %" PRIu16, hdr->e_shnum);
    FLOGD("- e_shstrndx:       %" PRIu16, hdr->e_shstrndx);
}

static void dumpPhdr(const Sony_Elf32_Phdr *phdr, Elf32_Half n)
{
    const char *type;
    if (phdr->p_type == SONY_E_TYPE_KERNEL
            && phdr->p_flags == SONY_E_FLAGS_KERNEL) {
        type = "kernel";
    } else if (phdr->p_type == SONY_E_TYPE_RAMDISK
            && phdr->p_flags == SONY_E_FLAGS_RAMDISK) {
        type = "ramdisk";
    } else if (phdr->p_type == SONY_E_TYPE_IPL
            && phdr->p_flags == SONY_E_FLAGS_IPL) {
        type = "ipl";
    } else if (phdr->p_type == SONY_E_TYPE_CMDLINE
            && phdr->p_flags == SONY_E_FLAGS_CMDLINE) {
        type = "cmdline";
    } else if (phdr->p_type == SONY_E_TYPE_RPM
            && phdr->p_flags == SONY_E_FLAGS_RPM) {
        type = "rpm";
    } else if (phdr->p_type == SONY_E_TYPE_APPSBL
            && phdr->p_flags == SONY_E_FLAGS_APPSBL) {
        type = "appsbl";
    } else if (phdr->p_type == SONY_E_TYPE_SIN) {
        type = "Sony \"SIN!\"";
    } else {
        type = "unknown type";
    }

    FLOGD("ELF32 program segment %u (%s):", n, type);
    FLOGD("- p_type:           0x%08x",   phdr->p_type);
    FLOGD("- p_offset:         %" PRIu32, phdr->p_offset);
    FLOGD("- p_vaddr:          0x%08x",   phdr->p_vaddr);
    FLOGD("- p_paddr:          0x%08x",   phdr->p_paddr);
    FLOGD("- p_filesz:         %" PRIu32, phdr->p_filesz);
    FLOGD("- p_memsz:          %" PRIu32, phdr->p_memsz);
    FLOGD("- p_flags:          0x%08x",   phdr->p_flags);
    FLOGD("- p_align:          %" PRIu32, phdr->p_align);
}

bool SonyElfFormat::loadImage(const unsigned char *data, std::size_t size)
{
    if (size < sizeof(Sony_Elf32_Ehdr)) {
        FLOGE("ELF32 header exceeds size by %" PRIzu " bytes",
              sizeof(Sony_Elf32_Ehdr) - size);
        return false;
    }

    std::size_t offset = 0;

    // Read ELF32 header
    const Sony_Elf32_Ehdr *hdr =
            reinterpret_cast<const Sony_Elf32_Ehdr *>(data);

    // Verify magic bytes
    if (memcmp(hdr->e_ident, SONY_E_IDENT, SONY_EI_NIDENT) != 0) {
        LOGE("Unexpected e_ident value in ELF32 header");
        return false;
    }

    mI10e->hdrEntrypoint = hdr->e_entry;

    offset += sizeof(Sony_Elf32_Ehdr);

    dumpEhdr(hdr);

    for (Elf32_Half i = 0; i < hdr->e_phnum; ++i) {
        if (offset + sizeof(Sony_Elf32_Phdr) > size) {
            FLOGE("ELF32 program segment header exceeds size by %" PRIzu " bytes",
                  offset + sizeof(Sony_Elf32_Phdr) - size);
            return false;
        }

        // Read ELF32 program segment header
        const Sony_Elf32_Phdr *phdr =
                reinterpret_cast<const Sony_Elf32_Phdr *>(data + offset);
        offset += sizeof(Sony_Elf32_Phdr);

        if (phdr->p_offset + phdr->p_memsz > size) {
            FLOGE("Program segment data exceeds size by %" PRIzu " bytes",
                  phdr->p_offset + phdr->p_memsz - size);
            return false;
        }

        dumpPhdr(phdr, i);

        const unsigned char *begin = data + phdr->p_offset;
        const unsigned char *end = data + phdr->p_offset + phdr->p_memsz;

        if (phdr->p_type == SONY_E_TYPE_KERNEL
                && phdr->p_flags == SONY_E_FLAGS_KERNEL) {
            mI10e->kernelImage.assign(begin, end);
            mI10e->kernelAddr = phdr->p_vaddr;
        } else if (phdr->p_type == SONY_E_TYPE_RAMDISK
                && phdr->p_flags == SONY_E_FLAGS_RAMDISK) {
            mI10e->ramdiskImage.assign(begin, end);
            mI10e->ramdiskAddr = phdr->p_vaddr;
        } else if (phdr->p_type == SONY_E_TYPE_IPL
                && phdr->p_flags == SONY_E_FLAGS_IPL) {
            mI10e->iplImage.assign(begin, end);
            mI10e->iplAddr = phdr->p_vaddr;
        } else if (phdr->p_type == SONY_E_TYPE_CMDLINE
                && phdr->p_flags == SONY_E_FLAGS_CMDLINE) {
            mI10e->cmdline.assign(begin, end);
        } else if (phdr->p_type == SONY_E_TYPE_RPM
                && phdr->p_flags == SONY_E_FLAGS_RPM) {
            mI10e->rpmImage.assign(begin, end);
            mI10e->rpmAddr = phdr->p_vaddr;
        } else if (phdr->p_type == SONY_E_TYPE_APPSBL
                && phdr->p_flags == SONY_E_FLAGS_APPSBL) {
            mI10e->appsblImage.assign(begin, end);
            mI10e->appsblAddr = phdr->p_vaddr;
        } else if (phdr->p_type == SONY_E_TYPE_SIN) {
            // There are two extra bytes unaccounted for by p_filesz and
            // p_memsz. I don't know if they're significant or not, but they
            // exist in every boot image I've seen.

            if (phdr->p_offset + phdr->p_memsz + 2 > size) {
                LOGW("Trailing two bytes after \"SIN!\" image are truncated");
            } else if (*end == '\0' && *(end + 1) == '\0') {
                LOGW("Trailing two bytes after \"SIN!\" image are zero");
            } else {
                end += 2;
            }

            mI10e->sonySinImage.assign(begin, end);

            // Save header
            mI10e->sonySinHdr.resize(sizeof(Sony_Elf32_Phdr));
            std::memcpy(mI10e->sonySinHdr.data(), phdr,
                        sizeof(Sony_Elf32_Phdr));

            // Clear offset to allow unique comparison
            Sony_Elf32_Phdr *sinPhdr = reinterpret_cast<Sony_Elf32_Phdr *>(
                    mI10e->sonySinHdr.data());
            sinPhdr->p_offset = 0;
        } else {
            FLOGE("Invalid type and/or flags in ELF32 program segment header %u", i);
            return false;
        }
    }

    return true;
}

bool SonyElfFormat::createImage(std::vector<unsigned char> *dataOut)
{
    std::vector<unsigned char> data;

    // Figure out which images we have
    Elf32_Half phnum = 0;

    bool haveKernel = !mI10e->kernelImage.empty();
    bool haveRamdisk = !mI10e->ramdiskImage.empty();
    bool haveCmdline = !mI10e->cmdline.empty();
    bool haveIpl = !mI10e->iplImage.empty();
    bool haveRpm = !mI10e->rpmImage.empty();
    bool haveAppsbl = !mI10e->appsblImage.empty();
    bool haveSin = !mI10e->sonySinImage.empty() && !mI10e->sonySinHdr.empty();

    phnum += haveKernel;
    phnum += haveRamdisk;
    phnum += haveCmdline;
    phnum += haveSin;

    Elf32_Addr entrypoint = mI10e->hdrEntrypoint;
    if (entrypoint == 0 && haveKernel) {
        entrypoint = mI10e->kernelAddr;
    }

    // Create ELF32 header
    Sony_Elf32_Ehdr hdr;
    std::memset(&hdr, 0, sizeof(Sony_Elf32_Ehdr));

    std::memcpy(&hdr.e_ident, SONY_E_IDENT, SONY_EI_NIDENT);
    hdr.e_type = 2;
    hdr.e_machine = 40;
    hdr.e_version = 1;
    hdr.e_entry = entrypoint;
    hdr.e_phoff = 52;
    hdr.e_shoff = 0;
    hdr.e_flags = 0;
    hdr.e_ehsize = sizeof(Sony_Elf32_Ehdr);
    hdr.e_phentsize = sizeof(Sony_Elf32_Phdr);
    hdr.e_phnum = phnum;
    hdr.e_shentsize = 0;
    hdr.e_shnum = 0;
    hdr.e_shstrndx = 0;

    // Write ELF32 header
    unsigned char *hdrPtr = reinterpret_cast<unsigned char *>(&hdr);
    data.insert(data.end(), hdrPtr, hdrPtr + sizeof(Sony_Elf32_Ehdr));

    // ELF32 program segment data starts at 4096 bytes
    std::size_t offset = 4096;

    // Write kernel header
    if (haveKernel) {
        Sony_Elf32_Phdr phdr;
        std::memset(&phdr, 0, sizeof(Sony_Elf32_Phdr));
        phdr.p_type = SONY_E_TYPE_KERNEL;
        phdr.p_offset = offset;
        phdr.p_vaddr = mI10e->kernelAddr;
        phdr.p_paddr = mI10e->kernelAddr;
        phdr.p_filesz = mI10e->kernelImage.size();
        phdr.p_memsz = mI10e->kernelImage.size();
        phdr.p_flags = SONY_E_FLAGS_KERNEL;
        phdr.p_align = 0;

        offset += mI10e->kernelImage.size();

        unsigned char *phdrPtr = reinterpret_cast<unsigned char *>(&phdr);
        data.insert(data.end(), phdrPtr, phdrPtr + sizeof(Sony_Elf32_Phdr));
    }

    // Write ramdisk header
    if (haveRamdisk) {
        Sony_Elf32_Phdr phdr;
        std::memset(&phdr, 0, sizeof(Sony_Elf32_Phdr));
        phdr.p_type = SONY_E_TYPE_RAMDISK;
        phdr.p_offset = offset;
        phdr.p_vaddr = mI10e->ramdiskAddr;
        phdr.p_paddr = mI10e->ramdiskAddr;
        phdr.p_filesz = mI10e->ramdiskImage.size();
        phdr.p_memsz = mI10e->ramdiskImage.size();
        phdr.p_flags = SONY_E_FLAGS_RAMDISK;
        phdr.p_align = 0;

        offset += mI10e->ramdiskImage.size();

        unsigned char *phdrPtr = reinterpret_cast<unsigned char *>(&phdr);
        data.insert(data.end(), phdrPtr, phdrPtr + sizeof(Sony_Elf32_Phdr));
    }

    // Write cmdline header
    if (haveCmdline) {
        Sony_Elf32_Phdr phdr;
        std::memset(&phdr, 0, sizeof(Sony_Elf32_Phdr));
        phdr.p_type = SONY_E_TYPE_CMDLINE;
        phdr.p_offset = offset;
        phdr.p_vaddr = 0;
        phdr.p_paddr = 0;
        phdr.p_filesz = mI10e->cmdline.size();
        phdr.p_memsz = mI10e->cmdline.size();
        phdr.p_flags = SONY_E_FLAGS_CMDLINE;
        phdr.p_align = 0;

        offset += mI10e->cmdline.size();

        unsigned char *phdrPtr = reinterpret_cast<unsigned char *>(&phdr);
        data.insert(data.end(), phdrPtr, phdrPtr + sizeof(Sony_Elf32_Phdr));
    }

    // Write ipl header
    if (haveIpl) {
        Sony_Elf32_Phdr phdr;
        std::memset(&phdr, 0, sizeof(Sony_Elf32_Phdr));
        phdr.p_type = SONY_E_TYPE_IPL;
        phdr.p_offset = offset;
        phdr.p_vaddr = mI10e->iplAddr;
        phdr.p_paddr = mI10e->iplAddr;
        phdr.p_filesz = mI10e->iplImage.size();
        phdr.p_memsz = mI10e->iplImage.size();
        phdr.p_flags = SONY_E_FLAGS_IPL;
        phdr.p_align = 0;

        offset += mI10e->iplImage.size();

        unsigned char *phdrPtr = reinterpret_cast<unsigned char *>(&phdr);
        data.insert(data.end(), phdrPtr, phdrPtr + sizeof(Sony_Elf32_Phdr));
    }

    // Write rpm header
    if (haveRpm) {
        Sony_Elf32_Phdr phdr;
        std::memset(&phdr, 0, sizeof(Sony_Elf32_Phdr));
        phdr.p_type = SONY_E_TYPE_RPM;
        phdr.p_offset = offset;
        phdr.p_vaddr = mI10e->rpmAddr;
        phdr.p_paddr = mI10e->rpmAddr;
        phdr.p_filesz = mI10e->rpmImage.size();
        phdr.p_memsz = mI10e->rpmImage.size();
        phdr.p_flags = SONY_E_FLAGS_RPM;
        phdr.p_align = 0;

        offset += mI10e->rpmImage.size();

        unsigned char *phdrPtr = reinterpret_cast<unsigned char *>(&phdr);
        data.insert(data.end(), phdrPtr, phdrPtr + sizeof(Sony_Elf32_Phdr));
    }

    // Write appsbl header
    if (haveAppsbl) {
        Sony_Elf32_Phdr phdr;
        std::memset(&phdr, 0, sizeof(Sony_Elf32_Phdr));
        phdr.p_type = SONY_E_TYPE_APPSBL;
        phdr.p_offset = offset;
        phdr.p_vaddr = mI10e->appsblAddr;
        phdr.p_paddr = mI10e->appsblAddr;
        phdr.p_filesz = mI10e->appsblImage.size();
        phdr.p_memsz = mI10e->appsblImage.size();
        phdr.p_flags = SONY_E_FLAGS_APPSBL;
        phdr.p_align = 0;

        offset += mI10e->appsblImage.size();

        unsigned char *phdrPtr = reinterpret_cast<unsigned char *>(&phdr);
        data.insert(data.end(), phdrPtr, phdrPtr + sizeof(Sony_Elf32_Phdr));
    }

    // Write sin header and image
    if (haveSin) {
        if (mI10e->sonySinHdr.size() != sizeof(Sony_Elf32_Phdr)) {
            FLOGE("The specified sin header is not %" PRIzu " bytes",
                  sizeof(Sony_Elf32_Phdr));
            return false;
        }

        Sony_Elf32_Phdr phdr;
        std::memcpy(&phdr, mI10e->sonySinHdr.data(), sizeof(Sony_Elf32_Phdr));
        // The sin image directly follows the phdrs
        phdr.p_offset = sizeof(Sony_Elf32_Ehdr)
                + phnum * sizeof(Sony_Elf32_Phdr);

        if (phdr.p_filesz + 2 == mI10e->sonySinImage.size()) {
            LOGI("The sin image contains the two unidentified trailing bytes");
        } else if (phdr.p_filesz != mI10e->sonySinImage.size()) {
            LOGE("The sin image size does not match the size in the phdr");
            return false;
        }

        if (phdr.p_offset + phdr.p_filesz >= 4096) {
            LOGE("The sin image does not fit within the first 4096 bytes");
            return false;
        }

        // Write header
        unsigned char *phdrPtr = reinterpret_cast<unsigned char *>(&phdr);
        data.insert(data.end(), phdrPtr, phdrPtr + sizeof(Sony_Elf32_Phdr));

        // Write data
        data.insert(data.end(),
                    mI10e->sonySinImage.begin(), mI10e->sonySinImage.end());
    }

    // Pad to 4096 bytes
    data.resize(4096);

    if (haveKernel) {
        data.insert(data.end(),
                    mI10e->kernelImage.begin(), mI10e->kernelImage.end());
    }
    if (haveRamdisk) {
        data.insert(data.end(),
                    mI10e->ramdiskImage.begin(), mI10e->ramdiskImage.end());
    }
    if (haveCmdline) {
        data.insert(data.end(),
                    mI10e->cmdline.begin(), mI10e->cmdline.end());
    }
    if (haveIpl) {
        data.insert(data.end(),
                    mI10e->iplImage.begin(), mI10e->iplImage.end());
    }
    if (haveRpm) {
        data.insert(data.end(),
                    mI10e->rpmImage.begin(), mI10e->rpmImage.end());
    }
    if (haveAppsbl) {
        data.insert(data.end(),
                    mI10e->appsblImage.begin(), mI10e->appsblImage.end());
    }

    dataOut->swap(data);

    return true;
}

}
