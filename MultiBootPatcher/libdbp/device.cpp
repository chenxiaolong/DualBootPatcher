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


/*! \cond INTERNAL */
class Device::Impl
{
public:
    std::string codename;
    std::string name;
    std::string architecture;
    std::string selinux;

    std::unordered_map<std::string, std::string> partitions;
};
/*! \endcond */


/*! \brief SELinux permissive mode */
const std::string Device::SelinuxPermissive = "permissive";
/*! \brief Leave SELinux mode unchanged */
const std::string Device::SelinuxUnchanged = "unchanged";
/*! \brief System partition */
const std::string Device::SystemPartition = "system";
/*! \brief Cache partition */
const std::string Device::CachePartition = "cache";
/*! \brief Data partition */
const std::string Device::DataPartition = "data";


/*!
 * \class Device
 * \brief Simple class containing information about a supported device
 *
 * This class stores the following information:
 *
 * - Device codename (eg. jflte)
 * - Device name (eg. Samsung Galaxy S 4)
 * - CPU architecture (eg. armeabi-v7a)
 * - SELinux mode (eg. enforcing)
 * - Partition numbers for the system, cache, and data partitions
 */

Device::Device() : m_impl(new Impl())
{
    // Other architectures currently aren't supported
    m_impl->architecture = "armeabi-v7a";
}

Device::~Device()
{
}

/*!
 * \brief Device's codename
 *
 * \return Device codename
 */
std::string Device::codename() const
{
    return m_impl->codename;
}

/*!
 * \brief Set the device's codename
 *
 * \param name Codename
 */
void Device::setCodename(std::string name)
{
    m_impl->codename = std::move(name);
}

/*!
 * \brief Device's full name
 *
 * \return Device name
 */
std::string Device::name() const
{
    return m_impl->name;
}

/*!
 * \brief Set the device's full name
 *
 * \param name Name
 */
void Device::setName(std::string name)
{
    m_impl->name = std::move(name);
}

/*!
 * \brief Device's CPU architecture
 *
 * \note The patcher only supports `armeabi-v7a` at the moment.
 *
 * \return Architecture
 */
std::string Device::architecture() const
{
    return m_impl->architecture;
}

/*!
 * \brief Set the device's CPU architecture
 *
 * \note The patcher only supports `armeabi-v7a` at the moment.
 *
 * \param arch Architecture
 */
void Device::setArchitecture(std::string arch)
{
    m_impl->architecture = std::move(arch);
}

/*!
 * \brief Device's SELinux mode
 *
 * \return SELinux mode
 */
std::string Device::selinux() const
{
    if (m_impl->selinux == SelinuxUnchanged) {
        return std::string();
    }

    return m_impl->selinux;
}

/*!
 * \brief Set the device's SELinux mode
 *
 * \param selinux SELinux mode
 */
void Device::setSelinux(std::string selinux)
{
    m_impl->selinux = std::move(selinux);
}

/*!
 * \brief Get partition number for a specific partition
 *
 * \param which Partition
 *
 * \return Partition
 */
std::string Device::partition(const std::string &which) const
{
    if (m_impl->partitions.find(which) != m_impl->partitions.end()) {
        return m_impl->partitions[which];
    } else {
        return std::string();
    }
}

/*!
 * \brief Set the partition number for a specific partition
 *
 * \param which Partition
 * \param partition Partition number (eg. `mmcblk16`)
 */
void Device::setPartition(const std::string &which, std::string partition)
{
    m_impl->partitions[which] = std::move(partition);
}

/*!
 * \brief List of partition types with partition numbers assigned
 *
 * \return List of partition types
 */
std::vector<std::string> Device::partitionTypes() const
{
    std::vector<std::string> keys;

    for (auto const &p :  m_impl->partitions) {
        keys.push_back(p.first);
    }

    return keys;
}
