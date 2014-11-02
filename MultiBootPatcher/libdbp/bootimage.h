/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#ifndef BOOTIMAGE_H
#define BOOTIMAGE_H

#include <memory>
#include <vector>

#include "libdbp_global.h"
#include "patchererror.h"


struct LokiHeader;

class LIBDBPSHARED_EXPORT BootImage
{
public:
    BootImage();
    ~BootImage();

    PatcherError error() const;

    bool load(const std::vector<unsigned char> &data);
    bool load(const std::string &filename);
    std::vector<unsigned char> create() const;
    bool createFile(const std::string &path);
    bool extract(const std::string &directory, const std::string &prefix);

    std::string boardName() const;
    void setBoardName(const std::string &name);
    void resetBoardName();

    std::string kernelCmdline() const;
    void setKernelCmdline(const std::string &cmdline);
    void resetKernelCmdline();

    unsigned int pageSize() const;
    void setPageSize(unsigned int size);
    void resetPageSize();

    unsigned int kernelAddress() const;
    void setKernelAddress(unsigned int address);
    void resetKernelAddress();

    unsigned int ramdiskAddress() const;
    void setRamdiskAddress(unsigned int address);
    void resetRamdiskAddress();

    unsigned int secondBootloaderAddress() const;
    void setSecondBootloaderAddress(unsigned int address);
    void resetSecondBootloaderAddress();

    unsigned int kernelTagsAddress() const;
    void setKernelTagsAddress(unsigned int address);
    void resetKernelTagsAddress();

    // Set addresses using a base and offsets
    void setAddresses(unsigned int base, unsigned int kernelOffset,
                      unsigned int ramdiskOffset,
                      unsigned int secondBootloaderOffset,
                      unsigned int kernelTagsOffset);

    // For setting the various images

    std::vector<unsigned char> kernelImage() const;
    void setKernelImage(std::vector<unsigned char> data);

    std::vector<unsigned char> ramdiskImage() const;
    void setRamdiskImage(std::vector<unsigned char> data);

    std::vector<unsigned char> secondBootloaderImage() const;
    void setSecondBootloaderImage(std::vector<unsigned char> data);

    std::vector<unsigned char> deviceTreeImage() const;
    void setDeviceTreeImage(std::vector<unsigned char> data);


private:
    bool loadAndroidHeader(const std::vector<unsigned char> &data,
                           const int headerIndex,
                           const bool isLoki);
    bool loadLokiHeader(const std::vector< unsigned char> &data,
                        const int headerIndex,
                        const bool isLoki);
    int lokiFindGzipOffset(const std::vector<unsigned char> &data) const;
    int lokiFindRamdiskSize(const std::vector<unsigned char> &data,
                            const LokiHeader *loki,
                            const int &ramdiskOffset) const;
    int lokiFindKernelSize(const std::vector<unsigned char> &data,
                           const LokiHeader *loki) const;
    unsigned int lokiFindRamdiskAddress(const std::vector<unsigned char> &data,
                                        const LokiHeader *loki) const;
    int skipPadding(const int &itemSize,
                    const int &pageSize) const;
    void updateSHA1Hash();

    void dumpHeader() const;

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // BOOTIMAGE_H
