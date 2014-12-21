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

#include <archive.h>

#include "patcherconfig.h"
#include "patcherinterface.h"


class MultiBootPatcher : public Patcher
{
public:
    explicit MultiBootPatcher(PatcherConfig * const pc);
    ~MultiBootPatcher();

    static const std::string Id;
    static const std::string Name;

    static std::vector<PartitionConfig *> partConfigs();

    virtual PatcherError error() const override;

    // Patcher info
    virtual std::string id() const override;
    virtual std::string name() const override;
    virtual bool usesPatchInfo() const override;
    virtual std::vector<std::string> supportedPartConfigIds() const override;

    // Patching
    virtual void setFileInfo(const FileInfo * const info) override;

    virtual std::string newFilePath() override;

    virtual bool patchFile(MaxProgressUpdatedCallback maxProgressCb,
                           ProgressUpdatedCallback progressCb,
                           DetailsUpdatedCallback detailsCb,
                           void *userData) override;

    virtual void cancelPatching() override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
