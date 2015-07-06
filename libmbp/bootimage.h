/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "bootimage-common.h"
#include "libmbp_global.h"
#include "patchererror.h"


namespace mbp
{

class MBP_EXPORT BootImage
{
public:
    static const char *AndroidBootMagic;
    static const uint32_t AndroidBootMagicSize;
    static const uint32_t AndroidBootNameSize;
    static const uint32_t AndroidBootArgsSize;

    // Universal defaults
    static const char *DefaultCmdline;

    // Android-based boot image defaults
    static const char *AndroidDefaultBoard;
    static const uint32_t AndroidDefaultPageSize;
    static const uint32_t AndroidDefaultBase;
    static const uint32_t AndroidDefaultKernelOffset;
    static const uint32_t AndroidDefaultRamdiskOffset;
    static const uint32_t AndroidDefaultSecondOffset;
    static const uint32_t AndroidDefaultTagsOffset;

    // Sony ELF boot image defaults
    static const uint32_t SonyElfDefaultKernelAddress;
    static const uint32_t SonyElfDefaultRamdiskAddress;
    static const uint32_t SonyElfDefaultIplAddress;
    static const uint32_t SonyElfDefaultRpmAddress;
    static const uint32_t SonyElfDefaultAppsblAddress;
    static const uint32_t SonyElfDefaultEntrypointAddress;

    enum class Type : int
    {
        Android = 1,
        Loki = 2,
        Bump = 3,
        SonyElf = 4
    };

    BootImage();
    ~BootImage();

    PatcherError error() const;

    static bool isValid(const unsigned char *data, std::size_t size);

    bool load(const unsigned char *data, std::size_t size);
    bool load(const std::vector<unsigned char> &data);
    bool loadFile(const std::string &filename);
    bool create(std::vector<unsigned char> *data) const;
    bool createFile(const std::string &path);

    Type wasType() const;
    Type targetType() const;
    void setTargetType(Type type);

    static uint64_t typeSupportMask(Type type);

    // Board name
    const std::string & boardName() const;
    void setBoardName(std::string name);
    const char * boardNameC() const;
    void setBoardNameC(const char *name);

    // Kernel cmdline
    const std::string & kernelCmdline() const;
    void setKernelCmdline(std::string cmdline);
    const char * kernelCmdlineC() const;
    void setKernelCmdlineC(const char *cmdline);

    // Page size
    uint32_t pageSize() const;
    void setPageSize(uint32_t size);

    // Kernel address
    uint32_t kernelAddress() const;
    void setKernelAddress(uint32_t address);

    // Ramdisk address
    uint32_t ramdiskAddress() const;
    void setRamdiskAddress(uint32_t address);

    // Second bootloader address
    uint32_t secondBootloaderAddress() const;
    void setSecondBootloaderAddress(uint32_t address);

    // Kernel tags address
    uint32_t kernelTagsAddress() const;
    void setKernelTagsAddress(uint32_t address);

    // Sony ipl address
    uint32_t iplAddress() const;
    void setIplAddress(uint32_t address);

    // Sony rpm address
    uint32_t rpmAddress() const;
    void setRpmAddress(uint32_t address);

    // Sony appsbl address
    uint32_t appsblAddress() const;
    void setAppsblAddress(uint32_t address);

    // Sony entrypoint address
    uint32_t entrypointAddress() const;
    void setEntrypointAddress(uint32_t address);

    // Kernel image
    const std::vector<unsigned char> & kernelImage() const;
    void setKernelImage(std::vector<unsigned char> data);
    void kernelImageC(const unsigned char **data, std::size_t *size) const;
    void setKernelImageC(const unsigned char *data, std::size_t size);

    // Ramdisk image
    const std::vector<unsigned char> & ramdiskImage() const;
    void setRamdiskImage(std::vector<unsigned char> data);
    void ramdiskImageC(const unsigned char **data, std::size_t *size) const;
    void setRamdiskImageC(const unsigned char *data, std::size_t size);

    // Second bootloader image
    const std::vector<unsigned char> & secondBootloaderImage() const;
    void setSecondBootloaderImage(std::vector<unsigned char> data);
    void secondBootloaderImageC(const unsigned char **data, std::size_t *size) const;
    void setSecondBootloaderImageC(const unsigned char *data, std::size_t size);

    // Device tree image
    const std::vector<unsigned char> & deviceTreeImage() const;
    void setDeviceTreeImage(std::vector<unsigned char> data);
    void deviceTreeImageC(const unsigned char **data, std::size_t *size) const;
    void setDeviceTreeImageC(const unsigned char *data, std::size_t size);

    // Aboot image
    const std::vector<unsigned char> & abootImage() const;
    void setAbootImage(std::vector<unsigned char> data);
    void abootImageC(const unsigned char **data, std::size_t *size) const;
    void setAbootImageC(const unsigned char *data, std::size_t size);

    // Sony ipl image
    const std::vector<unsigned char> & iplImage() const;
    void setIplImage(std::vector<unsigned char> data);
    void iplImageC(const unsigned char **data, std::size_t *size) const;
    void setIplImageC(const unsigned char *data, std::size_t size);

    // Sony rpm image
    const std::vector<unsigned char> & rpmImage() const;
    void setRpmImage(std::vector<unsigned char> data);
    void rpmImageC(const unsigned char **data, std::size_t *size) const;
    void setRpmImageC(const unsigned char *data, std::size_t size);

    // Sony appsbl image
    const std::vector<unsigned char> & appsblImage() const;
    void setAppsblImage(std::vector<unsigned char> data);
    void appsblImageC(const unsigned char **data, std::size_t *size) const;
    void setAppsblImageC(const unsigned char *data, std::size_t size);

    // Sony SIN! image
    const std::vector<unsigned char> & sinImage() const;
    void setSinImage(std::vector<unsigned char> data);
    void sinImageC(const unsigned char **data, std::size_t *size) const;
    void setSinImageC(const unsigned char *data, std::size_t size);

    // Sony SIN! header
    const std::vector<unsigned char> & sinHeader() const;
    void setSinHeader(std::vector<unsigned char> data);
    void sinHeaderC(const unsigned char **data, std::size_t *size) const;
    void setSinHeaderC(const unsigned char *data, std::size_t size);

    bool operator==(const BootImage &other) const;
    bool operator!=(const BootImage &other) const;

    BootImage(const BootImage &) = delete;
    BootImage(BootImage &&) = default;
    BootImage & operator=(const BootImage &) & = delete;
    BootImage & operator=(BootImage &&) & = default;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}
