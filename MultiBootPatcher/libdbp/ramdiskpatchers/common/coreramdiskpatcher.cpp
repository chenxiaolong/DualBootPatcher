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

#include "coreramdiskpatcher.h"

#include <boost/format.hpp>
#include <json/json.h>

#include "patcherconfig.h"


/*! \cond INTERNAL */
class CoreRamdiskPatcher::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};
/*! \endcond */


const std::string CoreRamdiskPatcher::FstabRegex
        = "^(#.+)?(/dev/\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)";
const std::string CoreRamdiskPatcher::PropPartConfig
        = "ro.patcher.patched=%1%\n";
const std::string CoreRamdiskPatcher::PropVersion
        = "ro.patcher.version=%1%\n";
const std::string CoreRamdiskPatcher::SyncdaemonService
        = "\nservice syncdaemon /sbin/syncdaemon\n"
        "    class main\n"
        "    user root\n";

static const std::string DefaultProp = "default.prop";
static const std::string InitRc = "init.rc";

static const std::string TagVersion = "version";
static const std::string TagPartConfigs = "partconfigs";
static const std::string TagInstalled = "installed";
static const std::string TagPcId = "id";
static const std::string TagPcKernelId = "kernel-id";
static const std::string TagPcName = "name";
static const std::string TagPcDescription = "description";
static const std::string TagPcTargetSystem = "target-system";
static const std::string TagPcTargetCache = "target-cache";
static const std::string TagPcTargetData = "target-data";
static const std::string TagPcTargetSystemPartition = "target-system-partition";
static const std::string TagPcTargetCachePartition = "target-cache-partition";
static const std::string TagPcTargetDataPartition = "target-data-partition";

CoreRamdiskPatcher::CoreRamdiskPatcher(const PatcherConfig * const pc,
                                       const FileInfo * const info,
                                       CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

CoreRamdiskPatcher::~CoreRamdiskPatcher()
{
}

PatcherError CoreRamdiskPatcher::error() const
{
    return m_impl->error;
}

std::string CoreRamdiskPatcher::id() const
{
    return std::string();
}

bool CoreRamdiskPatcher::patchRamdisk()
{
    if (!modifyDefaultProp()) {
        return false;
    }
    if (!addSyncdaemon()) {
        return false;
    }
    if (!addConfigJson()) {
        return false;
    }
    return true;
}

bool CoreRamdiskPatcher::modifyDefaultProp()
{
    auto defaultProp = m_impl->cpio->contents(DefaultProp);
    if (defaultProp.empty()) {
        m_impl->error = PatcherError::createCpioError(
                MBP::ErrorCode::CpioFileNotExistError, DefaultProp);
        return false;
    }

    defaultProp.insert(defaultProp.end(), '\n');

    std::string propPartConfig = (boost::format(PropPartConfig)
            % m_impl->info->partConfig()->id()).str();
    defaultProp.insert(defaultProp.end(),
                       propPartConfig.begin(), propPartConfig.end());

    std::string propVersion = (boost::format(PropVersion)
            % m_impl->pc->version()).str();
    defaultProp.insert(defaultProp.end(),
                       propVersion.begin(), propVersion.end());

    m_impl->cpio->setContents(DefaultProp, std::move(defaultProp));

    return true;
}

bool CoreRamdiskPatcher::addSyncdaemon()
{
    auto initRc = m_impl->cpio->contents(InitRc);
    if (initRc.empty()) {
        m_impl->error = PatcherError::createCpioError(
                MBP::ErrorCode::CpioFileNotExistError, InitRc);
        return false;
    }

    initRc.insert(initRc.end(),
                  SyncdaemonService.begin(), SyncdaemonService.end());

    m_impl->cpio->setContents(InitRc, std::move(initRc));

    return true;
}

bool CoreRamdiskPatcher::addConfigJson()
{
    Json::Value root;
    root[TagVersion] = 1;

    Json::Value jsonPcs(Json::arrayValue);
    for (PartitionConfig *config : m_impl->pc->partitionConfigs()) {
        Json::Value jsonPc;
        jsonPc[TagPcId] = config->id();
        jsonPc[TagPcKernelId] = config->kernel();
        jsonPc[TagPcName] = config->name();
        jsonPc[TagPcDescription] = config->description();
        jsonPc[TagPcTargetSystem] = config->targetSystem();
        jsonPc[TagPcTargetCache] = config->targetCache();
        jsonPc[TagPcTargetData] = config->targetData();

        // TODO: Temporary hack
        std::string tpSystem(config->targetSystemPartition());
        std::string tpCache(config->targetCachePartition());
        std::string tpData(config->targetDataPartition());

        if (tpSystem == PartitionConfig::System) {
            jsonPc[TagPcTargetSystemPartition] = "/system";
        } else if (tpSystem == PartitionConfig::Cache) {
            jsonPc[TagPcTargetSystemPartition] = "/cache";
        } else if (tpSystem == PartitionConfig::Data) {
            jsonPc[TagPcTargetSystemPartition] = "/data";
        } else {
            jsonPc[TagPcTargetSystemPartition] = tpSystem;
        }

        if (tpCache == PartitionConfig::System) {
            jsonPc[TagPcTargetCachePartition] = "/system";
        } else if (tpCache == PartitionConfig::Cache) {
            jsonPc[TagPcTargetCachePartition] = "/cache";
        } else if (tpCache == PartitionConfig::Data) {
            jsonPc[TagPcTargetCachePartition] = "/data";
        } else {
            jsonPc[TagPcTargetCachePartition] = tpCache;
        }

        if (tpData == PartitionConfig::System) {
            jsonPc[TagPcTargetDataPartition] = "/system";
        } else if (tpData == PartitionConfig::Cache) {
            jsonPc[TagPcTargetDataPartition] = "/cache";
        } else if (tpData == PartitionConfig::Data) {
            jsonPc[TagPcTargetDataPartition] = "/data";
        } else {
            jsonPc[TagPcTargetDataPartition] = tpData;
        }

        jsonPcs.append(jsonPc);
    }
    root[TagPartConfigs] = jsonPcs;

    root[TagInstalled] = m_impl->info->partConfig()->id();

    std::string json = root.toStyledString();
    std::vector<unsigned char> data(json.begin(), json.end());
    m_impl->cpio->addFile(std::move(data), "config.json", 0640);

    return true;
}
