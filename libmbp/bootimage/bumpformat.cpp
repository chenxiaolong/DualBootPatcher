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

#include "bootimage/bumpformat.h"

#include <cstring>

#include "bootimage/bumppatcher.h"
#include "private/logging.h"

namespace mbp
{

BumpFormat::BumpFormat(BootImageIntermediate *i10e) : AndroidFormat(i10e)
{
}

BumpFormat::~BumpFormat()
{
}

bool BumpFormat::isValid(const unsigned char *data, std::size_t size)
{
    // We have to parse the boot image to find the end so we can compare the
    // trailing bytes to the bump magic string

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
    // Skip kernel image
    pos += hdr->kernel_size;
    pos += skipPadding(hdr->kernel_size, hdr->page_size);

    // Check for ramdisk size overflow
    if (pos + hdr->ramdisk_size > size) {
        return false;
    }
    // Skip ramdisk image
    pos += hdr->ramdisk_size;
    pos += skipPadding(hdr->ramdisk_size, hdr->page_size);

    // Check for second bootloader size overflow
    if (pos + hdr->second_size > size) {
        return false;
    }
    // Skip second bootloader image
    pos += hdr->second_size;
    pos += skipPadding(hdr->second_size, hdr->page_size);

    // Check for device tree image size overflow
    if (pos + hdr->dt_size > size) {
        return false;
    }
    // Skip device tree image
    pos += hdr->dt_size;
    pos += skipPadding(hdr->dt_size, hdr->page_size);

    // We are now at the end of the boot image, so check for the bump magic
    return (size >= pos + BUMP_MAGIC_SIZE)
            && std::memcmp(data + pos, BUMP_MAGIC, BUMP_MAGIC_SIZE) == 0;
}

bool BumpFormat::createImage(std::vector<unsigned char> *dataOut)
{
    std::vector<unsigned char> data;
    if (!AndroidFormat::createImage(&data)) {
        return false;
    }

    if (!BumpPatcher::patchImage(&data)) {
        return false;
    }

    dataOut->swap(data);
    return true;
}

}
