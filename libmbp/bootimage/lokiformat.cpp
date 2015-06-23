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

#include "bootimage/lokiformat.h"

#include <algorithm>

#include <cstring>

#include "bootimage.h"
#include "private/logging.h"

namespace mbp
{

LokiFormat::LokiFormat(BootImageIntermediate *i10e) : AndroidFormat(i10e)
{
}

LokiFormat::~LokiFormat()
{
}

bool LokiFormat::isValid(const unsigned char *data, std::size_t size)
{
    // Check that the size of the boot image is okay
    if (size < 0x400 + LOKI_MAGIC_SIZE) {
        return false;
    }

    // Loki boot images have both the Loki header and the Android header
    std::size_t headerIndex;
    return std::memcmp(&data[0x400], LOKI_MAGIC, LOKI_MAGIC_SIZE) == 0
            && findHeader(data, size, 32, &headerIndex);
}

bool LokiFormat::loadImage(const unsigned char *data, std::size_t size)
{
    // Make sure the file is large enough to contain the Loki header
    if (!isValid(data, size)) {
        return false;
    }

    std::size_t headerIndex;
    if (!findHeader(data, size, 32, &headerIndex)) {
        LOGE("Failed to find Android header in loki'd boot image");
        return false;
    }

    FLOGD("Found Android boot image header at: %" PRIzu, headerIndex);

    if (!loadHeader(data, size, headerIndex)) {
        return false;
    }

    const LokiHeader *loki = reinterpret_cast<const LokiHeader *>(&data[0x400]);

    FLOGD("Found Loki boot image header at 0x%x", 0x400);
    FLOGD("- magic:             %s", StringUtils::toMaxString(
          reinterpret_cast<const char *>(loki->magic), 4).c_str());
    FLOGD("- recovery:          %u", loki->recovery);
    FLOGD("- build:             %s",
          StringUtils::toMaxString(loki->build, 128).c_str());
    FLOGD("- orig_kernel_size:  %u", loki->orig_kernel_size);
    FLOGD("- orig_ramdisk_size: %u", loki->orig_ramdisk_size);
    FLOGD("- ramdisk_addr:      0x%08x", loki->ramdisk_addr);

    if (loki->orig_kernel_size != 0
            && loki->orig_ramdisk_size != 0
            && loki->ramdisk_addr != 0) {
        return loadLokiNewImage(data, size, loki);
    } else {
        return loadLokiOldImage(data, size, loki);
    }
}

bool LokiFormat::createImage(std::vector<unsigned char> *dataOut)
{
    std::vector<unsigned char> data;
    if (!AndroidFormat::createImage(&data)) {
        return false;
    }

    if (!LokiPatcher::patchImage(&data, mI10e->abootImage)) {
        return false;
    }

    dataOut->swap(data);
    return true;
}

bool LokiFormat::loadLokiNewImage(const unsigned char *data, std::size_t size,
                                  const LokiHeader *loki)
{
    LOGD("This is a new loki image");

    uint32_t pageMask = mI10e->pageSize - 1;
    uint32_t fakeSize;

    // From loki_unlok.c
    if (mI10e->ramdiskAddr > 0x88f00000 || mI10e->ramdiskAddr < 0xfa00000) {
        fakeSize = mI10e->pageSize;
    } else {
        fakeSize = 0x200;
    }

    // Find original ramdisk address
    uint32_t ramdiskAddr = lokiFindRamdiskAddress(data, size, loki);
    if (ramdiskAddr == 0) {
        LOGE("Could not find ramdisk address in new loki boot image");
        return false;
    }

    // Restore original values in boot image header
    mI10e->hdrKernelSize = loki->orig_kernel_size;
    mI10e->hdrRamdiskSize = loki->orig_ramdisk_size;
    mI10e->ramdiskAddr = ramdiskAddr;

    uint32_t pageKernelSize = (loki->orig_kernel_size + pageMask) & ~pageMask;
    uint32_t pageRamdiskSize = (loki->orig_ramdisk_size + pageMask) & ~pageMask;

    // Kernel image
    mI10e->kernelImage.assign(
            data + mI10e->pageSize,
            data + mI10e->pageSize + loki->orig_kernel_size);

    // Ramdisk image
    mI10e->ramdiskImage.assign(
            data + mI10e->pageSize + pageKernelSize,
            data + mI10e->pageSize + pageKernelSize + loki->orig_ramdisk_size);

    // No second bootloader image
    mI10e->secondImage.clear();

    // Possible device tree image
    if (mI10e->hdrDtSize != 0) {
        auto startPtr = data + mI10e->pageSize
                + pageKernelSize + pageRamdiskSize + fakeSize;
        mI10e->dtImage.assign(startPtr, startPtr + mI10e->hdrDtSize);
    } else {
        mI10e->dtImage.clear();
    }

    return true;
}

bool LokiFormat::loadLokiOldImage(const unsigned char *data, std::size_t size,
                                  const LokiHeader *loki)
{
    LOGD("This is an old loki image");

    // The kernel tags address is invalid in the old loki images
    mI10e->tagsAddr = BootImage::DefaultBase + BootImage::DefaultTagsOffset;
    FLOGD("Setting kernel tags address to default: 0x%08x", mI10e->tagsAddr);

    uint32_t kernelSize;
    uint32_t ramdiskSize;
    uint32_t ramdiskAddr;

    if (size < mI10e->pageSize + 0x2c + sizeof(int32_t)) {
        FLOGE("Kernel size field offset exceeds boot image size by %" PRIzu "bytes",
              mI10e->pageSize + 0x2c + sizeof(int32_t) - size);
        return false;
    }

    // If the boot image was patched with an early version of loki, the original
    // kernel size is not stored in the loki header properly (or in the shellcode).
    // The size is stored in the kernel image's header though, so we'll use that.
    // http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html#d0e309
    kernelSize = *(reinterpret_cast<const int32_t *>(data + mI10e->pageSize + 0x2c));
    FLOGD("Kernel size: %u", kernelSize);


    // The ramdisk always comes after the kernel in boot images, so start the
    // search there
    uint32_t gzipOffset = lokiOldFindGzipOffset(
            data, size, mI10e->pageSize + kernelSize);
    if (gzipOffset == 0) {
        LOGE("Could not find gzip offset in old loki boot image");
        return false;
    }

    ramdiskSize = lokiOldFindRamdiskSize(data, size, gzipOffset);

    ramdiskAddr = lokiFindRamdiskAddress(data, size, loki);
    if (ramdiskAddr == 0) {
        LOGE("Could not find ramdisk address in old loki boot image");
        return false;
    }

    mI10e->hdrKernelSize = kernelSize;
    mI10e->hdrRamdiskSize = ramdiskSize;
    mI10e->ramdiskAddr = ramdiskAddr;

    // Kernel image
    mI10e->kernelImage.assign(
            data + mI10e->pageSize,
            data + mI10e->pageSize + kernelSize);

    // Ramdisk image
    mI10e->ramdiskImage.assign(
            data + gzipOffset,
            data + gzipOffset + ramdiskSize);

    // No second bootloader image
    mI10e->secondImage.clear();

    // No device tree image
    mI10e->dtImage.clear();

    return true;
}

uint32_t LokiFormat::lokiOldFindGzipOffset(const unsigned char *data, std::size_t size,
                                           const uint32_t startOffset) const
{
    // gzip header:
    // byte 0-1 : magic bytes 0x1f, 0x8b
    // byte 2   : compression (0x08 = deflate)
    // byte 3   : flags
    // byte 4-7 : modification timestamp
    // byte 8   : compression flags
    // byte 9   : operating system

    static const unsigned char gzipDeflate[] = { 0x1f, 0x8b, 0x08 };

    std::vector<uint32_t> offsetsFlag8; // Has original file name
    std::vector<uint32_t> offsetsFlag0; // No flags

    uint32_t curOffset = startOffset - 1;

    while (true) {
        // Try to find gzip header
        auto it = std::search(data + curOffset + 1, data + size,
                              gzipDeflate, gzipDeflate + 3);

        if (it == data + size) {
            break;
        }

        curOffset = it - data;

        // We're checking 1 more byte so make sure it's within bounds
        if (curOffset + 1 >= size) {
            break;
        }

        if (data[curOffset + 3] == '\x08') {
            FLOGD("Found a gzip header (flag 0x08) at 0x%x", curOffset);
            offsetsFlag8.push_back(curOffset);
        } else if (data[curOffset + 3] == '\x00') {
            FLOGD("Found a gzip header (flag 0x00) at 0x%x", curOffset);
            offsetsFlag0.push_back(curOffset);
        } else {
            FLOGW("Unexpected flag 0x%02x found in gzip header at 0x%x",
                  static_cast<int32_t>(data[curOffset + 3]), curOffset);
            continue;
        }
    }

    FLOGD("Found %" PRIzu " total gzip headers",
          offsetsFlag8.size() + offsetsFlag0.size());

    uint32_t gzipOffset = 0;

    // Prefer gzip headers with original filename flag since most loki'd boot
    // images will have probably been compressed manually using the gzip tool
    if (!offsetsFlag8.empty()) {
        gzipOffset = offsetsFlag8[0];
    }

    if (gzipOffset == 0) {
        if (offsetsFlag0.empty()) {
            LOGW("Could not find the ramdisk's gzip header");
            return 0;
        } else {
            gzipOffset = offsetsFlag0[0];
        }
    }

    FLOGD("Using offset 0x%x", gzipOffset);

    return gzipOffset;
}

uint32_t LokiFormat::lokiOldFindRamdiskSize(const unsigned char *data, std::size_t size,
                                            const uint32_t ramdiskOffset) const
{
    uint32_t ramdiskSize;

    // If the boot image was patched with an old version of loki, the ramdisk
    // size is not stored properly. We'll need to guess the size of the archive.

    // The ramdisk is supposed to be from the gzip header to EOF, but loki needs
    // to store a copy of aboot, so it is put in the last 0x200 bytes of the file.
    ramdiskSize = size - ramdiskOffset - 0x200;
    // For LG kernels:
    // ramdiskSize = size - ramdiskOffset - mPageSize;

    // The gzip file is zero padded, so we'll search backwards until we find a
    // non-zero byte
    std::size_t begin = size - 0x200;
    std::size_t location;
    bool found = false;

    if (begin < mI10e->pageSize) {
        return -1;
    }

    for (std::size_t i = begin; i > begin - mI10e->pageSize; --i) {
        if (data[i] != 0) {
            location = i;
            found = true;
            break;
        }
    }

    if (!found) {
        FLOGD("Ramdisk size: %u (may include some padding)", ramdiskSize);
    } else {
        ramdiskSize = location - ramdiskOffset;
        FLOGD("Ramdisk size: %u (with padding removed)", ramdiskSize);
    }

    return ramdiskSize;
}

uint32_t LokiFormat::lokiFindRamdiskAddress(const unsigned char *data, std::size_t size,
                                            const LokiHeader *loki) const
{
    // If the boot image was patched with a newer version of loki, find the ramdisk
    // offset in the shell code
    uint32_t ramdiskAddr = 0;

    if (loki->ramdisk_addr != 0) {
        for (uint32_t i = 0; i < size - (LOKI_SHELLCODE_SIZE - 9); ++i) {
            if (std::memcmp(&data[i], LOKI_SHELLCODE, LOKI_SHELLCODE_SIZE - 9) == 0) {
                ramdiskAddr = *(reinterpret_cast<const uint32_t *>(
                        &data[i] + LOKI_SHELLCODE_SIZE - 5));
                break;
            }
        }

        if (ramdiskAddr == 0) {
            LOGW("Couldn't determine ramdisk offset");
            return 0;
        }

        FLOGD("Original ramdisk address: 0x%08x", ramdiskAddr);
    } else {
        // Otherwise, use the default for jflte
        ramdiskAddr = mI10e->kernelAddr - 0x00008000 + 0x02000000;
        FLOGD("Default ramdisk address: 0x%08x", ramdiskAddr);
    }

    return ramdiskAddr;
}

}
