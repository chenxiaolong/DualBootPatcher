/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "mbpatcher/patcherconfig.h"
#include "mbpatcher/patcherinterface.h"


namespace mb::patcher
{

class MountCmdPatcher : public AutoPatcher
{
public:
    MountCmdPatcher(const PatcherConfig &pc, const FileInfo &info);
    virtual ~MountCmdPatcher();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(MountCmdPatcher)
    MB_DISABLE_MOVE_CONSTRUCT_AND_ASSIGN(MountCmdPatcher)

    static const std::string Id;

    ErrorCode error() const override;

    std::string id() const override;

    std::vector<std::string> new_files() const override;
    std::vector<std::string> existing_files() const override;

    bool patch_files(const std::string &directory) override;
};

}
