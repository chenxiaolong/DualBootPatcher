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

#include "mbp/device.h"

#include <unordered_map>

#include "mbp/ramdiskpatchers/default.h"


namespace mbp
{

/*! \cond INTERNAL */
class Device::Impl
{
public:
    std::string id;
    std::vector<std::string> codenames;
    std::string name;
    std::string architecture;
    uint64_t flags = 0;
    std::string defaultRP;

    std::vector<std::string> baseDirs;

    std::vector<std::string> systemDevs;
    std::vector<std::string> cacheDevs;
    std::vector<std::string> dataDevs;
    std::vector<std::string> bootDevs;
    std::vector<std::string> recoveryDevs;
    std::vector<std::string> extraDevs;
};
/*! \endcond */


/*!
 * \class Device
 * \brief Simple class containing information about a supported device
 */

Device::Device() : m_impl(new Impl())
{
    m_impl->architecture = ARCH_ARMEABI_V7A;
    m_impl->defaultRP = DefaultRP::Id;
}

Device::~Device()
{
}

std::string Device::id() const
{
    return m_impl->id;
}

void Device::setId(std::string id)
{
    m_impl->id = std::move(id);
}

/*!
 * \brief Device's codenames
 *
 * \return Device codenames
 */
std::vector<std::string> Device::codenames() const
{
    return m_impl->codenames;
}

/*!
 * \brief Set the device's codenames
 *
 * \param names Codenames
 */
void Device::setCodenames(std::vector<std::string> names)
{
    m_impl->codenames = std::move(names);
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

uint64_t Device::flags() const
{
    return m_impl->flags;
}

void Device::setFlags(uint64_t flags)
{
    m_impl->flags = flags;
}

std::string Device::ramdiskPatcher() const
{
    return m_impl->defaultRP;
}

void Device::setRamdiskPatcher(std::string id)
{
    m_impl->defaultRP = std::move(id);
}

std::vector<std::string> Device::blockDevBaseDirs() const
{
    return m_impl->baseDirs;
}

void Device::setBlockDevBaseDirs(std::vector<std::string> dirs)
{
    m_impl->baseDirs = std::move(dirs);
}

std::vector<std::string> Device::systemBlockDevs() const
{
    return m_impl->systemDevs;
}

void Device::setSystemBlockDevs(std::vector<std::string> blockDevs)
{
    m_impl->systemDevs = std::move(blockDevs);
}

std::vector<std::string> Device::cacheBlockDevs() const
{
    return m_impl->cacheDevs;
}

void Device::setCacheBlockDevs(std::vector<std::string> blockDevs)
{
    m_impl->cacheDevs = std::move(blockDevs);
}

std::vector<std::string> Device::dataBlockDevs() const
{
    return m_impl->dataDevs;
}

void Device::setDataBlockDevs(std::vector<std::string> blockDevs)
{
    m_impl->dataDevs = std::move(blockDevs);
}

std::vector<std::string> Device::bootBlockDevs() const
{
    return m_impl->bootDevs;
}

void Device::setBootBlockDevs(std::vector<std::string> blockDevs)
{
    m_impl->bootDevs = std::move(blockDevs);
}

std::vector<std::string> Device::recoveryBlockDevs() const
{
    return m_impl->recoveryDevs;
}

void Device::setRecoveryBlockDevs(std::vector<std::string> blockDevs)
{
    m_impl->recoveryDevs = std::move(blockDevs);
}

std::vector<std::string> Device::extraBlockDevs() const
{
    return m_impl->extraDevs;
}

void Device::setExtraBlockDevs(std::vector<std::string> blockDevs)
{
    m_impl->extraDevs = std::move(blockDevs);
}

}
