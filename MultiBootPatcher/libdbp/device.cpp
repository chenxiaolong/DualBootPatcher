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

#include "device.h"

#include <unordered_map>


class Device::Impl
{
public:
    std::string codename;
    std::string name;
    std::string architecture;
    std::string selinux;

    std::unordered_map<std::string, std::string> partitions;
};


const std::string Device::SelinuxPermissive = "permissive";
const std::string Device::SelinuxUnchanged = "unchanged";
const std::string Device::SystemPartition = "system";
const std::string Device::CachePartition = "cache";
const std::string Device::DataPartition = "data";


Device::Device() : m_impl(new Impl())
{
    // Other architectures currently aren't supported
    m_impl->architecture = "armeabi-v7a";
}

Device::~Device()
{
}

std::string Device::codename() const
{
    return m_impl->codename;
}

void Device::setCodename(std::string name)
{
    m_impl->codename = std::move(name);
}

std::string Device::name() const
{
    return m_impl->name;
}

void Device::setName(std::string name)
{
    m_impl->name = std::move(name);
}

std::string Device::architecture() const
{
    return m_impl->architecture;
}

void Device::setArchitecture(std::string arch)
{
    m_impl->architecture = std::move(arch);
}

std::string Device::selinux() const
{
    if (m_impl->selinux == SelinuxUnchanged) {
        return std::string();
    }

    return m_impl->selinux;
}

void Device::setSelinux(std::string selinux)
{
    m_impl->selinux = std::move(selinux);
}

std::string Device::partition(const std::string &which) const
{
    if (m_impl->partitions.find(which) != m_impl->partitions.end()) {
        return m_impl->partitions[which];
    } else {
        return std::string();
    }
}

void Device::setPartition(const std::string &which, std::string partition)
{
    m_impl->partitions[which] = std::move(partition);
}

std::vector<std::string> Device::partitionTypes() const
{
    std::vector<std::string> keys;

    for (auto const &p :  m_impl->partitions) {
        keys.push_back(p.first);
    }

    return keys;
}
