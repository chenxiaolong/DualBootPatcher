/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#include <memory>

#include "libmbp_global.h"

#include "cpiofile.h"
#include "device.h"
#ifndef LIBMBP_MINI
#include "fileinfo.h"
#endif
#include "patchererror.h"
#ifndef LIBMBP_MINI
#include "patchinfo.h"
#endif


namespace mbp
{

#ifndef LIBMBP_MINI
class Patcher;
class AutoPatcher;
class RamdiskPatcher;
#endif

class MBP_EXPORT PatcherConfig
{
public:
    PatcherConfig();
    ~PatcherConfig();

    PatcherError error() const;

    std::string dataDirectory() const;
    std::string tempDirectory() const;

    void setDataDirectory(std::string path);
    void setTempDirectory(std::string path);

    std::string version() const;
    std::vector<Device *> devices() const;
#ifndef LIBMBP_MINI
    std::vector<PatchInfo *> patchInfos() const;
    std::vector<PatchInfo *> patchInfos(const Device * const device) const;

    PatchInfo * findMatchingPatchInfo(Device *device,
                                      const std::string &filename) const;

    std::vector<std::string> patchers() const;
    std::vector<std::string> autoPatchers() const;
    std::vector<std::string> ramdiskPatchers() const;

    Patcher * createPatcher(const std::string &id);
    AutoPatcher * createAutoPatcher(const std::string &id,
                                    const FileInfo * const info,
                                    const PatchInfo::AutoPatcherArgs &args);
    RamdiskPatcher * createRamdiskPatcher(const std::string &id,
                                          const FileInfo * const info,
                                          CpioFile * const cpio);

    void destroyPatcher(Patcher *patcher);
    void destroyAutoPatcher(AutoPatcher *patcher);
    void destroyRamdiskPatcher(RamdiskPatcher *patcher);

    bool loadPatchInfos();
#endif

    PatcherConfig(const PatcherConfig &) = delete;
    PatcherConfig(PatcherConfig &&) = default;
    PatcherConfig & operator=(const PatcherConfig &) & = delete;
    PatcherConfig & operator=(PatcherConfig &&) & = default;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}
