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

    std::string targetSystemPartition;
    std::string targetCachePartition;
    std::string targetDataPartition;
};
/*! \endcond */


static const std::string ReplaceLine
        = "# PATCHER REPLACE ME - DO NOT REMOVE";

// Device and mount point for the system, cache, and data filesystems
static const std::string DevSystem
        = "/dev/block/platform/msm_sdcc.1/by-name/system::/raw-system";
static const std::string DevCache
        = "/dev/block/platform/msm_sdcc.1/by-name/cache::/raw-cache";
static const std::string DevData
        = "/dev/block/platform/msm_sdcc.1/by-name/userdata::/raw-data";

/*! \brief System partition */
const std::string PartitionConfig::System = "$DEV_SYSTEM";
/*! \brief Cache partition */
const std::string PartitionConfig::Cache = "$DEV_CACHE";
/*! \brief Data partition */
const std::string PartitionConfig::Data = "$DEV_DATA";


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

/*!
 * \brief Source partition of /system bind mount
 *
 * \return PartitionConfig::System, PartitionConfig::Cache, or
 *         PartitionConfig::Data
 */
std::string PartitionConfig::targetSystemPartition() const
{
    return m_impl->targetSystemPartition;
}

/*!
 * \brief Set source partition of /system bind mount
 *
 * \param partition Source partition
 */
void PartitionConfig::setTargetSystemPartition(std::string partition)
{
    m_impl->targetSystemPartition = std::move(partition);
}

/*!
 * \brief Source partition of /cache bind mount
 *
 * \return PartitionConfig::System, PartitionConfig::Cache, or
 *         PartitionConfig::Data
 */
std::string PartitionConfig::targetCachePartition() const
{
    return m_impl->targetCachePartition;
}

/*!
 * \brief Set source partition of /cache bind mount
 *
 * \param partition Source partition
 */
void PartitionConfig::setTargetCachePartition(std::string partition)
{
    m_impl->targetCachePartition = std::move(partition);
}

/*!
 * \brief Source partition of /data bind mount
 *
 * \return PartitionConfig::System, PartitionConfig::Cache, or
 *         PartitionConfig::Data
 */
std::string PartitionConfig::targetDataPartition() const
{
    return m_impl->targetDataPartition;
}

/*!
 * \brief Set source partition of /cache bind mount
 *
 * \param partition Source partition
 */
void PartitionConfig::setTargetDataPartition(std::string partition)
{
    m_impl->targetDataPartition = std::move(partition);
}

/*!
 * \brief Replace magic string in shell script with partition configuration
 *        variables
 *
 * Any line containing the string, "`# PATCHER REPLACE ME - DO NOT REMOVE`",
 * will be replaced with the following variables:
 *
 * - `KERNEL_NAME`: Kernel ID
 * - `DEV_SYSTEM`: System partition and mount point
 * - `DEV_CACHE`: Cache partition and mount point
 * - `DEV_DATA`: Data partition and mount point
 * - `TARGET_SYSTEM_PARTITION`: Set to `$DEV_SYSTEM`, `$DEV_CACHE`, or `$DEV_DATA`
 * - `TARGET_CACHE_PARTITION`: Set to `$DEV_SYSTEM`, `$DEV_CACHE`, or `$DEV_DATA`
 * - `TARGET_DATA_PARTITION`: Set to `$DEV_SYSTEM`, `$DEV_CACHE`, or `$DEV_DATA`
 * - `TARGET_SYSTEM`: Source directory for /system bind mount
 * - `TARGET_CACHE`: Source directory for /system bind mount
 * - `TARGET_DATA`: Source directory for /system bind mount
 *
 * \param contents Vector containing shell script contents
 * \param targetPathOnly Only insert `TARGET_*` variables
 */
bool PartitionConfig::replaceShellLine(std::vector<unsigned char> *contents,
                                       bool targetPathOnly) const
{
    std::string strContents(contents->begin(), contents->end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if ((*it).find(ReplaceLine) != std::string::npos) {
            it = lines.erase(it);

            if (!targetPathOnly) {
                it = lines.insert(it, (boost::format("KERNEL_NAME=\"%1%\"")
                        % m_impl->kernel).str());

                it = lines.insert(++it,
                        (boost::format("DEV_SYSTEM=\"%1%\"") % DevSystem).str());
                it = lines.insert(++it,
                        (boost::format("DEV_CACHE=\"%1%\"") % DevCache).str());
                it = lines.insert(++it,
                        (boost::format("DEV_DATA=\"%1%\"") % DevData).str());

                it = lines.insert(++it,
                        (boost::format("TARGET_SYSTEM_PARTITION=\"%1%\"")
                        % m_impl->targetSystemPartition).str());
                it = lines.insert(++it,
                        (boost::format("TARGET_CACHE_PARTITION=\"%1%\"")
                        % m_impl->targetCachePartition).str());
                it = lines.insert(++it,
                        (boost::format("TARGET_DATA_PARTITION=\"%1%\"")
                        % m_impl->targetDataPartition).str());

                ++it;
            }

            it = lines.insert(it, (boost::format("TARGET_SYSTEM=\"%1%\"")
                    % m_impl->targetSystem).str());
            it = lines.insert(++it, (boost::format("TARGET_CACHE=\"%1%\"")
                    % m_impl->targetCache).str());
            it = lines.insert(++it, (boost::format("TARGET_DATA=\"%1%\"")
                    % m_impl->targetData).str());
        }
    }

    strContents = boost::join(lines, "\n");
    contents->assign(strContents.begin(), strContents.end());

    return true;
}
