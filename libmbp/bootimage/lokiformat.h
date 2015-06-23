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

#pragma once

#include "bootimage/androidformat.h"
#include "bootimage/lokipatcher.h"

namespace mbp
{

class LokiFormat : public AndroidFormat
{
public:
    LokiFormat(BootImageIntermediate *i10e);
    virtual ~LokiFormat();

    static bool isValid(const unsigned char *data, std::size_t size);

    virtual bool loadImage(const unsigned char *data, std::size_t size) override;

    virtual bool createImage(std::vector<unsigned char> *dataOut) override;

    ///

    bool loadLokiNewImage(const unsigned char *data, std::size_t size,
                          const LokiHeader *loki);
    bool loadLokiOldImage(const unsigned char *data, std::size_t size,
                          const LokiHeader *loki);
    uint32_t lokiOldFindGzipOffset(const unsigned char *data, std::size_t size,
                                   const uint32_t startOffset) const;
    uint32_t lokiOldFindRamdiskSize(const unsigned char *data, std::size_t size,
                                    const uint32_t ramdiskOffset) const;
    uint32_t lokiFindRamdiskAddress(const unsigned char *data, std::size_t size,
                                    const LokiHeader *loki) const;
};

}
