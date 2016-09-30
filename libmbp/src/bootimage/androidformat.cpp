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

#include "mbp/bootimage/androidformat.h"

#include <cstring>

#include "mblog/logging.h"

#include "mbp/bootimage-common.h"
#include "mbp/private/stringutils.h"
#include "external/sha.h"

#define SAMSUNG_SEANDROID_MAGIC         "SEANDROIDENFORCE"

namespace mbp
{

AndroidFormat::AndroidFormat(BootImageIntermediate *i10e)
    : BootImageFormat(i10e)
{
}

AndroidFormat::~AndroidFormat()
{
}

uint64_t AndroidFormat::typeSupportMask()
{
    return SUPPORTS_KERNEL_ADDRESS
            | SUPPORTS_RAMDISK_ADDRESS
            | SUPPORTS_SECOND_ADDRESS
            | SUPPORTS_TAGS_ADDRESS
            | SUPPORTS_PAGE_SIZE
            | SUPPORTS_BOARD_NAME
            | SUPPORTS_CMDLINE
            | SUPPORTS_KERNEL_IMAGE
            | SUPPORTS_RAMDISK_IMAGE
            | SUPPORTS_SECOND_IMAGE
            | SUPPORTS_DT_IMAGE;
}

bool AndroidFormat::isValid(const unsigned char *data, std::size_t size)
{
    std::size_t headerIndex;
    return findHeader(data, size, 512, &headerIndex);
}

bool AndroidFormat::loadImage(const unsigned char *data, std::size_t size)
{
    std::size_t headerIndex;
    if (!findHeader(data, size, 512, &headerIndex)) {
        LOGE("Failed to find Android header in boot image");
        return false;
    }

    LOGD("Found Android boot image header at: %" PRIzu, headerIndex);

    if (!loadHeader(data, size, headerIndex)) {
        return false;
    }

    uint32_t pos = 0;

    // Save kernel image
    pos += headerIndex;
    pos += sizeof(BootImageHeader);
    pos += skipPadding(sizeof(BootImageHeader), mI10e->pageSize);
    if (pos + mI10e->hdrKernelSize > size) {
        LOGE("Kernel image exceeds boot image size by %" PRIzu " bytes",
             pos + mI10e->hdrKernelSize - size);
        return false;
    }

    mI10e->kernelImage.assign(data + pos, data + pos + mI10e->hdrKernelSize);

    // Save ramdisk image
    pos += mI10e->hdrKernelSize;
    pos += skipPadding(mI10e->hdrKernelSize, mI10e->pageSize);
    if (pos + mI10e->hdrRamdiskSize > size) {
        LOGE("Ramdisk image exceeds boot image size by %" PRIzu " bytes",
             pos + mI10e->hdrRamdiskSize - size);
        return false;
    }

    mI10e->ramdiskImage.assign(data + pos, data + pos + mI10e->hdrRamdiskSize);

    // Save second bootloader image
    pos += mI10e->hdrRamdiskSize;
    pos += skipPadding(mI10e->hdrRamdiskSize, mI10e->pageSize);
    if (pos + mI10e->hdrSecondSize > size) {
        LOGE("Second bootloader image exceeds boot image size by %" PRIzu " bytes",
             pos + mI10e->hdrSecondSize - size);
        return false;
    }

    // The second bootloader may not exist
    if (mI10e->hdrSecondSize > 0) {
        mI10e->secondImage.assign(data + pos, data + pos + mI10e->hdrSecondSize);
    } else {
        mI10e->secondImage.clear();
    }

    // Save device tree image
    pos += mI10e->hdrSecondSize;
    pos += skipPadding(mI10e->hdrSecondSize, mI10e->pageSize);
    if (pos + mI10e->hdrDtSize > size) {
        std::size_t diff = pos + mI10e->hdrDtSize - size;

        LOGE("WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING");
        LOGE("THIS BOOT IMAGE MAY NO LONGER BE BOOTABLE. YOU HAVE BEEN WARNED");
        LOGE("Device tree image exceeds boot image size by %" PRIzu
             " bytes and HAS BEEN TRUNCATED", diff);
        LOGE("WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING");

        mI10e->dtImage.assign(data + pos, data + pos + mI10e->hdrDtSize - diff);
    } else {
        mI10e->dtImage.assign(data + pos, data + pos + mI10e->hdrDtSize);
    }

    // The device tree image may not exist as well
    if (mI10e->hdrDtSize == 0) {
        mI10e->dtImage.clear();
    }

    pos += mI10e->hdrDtSize;
    pos += skipPadding(mI10e->hdrDtSize, mI10e->pageSize);

    return true;
}

static void updateSha1Hash(BootImageHeader *hdr,
                           const BootImageIntermediate *i10e)
{
    SHA_CTX ctx;
    SHA_init(&ctx);

    SHA_update(&ctx, i10e->kernelImage.data(), i10e->kernelImage.size());
    SHA_update(&ctx, reinterpret_cast<char *>(&hdr->kernel_size),
               sizeof(hdr->kernel_size));

    SHA_update(&ctx, i10e->ramdiskImage.data(), i10e->ramdiskImage.size());
    SHA_update(&ctx, reinterpret_cast<char *>(&hdr->ramdisk_size),
               sizeof(hdr->ramdisk_size));
    if (!i10e->secondImage.empty()) {
        SHA_update(&ctx, i10e->secondImage.data(), i10e->secondImage.size());
    }

    // Bug in AOSP? AOSP's mkbootimg adds the second bootloader size to the SHA1
    // hash even if it's 0
    SHA_update(&ctx, reinterpret_cast<char *>(&hdr->second_size),
               sizeof(hdr->second_size));

    if (!i10e->dtImage.empty()) {
        SHA_update(&ctx, i10e->dtImage.data(), i10e->dtImage.size());
        SHA_update(&ctx, reinterpret_cast<char *>(&hdr->dt_size),
                   sizeof(hdr->dt_size));
    }

    std::memset(hdr->id, 0, sizeof(hdr->id));
    memcpy(hdr->id, SHA_final(&ctx), SHA_DIGEST_SIZE);

    std::string hexDigest = StringUtils::toHex(
            reinterpret_cast<const unsigned char *>(hdr->id), SHA_DIGEST_SIZE);

    LOGD("Computed new ID hash: %s", hexDigest.c_str());
}

bool AndroidFormat::createImage(std::vector<unsigned char> *dataOut)
{
    BootImageHeader hdr;
    std::vector<unsigned char> data;

    memset(&hdr, 0, sizeof(BootImageHeader));

    // Set header metadata fields
    memcpy(hdr.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);
    hdr.kernel_size = mI10e->hdrKernelSize;
    hdr.kernel_addr = mI10e->kernelAddr;
    hdr.ramdisk_size = mI10e->hdrRamdiskSize;
    hdr.ramdisk_addr = mI10e->ramdiskAddr;
    hdr.second_size = mI10e->hdrSecondSize;
    hdr.second_addr = mI10e->secondAddr;
    hdr.tags_addr = mI10e->tagsAddr;
    hdr.page_size = mI10e->pageSize;
    hdr.dt_size = mI10e->hdrDtSize;
    hdr.unused = mI10e->hdrUnused;
    std::strncpy(reinterpret_cast<char *>(hdr.name),
                 mI10e->boardName.c_str(), BOOT_NAME_SIZE - 1);
    hdr.name[BOOT_NAME_SIZE - 1] = '\0';
    std::strncpy(reinterpret_cast<char *>(hdr.cmdline),
                 mI10e->cmdline.c_str(), BOOT_ARGS_SIZE - 1);
    hdr.cmdline[BOOT_ARGS_SIZE - 1] = '\0';

    // Update SHA1
    updateSha1Hash(&hdr, mI10e);

    switch (mI10e->pageSize) {
    case 2048:
    case 4096:
    case 8192:
    case 16384:
    case 32768:
    case 65536:
    case 131072:
        break;
    default:
        LOGE("Invalid page size: %u", mI10e->pageSize);
        return false;
    }

    // Header
    unsigned char *hdrBegin = reinterpret_cast<unsigned char *>(&hdr);
    data.insert(data.end(), hdrBegin, hdrBegin + sizeof(BootImageHeader));

    // Padding
    uint32_t paddingSize = skipPadding(sizeof(BootImageHeader), hdr.page_size);
    data.insert(data.end(), paddingSize, 0);

    // Kernel image
    data.insert(data.end(),
                mI10e->kernelImage.begin(),
                mI10e->kernelImage.end());

    // More padding
    paddingSize = skipPadding(mI10e->kernelImage.size(), hdr.page_size);
    data.insert(data.end(), paddingSize, 0);

    // Ramdisk image
    data.insert(data.end(),
                mI10e->ramdiskImage.begin(),
                mI10e->ramdiskImage.end());

    // Even more padding
    paddingSize = skipPadding(mI10e->ramdiskImage.size(), hdr.page_size);
    data.insert(data.end(), paddingSize, 0);

    // Second bootloader image
    if (!mI10e->secondImage.empty()) {
        data.insert(data.end(),
                    mI10e->secondImage.begin(),
                    mI10e->secondImage.end());

        // Enough padding already!
        paddingSize = skipPadding(mI10e->secondImage.size(), hdr.page_size);
        data.insert(data.end(), paddingSize, 0);
    }

    // Device tree image
    if (!mI10e->dtImage.empty()) {
        data.insert(data.end(),
                    mI10e->dtImage.begin(),
                    mI10e->dtImage.end());

        // Last bit of padding (I hope)
        paddingSize = skipPadding(mI10e->dtImage.size(), hdr.page_size);
        data.insert(data.end(), paddingSize, 0);
    }

    data.insert(data.end(), SAMSUNG_SEANDROID_MAGIC,
                SAMSUNG_SEANDROID_MAGIC + strlen(SAMSUNG_SEANDROID_MAGIC));

    dataOut->swap(data);
    return true;
}

bool AndroidFormat::findHeader(const unsigned char *data, std::size_t size,
                               std::size_t searchRange, std::size_t *headerIndex)
{
    // Check that the size of the boot image is okay
    if (size < searchRange + sizeof(BootImageHeader)) {
        LOGW("The boot image is smaller than the boot image header");
        return false;
    }

    // Find the Android magic string
    for (std::size_t i = 0; i <= searchRange; ++i) {
        if (std::memcmp(data + i, BOOT_MAGIC, BOOT_MAGIC_SIZE) == 0) {
            *headerIndex = i;
            return true;
        }
    }

    return false;
}

uint32_t AndroidFormat::skipPadding(const uint32_t itemSize,
                                    const uint32_t pageSize)
{
    uint32_t pageMask = pageSize - 1;

    if ((itemSize & pageMask) == 0) {
        return 0;
    }

    return pageSize - (itemSize & pageMask);
}

bool AndroidFormat::loadHeader(const unsigned char *data, std::size_t size,
                               const std::size_t headerIndex)
{
    // Make sure the file is large enough to contain the header
    if (size < headerIndex + sizeof(BootImageHeader)) {
        LOGE("Boot image header exceeds size by %" PRIzu " bytes",
             headerIndex + sizeof(BootImageHeader) - size);
        return false;
    }

    // Read the Android boot image header
    auto hdr = reinterpret_cast<const BootImageHeader *>(&data[headerIndex]);

    // Read the page size
    switch (hdr->page_size) {
    case 2048:
    case 4096:
    case 8192:
    case 16384:
    case 32768:
    case 65536:
    case 131072:
        break;
    default:
        LOGE("Invalid page size: %u", hdr->page_size);
        return false;
    }

    dumpHeader(hdr);

    // Save header fields
    mI10e->kernelAddr = hdr->kernel_addr;
    mI10e->ramdiskAddr = hdr->ramdisk_addr;
    mI10e->secondAddr = hdr->second_addr;
    mI10e->tagsAddr = hdr->tags_addr;
    mI10e->pageSize = hdr->page_size;
    mI10e->boardName = StringUtils::toMaxString(
            reinterpret_cast<const char *>(hdr->name), BOOT_NAME_SIZE);
    mI10e->cmdline = StringUtils::toMaxString(
            reinterpret_cast<const char *>(hdr->cmdline), BOOT_ARGS_SIZE);
    // Raw header fields
    mI10e->hdrKernelSize = hdr->kernel_size;
    mI10e->hdrRamdiskSize = hdr->ramdisk_size;
    mI10e->hdrSecondSize = hdr->second_size;
    mI10e->hdrDtSize = hdr->dt_size;
    mI10e->hdrUnused = hdr->unused;
    for (std::size_t i = 0; i < sizeof(hdr->id) / sizeof(hdr->id[0]); ++i) {
        mI10e->hdrId[i] = hdr->id[i];
    }

    return true;
}

void AndroidFormat::dumpHeader(const BootImageHeader *hdr)
{
    LOGD("- magic:        %s",
         std::string(hdr->magic, hdr->magic + BOOT_MAGIC_SIZE).c_str());
    LOGD("- kernel_size:  %u",     hdr->kernel_size);
    LOGD("- kernel_addr:  0x%08x", hdr->kernel_addr);
    LOGD("- ramdisk_size: %u",     hdr->ramdisk_size);
    LOGD("- ramdisk_addr: 0x%08x", hdr->ramdisk_addr);
    LOGD("- second_size:  %u",     hdr->second_size);
    LOGD("- second_addr:  0x%08x", hdr->second_addr);
    LOGD("- tags_addr:    0x%08x", hdr->tags_addr);
    LOGD("- page_size:    %u",     hdr->page_size);
    LOGD("- dt_size:      %u",     hdr->dt_size);
    LOGD("- unused:       0x%08x", hdr->unused);
    LOGD("- name:         %s",     StringUtils::toMaxString(
         reinterpret_cast<const char *>(hdr->name), BOOT_NAME_SIZE).c_str());
    LOGD("- cmdline:      %s",     StringUtils::toMaxString(
         reinterpret_cast<const char *>(hdr->cmdline), BOOT_ARGS_SIZE).c_str());
    LOGD("- id:           %s",     StringUtils::toHex(
         reinterpret_cast<const unsigned char *>(hdr->id), 32).c_str());
}

}
