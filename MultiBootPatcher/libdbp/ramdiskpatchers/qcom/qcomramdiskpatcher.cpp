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

#include "ramdiskpatchers/qcom/qcomramdiskpatcher.h"

#include <cassert>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/any.hpp>
#include <boost/format.hpp>

#include "ramdiskpatchers/common/coreramdiskpatcher.h"
#include "private/logging.h"
#include "private/regex.h"


/*! \cond INTERNAL */
class QcomRamdiskPatcher::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};
/*! \endcond */


const std::string QcomRamdiskPatcher::SystemPartition =
        "/dev/block/platform/msm_sdcc.1/by-name/system";
const std::string QcomRamdiskPatcher::CachePartition =
        "/dev/block/platform/msm_sdcc.1/by-name/cache";
const std::string QcomRamdiskPatcher::DataPartition =
        "/dev/block/platform/msm_sdcc.1/by-name/userdata";

const std::string QcomRamdiskPatcher::ArgAdditionalFstabs =
        "AdditionalFstabs";
const std::string QcomRamdiskPatcher::ArgForceSystemRw =
        "ForceSystemRw";
const std::string QcomRamdiskPatcher::ArgForceCacheRw =
        "ForceCacheRw";
const std::string QcomRamdiskPatcher::ArgForceDataRw =
        "ForceDataRw";
const std::string QcomRamdiskPatcher::ArgKeepMountPoints =
        "KeepMountPoints";
const std::string QcomRamdiskPatcher::ArgSystemMountPoint =
        "SystemMountPoint";
const std::string QcomRamdiskPatcher::ArgCacheMountPoint =
        "CacheMountPoint";
const std::string QcomRamdiskPatcher::ArgDataMountPoint =
        "DataMountPoint";
const std::string QcomRamdiskPatcher::ArgDefaultSystemMountArgs =
        "DefaultSystemMountArgs";
const std::string QcomRamdiskPatcher::ArgDefaultSystemVoldArgs =
        "DefaultSystemVoldArgs";
const std::string QcomRamdiskPatcher::ArgDefaultCacheMountArgs =
        "DefaultCacheMountArgs";
const std::string QcomRamdiskPatcher::ArgDefaultCacheVoldArgs =
        "DefaultDataMountArgs";

static const std::string RawSystem = "/raw-system";
static const std::string RawCache = "/raw-cache";
static const std::string RawData = "/raw-data";
static const std::string System = "/system";
static const std::string Cache = "/cache";
static const std::string Data = "/data";


