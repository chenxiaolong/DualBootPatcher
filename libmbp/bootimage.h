/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <memory>
#include <vector>

#include "libmbp_global.h"
#include "patchererror.h"


namespace mbp
{

class MBP_EXPORT BootImage
{
public:
    static const char *BootMagic;
    static const uint32_t BootMagicSize;
    static const uint32_t BootNameSize;
    static const uint32_t BootArgsSize;

    static const char *DefaultBoard;
    static const char *DefaultCmdline;
    static const uint32_t DefaultPageSize;
    static const uint32_t DefaultBase;
    static const uint32_t DefaultKernelOffset;
    static const uint32_t DefaultRamdiskOffset;
    static const uint32_t DefaultSecondOffset;
    static const uint32_t DefaultTagsOffset;

    BootImage();
    ~BootImage();

    PatcherError error() const;

    bool load(const std::vector<unsigned char> &data);
    bool load(const std::string &filename);
    std::vector<unsigned char> create() const;
    bool createFile(const std::string &path);

    bool wasLoki() const;
    bool wasBump() const;

    void setApplyLoki(bool apply);
    void setApplyBump(bool apply);

    std::string boardName() const;
    void setBoardName(const std::string &name);
    void resetBoardName();

    std::string kernelCmdline() const;
    void setKernelCmdline(const std::string &cmdline);
    void resetKernelCmdline();

    uint32_t pageSize() const;
    void setPageSize(uint32_t size);
    void resetPageSize();

    uint32_t kernelAddress() const;
    void setKernelAddress(uint32_t address);
    void resetKernelAddress();

    uint32_t ramdiskAddress() const;
    void setRamdiskAddress(uint32_t address);
    void resetRamdiskAddress();

    uint32_t secondBootloaderAddress() const;
    void setSecondBootloaderAddress(uint32_t address);
    void resetSecondBootloaderAddress();

    uint32_t kernelTagsAddress() const;
    void setKernelTagsAddress(uint32_t address);
    void resetKernelTagsAddress();

    // Set addresses using a base and offsets
    void setAddresses(uint32_t base, uint32_t kernelOffset,
                      uint32_t ramdiskOffset,
                      uint32_t secondBootloaderOffset,
                      uint32_t kernelTagsOffset);

    // For setting the various images

    std::vector<unsigned char> kernelImage() const;
    void setKernelImage(std::vector<unsigned char> data);

    std::vector<unsigned char> ramdiskImage() const;
    void setRamdiskImage(std::vector<unsigned char> data);

    std::vector<unsigned char> secondBootloaderImage() const;
    void setSecondBootloaderImage(std::vector<unsigned char> data);

    std::vector<unsigned char> deviceTreeImage() const;
    void setDeviceTreeImage(std::vector<unsigned char> data);

    // For Loki only
    std::vector<unsigned char> abootImage() const;
    void setAbootImage(std::vector<unsigned char> data);

    bool operator==(const BootImage &other) const;
    bool operator!=(const BootImage &other) const;


private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}
