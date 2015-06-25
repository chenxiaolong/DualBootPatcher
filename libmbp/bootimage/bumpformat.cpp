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
    return size >= BUMP_MAGIC_SIZE
            && std::memcmp(data + size - BUMP_MAGIC_SIZE,
                           BUMP_MAGIC, BUMP_MAGIC_SIZE) == 0;
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
