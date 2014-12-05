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

#ifndef QCOMRAMDISKPATCHER_H
#define QCOMRAMDISKPATCHER_H

#include <memory>
#include <unordered_map>

#include <boost/any.hpp>

#include "cpiofile.h"
#include "patcherconfig.h"
#include "patcherinterface.h"


class QcomRamdiskPatcher : public RamdiskPatcher
{
public:
    typedef std::unordered_map<std::string, boost::any> FstabArgs;

    static const std::string SystemPartition;
    static const std::string CachePartition;
    static const std::string DataPartition;

    static const std::string ArgAdditionalFstabs;
    static const std::string ArgForceSystemRw;
    static const std::string ArgForceCacheRw;
    static const std::string ArgForceDataRw;
    static const std::string ArgKeepMountPoints;
    static const std::string ArgSystemMountPoint;
    static const std::string ArgCacheMountPoint;
    static const std::string ArgDataMountPoint;
    static const std::string ArgDefaultSystemMountArgs;
    static const std::string ArgDefaultSystemVoldArgs;
    static const std::string ArgDefaultCacheMountArgs;
    static const std::string ArgDefaultCacheVoldArgs;

    explicit QcomRamdiskPatcher(const PatcherConfig * const pc,
                                const FileInfo * const info,
                                CpioFile * const cpio);
    ~QcomRamdiskPatcher();

    virtual PatcherError error() const override;

    virtual std::string id() const override;

    virtual bool patchRamdisk() override;

    bool modifyInitRc();
    bool modifyInitQcomRc(const std::vector<std::string> &additionalFiles =
                          std::vector<std::string>());

    bool modifyFstab(// Optional
                     bool removeModemMounts = false);
    bool modifyFstab(FstabArgs args,
                     // Optional
                     bool removeModemMounts = false);
    bool modifyFstab(bool removeModemMounts,
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
                     const std::string &defaultCacheVoldArgs);

    bool addMissingCacheInFstab(const std::vector<std::string> &additionalFstabs);

    bool modifyInitTargetRc();
    bool modifyInitTargetRc(const std::string &filename);

    bool stripManualCacheMounts(const std::string &filename);
    bool useGeneratedFstab(const std::string &filename);

private:
    static std::string makeWritable(const std::string &mountArgs);

    class Impl;
    std::unique_ptr<Impl> m_impl;
};

#endif // QCOMRAMDISKPATCHER_H
