/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

class OdinPatcherPrivate;
class OdinPatcher : public Patcher
{
    MB_DECLARE_PRIVATE(OdinPatcher)

public:
    explicit OdinPatcher(PatcherConfig * const pc);
    ~OdinPatcher();

    static const std::string Id;

    virtual ErrorCode error() const override;

    // Patcher info
    virtual std::string id() const override;

    // Patching
    virtual void set_file_info(const FileInfo * const info) override;

    virtual bool patch_file(ProgressUpdatedCallback progress_cb,
                            FilesUpdatedCallback files_cb,
                            DetailsUpdatedCallback details_cb,
                            void *userdata) override;

    virtual void cancel_patching() override;

private:
    std::unique_ptr<OdinPatcherPrivate> _priv_ptr;
};

}
}
