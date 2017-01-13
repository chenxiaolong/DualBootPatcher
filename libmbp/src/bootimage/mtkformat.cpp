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

#include "mbp/bootimage/mtkformat.h"

#include <cstring>

#include "mbcommon/string.h"
#include "mblog/logging.h"

#include "mbp/bootimage-common.h"
#include "mbp/bootimage/mtk.h"
#include "mbp/private/stringutils.h"
#include "external/sha.h"

namespace mbp
{

MtkFormat::MtkFormat(BootImageIntermediate *i10e) : AndroidFormat(i10e)
{
}

MtkFormat::~MtkFormat()
{
}

uint64_t MtkFormat::typeSupportMask()
{
    return AndroidFormat::typeSupportMask()
            | SUPPORTS_KERNEL_MTKHDR
            | SUPPORTS_RAMDISK_MTKHDR;
}

bool MtkFormat::isValid(const unsigned char *data, std::size_t size)
{
    // We have to parse the boot image so we can search for the mtk headers

    // Find Android header
    std::size_t headerIndex;
    if (!findHeader(data, size, 512, &headerIndex)) {
        return false;
    }

    // Check for header size overflow
    if (size < headerIndex + sizeof(BootImageHeader)) {
        return false;
    }

    // Read the Android boot image header
    auto hdr = reinterpret_cast<const BootImageHeader *>(&data[headerIndex]);

    uint32_t pos = 0;

    // Skip header
    pos += headerIndex;
    pos += sizeof(BootImageHeader);
    pos += skipPadding(sizeof(BootImageHeader), hdr->page_size);

    // Check for kernel size overflow
    if (pos + hdr->kernel_size > size) {
        return false;
    }
    if (hdr->kernel_size >= sizeof(MtkHeader)) {
        auto mtkHdr = reinterpret_cast<const MtkHeader *>(&data[pos]);
        if (std::memcmp(mtkHdr->magic, MTK_MAGIC, MTK_MAGIC_SIZE) == 0) {
            return true;
        }
    }
    // Skip kernel image
    pos += hdr->kernel_size;
    pos += skipPadding(hdr->kernel_size, hdr->page_size);

    // Check for ramdisk size overflow
    if (pos + hdr->ramdisk_size > size) {
        return false;
    }
    if (hdr->ramdisk_size >= sizeof(MtkHeader)) {
        auto mtkHdr = reinterpret_cast<const MtkHeader *>(&data[pos]);
        if (std::memcmp(mtkHdr->magic, MTK_MAGIC, MTK_MAGIC_SIZE) == 0) {
            return true;
        }
    }
    // Skip ramdisk image
    pos += hdr->ramdisk_size;
    pos += skipPadding(hdr->ramdisk_size, hdr->page_size);

    // There's no need to check any other images since the mtk header should
    // only exist for the kernel and ramdisk
    return false;
}

void dumpMtkHeader(const MtkHeader *mtkHdr)
{
    LOGD("MTK header:");
    LOGD("- magic:        %s",
         StringUtils::toPrintable(mtkHdr->magic, MTK_MAGIC_SIZE).c_str());
    LOGD("- size:         %u", mtkHdr->size);
    LOGD("- type:         %s",
         StringUtils::toMaxString(mtkHdr->type, MTK_TYPE_SIZE).c_str());
    auto unusedUchar = reinterpret_cast<const unsigned char *>(mtkHdr->unused);
    LOGD("- unused:       %s",
         StringUtils::toPrintable(unusedUchar, MTK_UNUSED_SIZE).c_str());
}

bool MtkFormat::loadImage(const unsigned char *data, std::size_t size)
{
    // We can load the image as an Android boot image
    if (!mbp::AndroidFormat::loadImage(data, size)) {
        return false;
    }

    // Check if the kernel has an mtk header
    if (mI10e->hdrKernelSize >= sizeof(MtkHeader)) {
        auto mtkHdr = reinterpret_cast<const MtkHeader *>(mI10e->kernelImage.data());
        // Check magic
        if (std::memcmp(mtkHdr->magic, MTK_MAGIC, MTK_MAGIC_SIZE) == 0) {
            dumpMtkHeader(mtkHdr);

            std::size_t expected = sizeof(MtkHeader) + mtkHdr->size;
            std::size_t actual = mI10e->kernelImage.size();

            // Check size
            if (actual < expected) {
                LOGE("Expected %" MB_PRIzu " byte kernel image, but have %" MB_PRIzu " bytes",
                     expected, actual);
                return false;
            } else if (actual != expected) {
                LOGW("Expected %" MB_PRIzu " byte kernel image, but have %" MB_PRIzu " bytes",
                     expected, actual);
                LOGW("Repacked boot image will not be byte-for-byte identical to original");
            }

            // Move header to mI10e->mtkKernelHdr
            mI10e->mtkKernelHdr.assign(
                    mI10e->kernelImage.begin(),
                    mI10e->kernelImage.begin() + sizeof(MtkHeader));
            std::vector<unsigned char>(
                    mI10e->kernelImage.begin() + sizeof(MtkHeader),
                    mI10e->kernelImage.end())
                    .swap(mI10e->kernelImage);

            auto newMtkHdr = reinterpret_cast<MtkHeader *>(mI10e->mtkKernelHdr.data());
            newMtkHdr->size = 0;
        }
    }

    // Check if the ramdisk has an mtk header
    if (mI10e->hdrRamdiskSize >= sizeof(MtkHeader)) {
        auto mtkHdr = reinterpret_cast<const MtkHeader *>(mI10e->ramdiskImage.data());
        // Check magic
        if (std::memcmp(mtkHdr->magic, MTK_MAGIC, MTK_MAGIC_SIZE) == 0) {
            dumpMtkHeader(mtkHdr);

            std::size_t expected = sizeof(MtkHeader) + mtkHdr->size;
            std::size_t actual = mI10e->ramdiskImage.size();

            // Check size
            if (actual != expected) {
                LOGE("Expected %" MB_PRIzu " byte ramdisk image, but have %" MB_PRIzu " bytes",
                     expected, actual);
                return false;
            }

            // Move header to mI10e->mtkRamdiskHdr
            mI10e->mtkRamdiskHdr.assign(
                    mI10e->ramdiskImage.begin(),
                    mI10e->ramdiskImage.begin() + sizeof(MtkHeader));
            std::vector<unsigned char>(
                    mI10e->ramdiskImage.begin() + sizeof(MtkHeader),
                    mI10e->ramdiskImage.end())
                    .swap(mI10e->ramdiskImage);

            auto newMtkHdr = reinterpret_cast<MtkHeader *>(mI10e->mtkRamdiskHdr.data());
            newMtkHdr->size = 0;
        }
    }

    return true;
}

static void updateSha1Hash(BootImageHeader *hdr,
                           const BootImageIntermediate *i10e,
                           const MtkHeader *mtkKernelHdr,
                           const MtkHeader *mtkRamdiskHdr,
                           uint32_t kernelSize,
                           uint32_t ramdiskSize)
{
    SHA_CTX ctx;
    SHA_init(&ctx);

    if (mtkKernelHdr) {
        SHA_update(&ctx, mtkKernelHdr, sizeof(MtkHeader));
    }
    SHA_update(&ctx, i10e->kernelImage.data(), i10e->kernelImage.size());
    SHA_update(&ctx, reinterpret_cast<char *>(&kernelSize),
               sizeof(kernelSize));

    if (mtkRamdiskHdr) {
        SHA_update(&ctx, mtkRamdiskHdr, sizeof(MtkHeader));
    }
    SHA_update(&ctx, i10e->ramdiskImage.data(), i10e->ramdiskImage.size());
    SHA_update(&ctx, reinterpret_cast<char *>(&ramdiskSize),
               sizeof(ramdiskSize));
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

bool MtkFormat::createImage(std::vector<unsigned char> *dataOut)
{
    BootImageHeader hdr;
    std::vector<unsigned char> data;

    memset(&hdr, 0, sizeof(BootImageHeader));

    bool hasKernelHdr = !mI10e->mtkKernelHdr.empty();
    bool hasRamdiskHdr = !mI10e->mtkRamdiskHdr.empty();

    // Check header sizes
    if (hasKernelHdr && mI10e->mtkKernelHdr.size() != sizeof(MtkHeader)) {
        LOGE("Expected %" MB_PRIzu " byte kernel MTK header, but have %" MB_PRIzu " bytes",
             sizeof(MtkHeader), mI10e->mtkKernelHdr.size());
        return false;
    }
    if (hasRamdiskHdr && mI10e->mtkRamdiskHdr.size() != sizeof(MtkHeader)) {
        LOGE("Expected %" MB_PRIzu " byte ramdisk MTK header, but have %" MB_PRIzu " bytes",
             sizeof(MtkHeader), mI10e->mtkRamdiskHdr.size());
        return false;
    }

    std::size_t kernelSize =
            mI10e->kernelImage.size() + mI10e->mtkKernelHdr.size();
    std::size_t ramdiskSize =
            mI10e->ramdiskImage.size() + mI10e->mtkRamdiskHdr.size();

    MtkHeader mtkKernelHdr;
    MtkHeader mtkRamdiskHdr;
    if (hasKernelHdr) {
        std::memcpy(&mtkKernelHdr, mI10e->mtkKernelHdr.data(), sizeof(MtkHeader));
        mtkKernelHdr.size = mI10e->kernelImage.size();
    }
    if (hasRamdiskHdr) {
        std::memcpy(&mtkRamdiskHdr, mI10e->mtkRamdiskHdr.data(), sizeof(MtkHeader));
        mtkRamdiskHdr.size = mI10e->ramdiskImage.size();
    }

    // Set header metadata fields
    memcpy(hdr.magic, BOOT_MAGIC, BOOT_MAGIC_SIZE);
    hdr.kernel_size = kernelSize;
    hdr.kernel_addr = mI10e->kernelAddr;
    hdr.ramdisk_size = ramdiskSize;
    hdr.ramdisk_addr = mI10e->ramdiskAddr;
    hdr.second_size = mI10e->hdrSecondSize;
    hdr.second_addr = mI10e->secondAddr;
    hdr.tags_addr = mI10e->tagsAddr;
    hdr.page_size = mI10e->pageSize;
    hdr.dt_size = mI10e->hdrDtSize;
    hdr.unused = mI10e->hdrUnused;
    // -1 for null byte
    std::strcpy(reinterpret_cast<char *>(hdr.name),
                mI10e->boardName.substr(0, BOOT_NAME_SIZE - 1).c_str());
    std::strcpy(reinterpret_cast<char *>(hdr.cmdline),
                mI10e->cmdline.substr(0, BOOT_ARGS_SIZE - 1).c_str());

    // Update SHA1
    updateSha1Hash(&hdr, mI10e,
                   hasKernelHdr ? &mtkKernelHdr : nullptr,
                   hasRamdiskHdr ? &mtkRamdiskHdr : nullptr,
                   kernelSize, ramdiskSize);

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
    if (hasKernelHdr) {
        data.insert(data.end(),
                    reinterpret_cast<const unsigned char *>(&mtkKernelHdr),
                    reinterpret_cast<const unsigned char *>(&mtkKernelHdr) + sizeof(MtkHeader));
    }
    data.insert(data.end(),
                mI10e->kernelImage.begin(),
                mI10e->kernelImage.end());

    // More padding
    paddingSize = skipPadding(kernelSize, hdr.page_size);
    data.insert(data.end(), paddingSize, 0);

    // Ramdisk image
    if (hasRamdiskHdr) {
        data.insert(data.end(),
                    reinterpret_cast<const unsigned char *>(&mtkRamdiskHdr),
                    reinterpret_cast<const unsigned char *>(&mtkRamdiskHdr) + sizeof(MtkHeader));
    }
    data.insert(data.end(),
                mI10e->ramdiskImage.begin(),
                mI10e->ramdiskImage.end());

    // Even more padding
    paddingSize = skipPadding(ramdiskSize, hdr.page_size);
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

    dataOut->swap(data);
    return true;
}

}
