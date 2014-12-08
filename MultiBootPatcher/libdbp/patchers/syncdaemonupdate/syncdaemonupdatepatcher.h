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

#pragma once

#include <memory>

#include "patcherconfig.h"
#include "patcherinterface.h"


class SyncdaemonUpdatePatcher : public Patcher
{
public:
    explicit SyncdaemonUpdatePatcher(const PatcherConfig * const pc);
    ~SyncdaemonUpdatePatcher();

    static const std::string Id;
    static const std::string Name;

    static std::vector<PartitionConfig *> partConfigs();

    // Error reporting
    virtual PatcherError error() const;

    // Patcher info
    virtual std::string id() const;
    virtual std::string name() const;
    virtual bool usesPatchInfo() const;
    virtual std::vector<std::string> supportedPartConfigIds() const;

    // Patching
    virtual void setFileInfo(const FileInfo * const info);

    virtual std::string newFilePath();

    virtual bool patchFile(MaxProgressUpdatedCallback maxProgressCb,
                           ProgressUpdatedCallback progressCb,
                           DetailsUpdatedCallback detailsCb,
                           void *userData);

    virtual void cancelPatching();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