QcomRamdiskPatcher::QcomRamdiskPatcher(const PatcherConfig * const pc,
                                       const FileInfo * const info,
                                       CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

QcomRamdiskPatcher::~QcomRamdiskPatcher()
{
}

PatcherError QcomRamdiskPatcher::error() const
{
    return m_impl->error;
}

std::string QcomRamdiskPatcher::id() const
{
    return std::string();
}

bool QcomRamdiskPatcher::patchRamdisk()
{
    return false;
}

bool QcomRamdiskPatcher::modifyInitRc()
{
    static const std::string initRc("init.rc");

    auto contents = m_impl->cpio->contents(initRc);
    if (contents.empty()) {
        m_impl->error = PatcherError::createCpioError(
                MBP::ErrorCode::CpioFileNotExistError, initRc);
        return false;
    }

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (MBP_regex_search(*it, MBP_regex("mkdir /system(\\s|$)"))) {
            auto newLine = boost::replace_all_copy(*it, System, RawSystem);
            it = lines.insert(++it, newLine);
        } else if (MBP_regex_search(*it, MBP_regex("mkdir /cache(\\s|$)"))) {
            auto newLine = boost::replace_all_copy(*it, Cache, RawCache);
            it = lines.insert(++it, newLine);
        } else if (MBP_regex_search(*it, MBP_regex("mkdir /data(\\s|$)"))) {
            auto newLine = boost::replace_all_copy(*it, Data, RawData);
            it = lines.insert(++it, newLine);
        } else if (it->find("yaffs2") != std::string::npos) {
            it->insert(it->begin(), '#');
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(initRc, std::move(contents));

    return true;
}

bool QcomRamdiskPatcher::modifyInitQcomRc(const std::vector<std::string> &additionalFiles)
{
    static const std::string qcomRc("init.qcom.rc");
    static const std::string qcomCommonRc("init.qcom-common.rc");

    std::vector<std::string> files;
    if (m_impl->cpio->exists(qcomRc)) {
        files.push_back(qcomRc);
    } else if (m_impl->cpio->exists(qcomCommonRc)) {
        files.push_back(qcomCommonRc);
    } else {
        m_impl->error = PatcherError::createCpioError(
                MBP::ErrorCode::CpioFileNotExistError,
                qcomRc + ", " + qcomCommonRc);
        return false;
    }
    files.insert(files.end(), additionalFiles.begin(), additionalFiles.end());

    for (auto const &file : files) {
        auto contents = m_impl->cpio->contents(file);
        if (contents.empty()) {
            m_impl->error = PatcherError::createCpioError(
                    MBP::ErrorCode::CpioFileNotExistError, file);
            return false;
        }

        std::string strContents(contents.begin(), contents.end());
        std::vector<std::string> lines;
        boost::split(lines, strContents, boost::is_any_of("\n"));

        for (auto it = lines.begin(); it != lines.end(); ++it) {
            if (MBP_regex_search(*it, MBP_regex("\\s/data/media(\\s|$)"))) {
                boost::replace_all(*it, "/data/media", "/raw-data/media");
            }
        }

        strContents = boost::join(lines, "\n");
        contents.assign(strContents.begin(), strContents.end());
        m_impl->cpio->setContents(file, std::move(contents));
    }

    return true;
}

// Qt port of the C++ Levenshtein implementation from WikiBooks:
// http://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance
template<class T>
static unsigned int levenshteinDistance(const T &s1, const T &s2)
{
    const std::size_t len1 = s1.size();
    const std::size_t len2 = s2.size();

    std::vector<unsigned int> col(len2 + 1);
    std::vector<unsigned int> prevCol(len2 + 1);

    for (unsigned int i = 0; i < prevCol.size(); ++i) {
        prevCol[i] = i;
    }

    for (unsigned int i = 0; i < len1; ++i) {
        col[0] = i + 1;
        for (unsigned int j = 0; j < len2; ++j) {
            col[j + 1] = std::min(std::min(prevCol[1 + j] + 1, col[j] + 1),
                                           prevCol[j] + (s1[i] == s2[j] ? 0 : 1));
        }
        col.swap(prevCol);
    }

    return prevCol[len2];
}

static std::string closestMatch(const std::string &searchTerm,
                                const std::vector<std::string> &list)
{
    int index = 0;
    unsigned int min = levenshteinDistance(searchTerm, list[0]);

    for (unsigned int i = 1; i < list.size(); ++i) {
        unsigned int distance = levenshteinDistance(searchTerm, list[i]);
        if (distance < min) {
            min = distance;
            index = i;
        }
    }

    return list[index];
}

template<class MapContainer>
static std::vector<typename MapContainer::key_type> keys(const MapContainer &m) {
    std::vector<typename MapContainer::key_type> keys;

    for (auto const &p : m) {
        keys.push_back(p.first);
    }

    return keys;
}

bool QcomRamdiskPatcher::modifyFstab(bool removeModemMounts)
{
    return modifyFstab(FstabArgs(), removeModemMounts);
}

bool QcomRamdiskPatcher::modifyFstab(FstabArgs args,
                                     bool removeModemMounts)
{
    std::vector<std::string> additionalFstabs;
    bool forceSystemRw = false;
    bool forceCacheRw = false;
    bool forceDataRw = false;
    bool keepMountPoints = false;
    std::string systemMountPoint = System;
    std::string cacheMountPoint = Cache;
    std::string dataMountPoint = Data;
    std::string defaultSystemMountArgs = "ro,barrier=1,errors=panic";
    std::string defaultSystemVoldArgs = "wait";
    std::string defaultCacheMountArgs = "nosuid,nodev,barrier=1";
    std::string defaultCacheVoldArgs = "wait,check";

    try {
        if (args.find(ArgAdditionalFstabs) != args.end()) {
            additionalFstabs = boost::any_cast<std::vector<std::string>>(
                    args[ArgAdditionalFstabs]);
        }

        if (args.find(ArgForceSystemRw) != args.end()) {
            forceSystemRw = boost::any_cast<bool>(args[ArgForceSystemRw]);
        }

        if (args.find(ArgForceCacheRw) != args.end()) {
            forceCacheRw = boost::any_cast<bool>(args[ArgForceCacheRw]);
        }

        if (args.find(ArgForceDataRw) != args.end()) {
            forceDataRw = boost::any_cast<bool>(args[ArgForceDataRw]);
        }

        if (args.find(ArgKeepMountPoints) != args.end()) {
            keepMountPoints = boost::any_cast<bool>(args[ArgKeepMountPoints]);
        }

        if (args.find(ArgSystemMountPoint) != args.end()) {
            systemMountPoint = boost::any_cast<std::string>(
                    args[ArgSystemMountPoint]);
        }

        if (args.find(ArgCacheMountPoint) != args.end()) {
            cacheMountPoint = boost::any_cast<std::string>(
                    args[ArgCacheMountPoint]);
        }

        if (args.find(ArgDataMountPoint) != args.end()) {
            dataMountPoint = boost::any_cast<std::string>(
                    args[ArgDataMountPoint]);
        }

        if (args.find(ArgDefaultSystemMountArgs) != args.end()) {
            defaultSystemMountArgs = boost::any_cast<std::string>(
                    args[ArgDefaultSystemMountArgs]);
        }

        if (args.find(ArgDefaultSystemVoldArgs) != args.end()) {
            defaultSystemVoldArgs = boost::any_cast<std::string>(
                    args[ArgDefaultSystemVoldArgs]);
        }

        if (args.find(ArgDefaultCacheMountArgs) != args.end()) {
            defaultCacheMountArgs = boost::any_cast<std::string>(
                    args[ArgDefaultCacheMountArgs]);
        }

        if (args.find(ArgDefaultCacheVoldArgs) != args.end()) {
            defaultCacheVoldArgs = boost::any_cast<std::string>(
                    args[ArgDefaultCacheVoldArgs]);
        }
    } catch (const boost::bad_any_cast &e) {
        Log::log(Log::Error, e.what());

        assert(false);
    }

    return modifyFstab(removeModemMounts,
                       additionalFstabs,
                       forceSystemRw,
                       forceCacheRw,
                       forceDataRw,
                       keepMountPoints,
                       systemMountPoint,
                       cacheMountPoint,
                       dataMountPoint,
                       defaultSystemMountArgs,
                       defaultSystemVoldArgs,
                       defaultCacheMountArgs,
                       defaultCacheVoldArgs);
}

bool QcomRamdiskPatcher::modifyFstab(bool removeModemMounts,
                                     const std::vector<std::string> &additionalFstabs,
                                     bool forceSystemRw,
                                     bool forceCacheRw,
                                     bool forceDataRw,
                                     bool keepMountPoints,
                                     const std::string &systemMountPoint,
                                     const std::string &cacheMountPoint,
                                     const std::string &dataMountPoint,
                                     const std::string &defaultSystemMountArgs,
                                     const std::string &defaultSystemVoldArgs,
                                     const std::string &defaultCacheMountArgs,
                                     const std::string &defaultCacheVoldArgs)
{
    std::vector<std::string> fstabs;
    for (auto const &file : m_impl->cpio->filenames()) {
        if (boost::starts_with(file, "fstab.")) {
            fstabs.push_back(file);
        }
    }

    fstabs.insert(fstabs.end(),
                  additionalFstabs.begin(), additionalFstabs.end());

    for (auto const &fstab : fstabs) {
        auto contents = m_impl->cpio->contents(fstab);
        if (contents.empty()) {
            m_impl->error = PatcherError::createCpioError(
                    MBP::ErrorCode::CpioFileNotExistError, fstab);
            return false;
        }

        std::string strContents(contents.begin(), contents.end());
        std::vector<std::string> lines;
        boost::split(lines, strContents, boost::is_any_of("\n"));

        static const std::string mountLine = "%1%%2% %3% %4% %5% %6%";

        // Some Android 4.2 ROMs mount the cache partition in the init
        // scripts, so the fstab has no cache line
        bool hasCacheLine = false;

        // Find the mount and vold arguments for the system and cache
        // partitions
        std::unordered_map<std::string, std::string> systemMountArgs;
        std::unordered_map<std::string, std::string> systemVoldArgs;
        std::unordered_map<std::string, std::string> cacheMountArgs;
        std::unordered_map<std::string, std::string> cacheVoldArgs;

        for (auto const &line : lines) {
            MBP_smatch what;

            if (MBP_regex_search(line, what,
                    MBP_regex(CoreRamdiskPatcher::FstabRegex))) {
                std::string comment = what[1];
                std::string blockDev = what[2];
                //std::string mountPoint = what[3];
                //std::string fsType = what[4];
                std::string mountArgs = what[5];
                std::string voldArgs = what[6];

                if (blockDev == SystemPartition) {
                    systemMountArgs[comment] = mountArgs;
                    systemVoldArgs[comment] = voldArgs;
                } else if (blockDev == CachePartition) {
                    cacheMountArgs[comment] = mountArgs;
                    cacheVoldArgs[comment] = voldArgs;
                }
            }
        }

        // In the unlikely case that we couldn't parse the mount arguments,
        // we'll just use the defaults
        if (systemMountArgs.empty()) {
            systemMountArgs[std::string()] = defaultSystemMountArgs;
        }
        if (systemVoldArgs.empty()) {
            systemVoldArgs[std::string()] = defaultSystemVoldArgs;
        }
        if (cacheMountArgs.empty()) {
            cacheMountArgs[std::string()] = defaultCacheMountArgs;
        }
        if (cacheVoldArgs.empty()) {
            cacheVoldArgs[std::string()] = defaultCacheVoldArgs;
        }

        for (auto it = lines.begin(); it != lines.end(); ++it) {
            auto &line = *it;

            std::string comment;
            std::string blockDev;
            std::string mountPoint;
            std::string fsType;
            std::string mountArgs;
            std::string voldArgs;

            MBP_smatch what;
            bool hasMatch = MBP_regex_search(
                    line, what, MBP_regex(CoreRamdiskPatcher::FstabRegex));

            if (hasMatch) {
                comment = what[1];
                blockDev = what[2];
                mountPoint = what[3];
                fsType = what[4];
                mountArgs = what[5];
                voldArgs = what[6];
            }

            if (hasMatch && blockDev == SystemPartition
                    && mountPoint == systemMountPoint) {
                if (!keepMountPoints) {
                    mountPoint = RawSystem;
                }

                if (m_impl->info->partConfig()->targetCache().find(RawSystem)
                        != std::string::npos) {
                    std::string cacheComment =
                            closestMatch(comment, keys(cacheMountArgs));
                    mountArgs = cacheMountArgs[cacheComment];
                    voldArgs = cacheVoldArgs[cacheComment];
                }

                if (forceSystemRw) {
                    mountArgs = makeWritable(mountArgs);
                }

                line = (boost::format(mountLine) % comment % blockDev
                        % mountPoint % fsType % mountArgs % voldArgs).str();
            } else if (hasMatch && blockDev == CachePartition
                    && mountPoint == cacheMountPoint) {
                if (!keepMountPoints) {
                    mountPoint = RawCache;
                }

                hasCacheLine = true;

                if (m_impl->info->partConfig()->targetSystem().find(RawCache)
                        != std::string::npos) {
                    std::string systemComment =
                            closestMatch(comment, keys(systemMountArgs));
                    mountArgs = systemMountArgs[systemComment];
                    voldArgs = systemVoldArgs[systemComment];
                }

                if (forceCacheRw) {
                    mountArgs = makeWritable(mountArgs);
                }

                line = (boost::format(mountLine) % comment % blockDev
                        % mountPoint % fsType % mountArgs % voldArgs).str();
            } else if (hasMatch && blockDev == DataPartition
                    && mountPoint == dataMountPoint) {
                if (!keepMountPoints) {
                    mountPoint = RawData;
                }

                if (m_impl->info->partConfig()->targetSystem().find(RawData)
                        != std::string::npos) {
                    std::string systemComment =
                            closestMatch(comment, keys(systemMountArgs));
                    mountArgs = systemMountArgs[systemComment];
                    voldArgs = systemVoldArgs[systemComment];
                }

                if (forceDataRw) {
                    mountArgs = makeWritable(mountArgs);
                }

                line = (boost::format(mountLine) % comment % blockDev
                        % mountPoint % fsType % mountArgs % voldArgs).str();
            } else if (hasMatch && removeModemMounts
                    && (blockDev.find("apnhlos") != std::string::npos
                    || blockDev.find("mdm") != std::string::npos
                    || blockDev.find("modem") != std::string::npos)) {
                it = lines.erase(it);
                it--;
            }
        }

        if (!hasCacheLine) {
            std::string cacheLine = "%1% /raw-cache ext4 %2% %3%";
            std::string mountArgs;
            std::string voldArgs;

            if (m_impl->info->partConfig()->targetSystem().find(RawCache)
                    != std::string::npos) {
                mountArgs = systemMountArgs[std::string()];
                voldArgs = systemVoldArgs[std::string()];
            } else {
                mountArgs = systemMountArgs[std::string()];
                voldArgs = systemVoldArgs[std::string()];
            }

            lines.push_back((boost::format(cacheLine) % CachePartition
                             % mountArgs % voldArgs).str());
        }

        strContents = boost::join(lines, "\n");
        contents.assign(strContents.begin(), strContents.end());
        m_impl->cpio->setContents(fstab, std::move(contents));
    }

    return true;
}

bool QcomRamdiskPatcher::addMissingCacheInFstab(const std::vector<std::string> &additionalFstabs)
{
    std::vector<std::string> fstabs;
    for (auto const &file : m_impl->cpio->filenames()) {
        if (boost::starts_with(file, "fstab.")) {
            fstabs.push_back(file);
        }
    }

    fstabs.insert(fstabs.end(),
                  additionalFstabs.begin(), additionalFstabs.end());

    for (auto const &fstab : fstabs) {
        auto contents = m_impl->cpio->contents(fstab);
        if (contents.empty()) {
            m_impl->error = PatcherError::createCpioError(
                    MBP::ErrorCode::CpioFileNotExistError, fstab);
            return false;
        }

        std::string strContents(contents.begin(), contents.end());
        std::vector<std::string> lines;
        boost::split(lines, strContents, boost::is_any_of("\n"));

        static const std::string mountLine = "%1%%2% %3% %4% %5% %6%";

        // Some Android 4.2 ROMs mount the cache partition in the init
        // scripts, so the fstab has no cache line
        bool hasCacheLine = false;

        for (auto it = lines.begin(); it != lines.end(); ++it) {
            auto &line = *it;

            MBP_smatch what;
            bool hasMatch = MBP_regex_search(
                    line, what, MBP_regex(CoreRamdiskPatcher::FstabRegex));

            if (hasMatch && what[3] == Cache) {
                hasCacheLine = true;
            }
        }

        if (!hasCacheLine) {
            std::string cacheLine = "%1% /cache ext4 %2% %3%";
            std::string mountArgs = "nosuid,nodev,barrier=1";
            std::string voldArgs = "wait,check";

            lines.push_back((boost::format(cacheLine) % CachePartition
                             % mountArgs % voldArgs).str());
        }

        strContents = boost::join(lines, "\n");
        contents.assign(strContents.begin(), strContents.end());
        m_impl->cpio->setContents(fstab, std::move(contents));
    }

    return true;
}

static std::string whitespace(const std::string &str) {
    auto nonSpace = std::find_if(str.begin(), str.end(),
                                 std::not1(std::ptr_fun<int, int>(isspace)));
    int count = std::distance(str.begin(), nonSpace);

    return str.substr(0, count);
}

bool QcomRamdiskPatcher::modifyInitTargetRc()
{
    return modifyInitTargetRc("init.target.rc");
}

bool QcomRamdiskPatcher::modifyInitTargetRc(const std::string &filename)
{
    auto contents = m_impl->cpio->contents(filename);
    if (contents.empty()) {
        m_impl->error = PatcherError::createCpioError(
                MBP::ErrorCode::CpioFileNotExistError, filename);
        return false;
    }

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (MBP_regex_search(*it,
                MBP_regex("^\\s+wait\\s+/dev/.*/cache.*$"))
                || MBP_regex_search(*it,
                MBP_regex("^\\s+check_fs\\s+/dev/.*/cache.*$"))
                || MBP_regex_search(*it,
                MBP_regex("^\\s+mount\\s+ext4\\s+/dev/.*/cache.*$"))) {
            // Remove lines that mount /cache
            it->insert(it->begin(), '#');
        } else if (MBP_regex_search(*it,
                MBP_regex("^\\s+mount_all\\s+(\\./)?fstab\\..*$"))) {
            // Add command for our mount script
            std::string spaces = whitespace(*it);
            it = lines.insert(++it, spaces + CoreRamdiskPatcher::ExecMount);
        } else if (MBP_regex_search(*it,
                MBP_regex("\\s/data/media(\\s|$)"))) {
            boost::replace_all(*it, "/data/media", "/raw-data/media");
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(filename, std::move(contents));

    return true;
}

bool QcomRamdiskPatcher::stripManualCacheMounts(const std::string &filename)
{
    auto contents = m_impl->cpio->contents(filename);
    if (contents.empty()) {
        m_impl->error = PatcherError::createCpioError(
                MBP::ErrorCode::CpioFileNotExistError, filename);
        return false;
    }

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (MBP_regex_search(*it,
                MBP_regex("^\\s+wait\\s+/dev/.*/cache.*$"))
                || MBP_regex_search(*it,
                MBP_regex("^\\s+check_fs\\s+/dev/.*/cache.*$"))
                || MBP_regex_search(*it,
                MBP_regex("^\\s+mount\\s+ext4\\s+/dev/.*/cache.*$"))) {
            // Remove lines that mount /cache
            it->insert(it->begin(), '#');
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(filename, std::move(contents));

    return true;
}

bool QcomRamdiskPatcher::useGeneratedFstab(const std::string &filename)
{
    auto contents = m_impl->cpio->contents(filename);
    if (contents.empty()) {
        m_impl->error = PatcherError::createCpioError(
                MBP::ErrorCode::CpioFileNotExistError, filename);
        return false;
    }

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    std::vector<std::string> fstabs;

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        MBP_smatch what;

        if (MBP_regex_search(*it, what,
                MBP_regex("^\\s+mount_all\\s+([^\\s]+)\\s*(#.*)?$"))) {
            // Use fstab generated by mbtool
            std::string spaces = whitespace(*it);

            std::string fstab = what[1];
            std::string completed = "/." + fstab + ".completed";
            std::string generated = "/." + fstab + ".gen";

            // Debugging this: "- exec '/system/bin/sh' failed: No such file or directory (2) -"
            // sure was fun... Turns out service names > 16 characters are rejected
            // See valid_name() in https://android.googlesource.com/platform/system/core/+/master/init/init_parser.c

            // Keep track of fstabs, so we can create services for them
            int index;
            auto it2 = std::find(fstabs.begin(), fstabs.end(), fstab);
            if (it2 == fstabs.end()) {
                index = fstabs.size();
                fstabs.push_back(fstab);
            } else {
                index = it2 - fstabs.begin();
            }

            std::string serviceName =
                    (boost::format("mbtool-mount-%03d") % index).str();

            // Start mounting service
            it = lines.insert(it, spaces + "start " + serviceName);
            // Wait for mount to complete (this sucks, but only custom ROMs
            // implement the 'exec' init script command)
            it = lines.insert(++it, spaces + "wait " + completed + " 15");
            // Mount generated fstab
            ++it;
            *it = spaces + "mount_all " + generated;
        }
    }

    for (unsigned int i = 0; i < fstabs.size(); ++i) {
        std::string serviceName =
                (boost::format("mbtool-mount-%03d") % i).str();

        lines.push_back((boost::format("service %s /mbtool mount_fstab %s")
                % serviceName % fstabs[i]).str());
        lines.push_back("    class core");
        lines.push_back("    critical");
        lines.push_back("    oneshot");
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(filename, std::move(contents));

    return true;
}

std::string QcomRamdiskPatcher::makeWritable(const std::string &mountArgs)
{
    static const std::string PostRo = "ro,";
    static const std::string PostRw = "rw,";
    static const std::string PreRo = ",ro";
    static const std::string PreRw = ",rw";

    if (mountArgs.find(PostRo) != std::string::npos) {
        return boost::replace_all_copy(mountArgs, PostRo, PostRw);
    } else if (mountArgs.find(PreRo) != std::string::npos) {
        return boost::replace_all_copy(mountArgs, PreRo, PreRw);
    } else {
        return PreRw + mountArgs;
    }
}
