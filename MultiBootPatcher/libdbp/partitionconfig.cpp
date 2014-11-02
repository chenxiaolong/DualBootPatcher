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


const std::string PartitionConfig::ReplaceLine
        = "# PATCHER REPLACE ME - DO NOT REMOVE";

// Device and mount point for the system, cache, and data filesystems
const std::string PartitionConfig::DevSystem
        = "/dev/block/platform/msm_sdcc.1/by-name/system::/raw-system";
const std::string PartitionConfig::DevCache
        = "/dev/block/platform/msm_sdcc.1/by-name/cache::/raw-cache";
const std::string PartitionConfig::DevData
        = "/dev/block/platform/msm_sdcc.1/by-name/userdata::/raw-data";

const std::string PartitionConfig::System = "$DEV_SYSTEM";
const std::string PartitionConfig::Cache = "$DEV_CACHE";
const std::string PartitionConfig::Data = "$DEV_DATA";

PartitionConfig::PartitionConfig() : m_impl(new Impl())
{
}

PartitionConfig::~PartitionConfig()
{
}

std::string PartitionConfig::name() const
{
    return m_impl->name;
}

void PartitionConfig::setName(std::string name)
{
    m_impl->name = std::move(name);
}

std::string PartitionConfig::description() const
{
    return m_impl->description;
}

void PartitionConfig::setDescription(std::string description)
{
    m_impl->description = std::move(description);
}

std::string PartitionConfig::kernel() const
{
    return m_impl->kernel;
}

void PartitionConfig::setKernel(std::string kernel)
{
    m_impl->kernel = std::move(kernel);
}

std::string PartitionConfig::id() const
{
    return m_impl->id;
}

void PartitionConfig::setId(std::string id)
{
    m_impl->id = std::move(id);
}

std::string PartitionConfig::targetSystem() const
{
    return m_impl->targetSystem;
}

void PartitionConfig::setTargetSystem(std::string path)
{
    m_impl->targetSystem = std::move(path);
}

std::string PartitionConfig::targetCache() const
{
    return m_impl->targetCache;
}

void PartitionConfig::setTargetCache(std::string path)
{
    m_impl->targetCache = std::move(path);
}

std::string PartitionConfig::targetData() const
{
    return m_impl->targetData;
}

void PartitionConfig::setTargetData(std::string path)
{
    m_impl->targetData = std::move(path);
}

std::string PartitionConfig::targetSystemPartition() const
{
    return m_impl->targetSystemPartition;
}

void PartitionConfig::setTargetSystemPartition(std::string partition)
{
    m_impl->targetSystemPartition = std::move(partition);
}

std::string PartitionConfig::targetCachePartition() const
{
    return m_impl->targetCachePartition;
}

void PartitionConfig::setTargetCachePartition(std::string partition)
{
    m_impl->targetCachePartition = std::move(partition);
}

std::string PartitionConfig::targetDataPartition() const
{
    return m_impl->targetDataPartition;
}

void PartitionConfig::setTargetDataPartition(std::string partition)
{
    m_impl->targetDataPartition = std::move(partition);
}

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
