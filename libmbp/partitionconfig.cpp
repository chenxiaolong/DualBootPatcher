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

#include "partitionconfig.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/format.hpp>


/*! \cond INTERNAL */
class PartitionConfig::Impl
{
public:
    std::string name;
    std::string description;
    std::string kernel;
    std::string id;

    std::string targetSystem;
    std::string targetCache;
    std::string targetData;
};
/*! \endcond */


/*!
 * \class PartitionConfig
 * \brief Describes how a patched ROM or kernel should be installed
 *
 * This class tells the patcher how and where to install a patched ROM and
 * kernel. The ROM's system and cache files can be set to install to a
 * subdirectory within the device's /system or /cache partitions.
 */


PartitionConfig::PartitionConfig() : m_impl(new Impl())
{
}

PartitionConfig::~PartitionConfig()
{
}

/*!
 * \brief Partition configuration name
 *
 * \return Name
 */
std::string PartitionConfig::name() const
{
    return m_impl->name;
}

/*!
 * \brief Set partition configuration name
 *
 * \param name Name
 */
void PartitionConfig::setName(std::string name)
{
    m_impl->name = std::move(name);
}

/*!
 * \brief Partition configuration description
 *
 * \return Description
 */
std::string PartitionConfig::description() const
{
    return m_impl->description;
}

/*!
 * \brief Set partition configuration description
 *
 * \param description Description
 */
void PartitionConfig::setDescription(std::string description)
{
    m_impl->description = std::move(description);
}

/*!
 * \brief Kernel identifier
 *
 * `[Kernel ID].img` is the name of the boot image that is installed to
 * `/data/media/0/MultiKernels/`.
 *
 * \return Kernel ID
 */
std::string PartitionConfig::kernel() const
{
    return m_impl->kernel;
}

/*!
 * \brief Set kernel identifier
 *
 * \param kernel Kernel ID
 */
void PartitionConfig::setKernel(std::string kernel)
{
    m_impl->kernel = std::move(kernel);
}

/*!
 * \brief Partition configuration ID
 *
 * \return ID
 */
std::string PartitionConfig::id() const
{
    return m_impl->id;
}

/*!
 * \brief Set partition configuration ID
 *
 * The ID should be unique in whichever container the list of partition
 * configurations are stored.
 *
 * \param id ID
 */
void PartitionConfig::setId(std::string id)
{
    m_impl->id = std::move(id);
}

/*!
 * \brief Source path for /system bind mount
 *
 * \return Source path
 */
std::string PartitionConfig::targetSystem() const
{
    return m_impl->targetSystem;
}

/*!
 * \brief Set source path for /system bind-mount
 *
 * \param path Source path
 */
void PartitionConfig::setTargetSystem(std::string path)
{
    m_impl->targetSystem = std::move(path);
}

/*!
 * \brief Source path for /cache bind mount
 *
 * \return Source path
 */
std::string PartitionConfig::targetCache() const
{
    return m_impl->targetCache;
}

/*!
 * \brief Set source path for /cache bind mount
 *
 * \param path Source path
 */
void PartitionConfig::setTargetCache(std::string path)
{
    m_impl->targetCache = std::move(path);
}

/*!
 * \brief Source path for /data bind mount
 *
 * \return Source path
 */
std::string PartitionConfig::targetData() const
{
    return m_impl->targetData;
}

/*!
 * \brief Set source path for /data bind mount
 *
 * \param path Source path
 */
void PartitionConfig::setTargetData(std::string path)
{
    m_impl->targetData = std::move(path);
}
