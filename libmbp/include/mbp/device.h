/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include <string>
#include <vector>

#include "mbcommon/common.h"

#define ARCH_ARMEABI_V7A        "armeabi-v7a"
#define ARCH_ARM64_V8A          "arm64-v8a"
#define ARCH_X86                "x86"
#define ARCH_X86_64             "x86_64"

namespace mbp
{

class MB_EXPORT Device
{
public:
    enum Flags
    {
        FLAG_HAS_COMBINED_BOOT_AND_RECOVERY     = 0x1
    };

    Device();
    ~Device();

    std::string id() const;
    void setId(std::string id);
    std::vector<std::string> codenames() const;
    void setCodenames(std::vector<std::string> names);
    std::string name() const;
    void setName(std::string name);
    std::string architecture() const;
    void setArchitecture(std::string arch);
    uint64_t flags() const;
    void setFlags(uint64_t flags);
    std::string ramdiskPatcher() const;
    void setRamdiskPatcher(std::string id);
    std::vector<std::string> blockDevBaseDirs() const;
    void setBlockDevBaseDirs(std::vector<std::string> dirs);
    std::vector<std::string> systemBlockDevs() const;
    void setSystemBlockDevs(std::vector<std::string> blockDevs);
    std::vector<std::string> cacheBlockDevs() const;
    void setCacheBlockDevs(std::vector<std::string> blockDevs);
    std::vector<std::string> dataBlockDevs() const;
    void setDataBlockDevs(std::vector<std::string> blockDevs);
    std::vector<std::string> bootBlockDevs() const;
    void setBootBlockDevs(std::vector<std::string> blockDevs);
    std::vector<std::string> recoveryBlockDevs() const;
    void setRecoveryBlockDevs(std::vector<std::string> blockDevs);
    std::vector<std::string> extraBlockDevs() const;
    void setExtraBlockDevs(std::vector<std::string> blockDevs);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}
