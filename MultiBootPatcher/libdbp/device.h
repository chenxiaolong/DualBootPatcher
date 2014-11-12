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

#ifndef DEVICE_H
#define DEVICE_H

#include <memory>
#include <vector>

#include "libdbp_global.h"


class MBP_EXPORT Device
{
public:
    static const std::string SelinuxPermissive;
    static const std::string SelinuxUnchanged;
    static const std::string SystemPartition;
    static const std::string CachePartition;
    static const std::string DataPartition;

    Device();
    ~Device();

    std::string codename() const;
    void setCodename(std::string name);
    std::string name() const;
    void setName(std::string name);
    std::string architecture() const;
    void setArchitecture(std::string arch);
    std::string selinux() const;
    void setSelinux(std::string selinux);
    std::string partition(const std::string &which) const;
    void setPartition(const std::string &which, std::string partition);
    std::vector<std::string> partitionTypes() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // DEVICE_H
