/*
 * Copyright (C) 2014-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

struct ZipCtx;

class RamdiskUpdater : public Patcher
{
public:
    RamdiskUpdater(PatcherConfig &pc);
    virtual ~RamdiskUpdater();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(RamdiskUpdater)
    MB_DISABLE_MOVE_CONSTRUCT_AND_ASSIGN(RamdiskUpdater)

    static const std::string Id;

    ErrorCode error() const override;

    // Patcher info
    std::string id() const override;

    // Patching
    void set_file_info(const FileInfo * const info) override;

    bool patch_file(const ProgressUpdatedCallback &progress_cb,
                    const FilesUpdatedCallback &files_cb,
                    const DetailsUpdatedCallback &details_cb) override;

    void cancel_patching() override;

private:
    PatcherConfig &m_pc;
    const FileInfo *m_info;

    volatile bool m_cancelled;

    ErrorCode m_error;

    ZipCtx *m_z_output;

    bool create_zip();

    bool open_output_archive();
    void close_output_archive();
};

}
