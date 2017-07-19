/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <memory>

#include "mbpatcher/patcherconfig.h"
#include "mbpatcher/patcherinterface.h"


namespace mb
{
namespace patcher
{

class StandardPatcherPrivate;
class StandardPatcher : public AutoPatcher
{
    MB_DECLARE_PRIVATE(StandardPatcher)

public:
    explicit StandardPatcher(const PatcherConfig * const pc,
                             const FileInfo * const info);
    ~StandardPatcher();

    static const std::string Id;

    static const std::string UpdaterScript;

    static const std::string SystemTransferList;

    virtual ErrorCode error() const override;

    virtual std::string id() const override;

    virtual std::vector<std::string> new_files() const override;
    virtual std::vector<std::string> existing_files() const override;

    virtual bool patch_files(const std::string &directory) override;

    bool patch_updater(const std::string &directory);
    bool patch_transfer_list(const std::string &directory);

private:
    std::unique_ptr<StandardPatcherPrivate> _priv_ptr;
};

}
}
