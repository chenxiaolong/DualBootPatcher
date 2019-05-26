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

#include <unordered_set>

#include "mbpatcher/patcherconfig.h"
#include "mbpatcher/patcherinterface.h"


namespace mb::patcher
{

struct UnzCtx;
struct ZipCtx;

class ZipPatcher : public Patcher
{
public:
    ZipPatcher(PatcherConfig &pc);
    virtual ~ZipPatcher();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(ZipPatcher)
    MB_DISABLE_MOVE_CONSTRUCT_AND_ASSIGN(ZipPatcher)

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

    static std::string create_info_prop(const std::string &rom_id);

private:
    PatcherConfig &m_pc;
    const FileInfo *m_info;

    uint64_t m_bytes;
    uint64_t m_max_bytes;
    uint64_t m_files;
    uint64_t m_max_files;

    volatile bool m_cancelled;

    ErrorCode m_error;

    // Callbacks
    const ProgressUpdatedCallback *m_progress_cb;
    const FilesUpdatedCallback *m_files_cb;
    const DetailsUpdatedCallback *m_details_cb;

    // Patching
    ZipCtx *m_z_input = nullptr;
    ZipCtx *m_z_output = nullptr;
    std::vector<AutoPatcher *> m_auto_patchers;

    bool patch_zip();

    bool pass1(const std::string &temporary_dir,
               const std::unordered_set<std::string> &exclude);
    bool pass2(const std::string &temporary_dir,
               const std::unordered_set<std::string> &files);
    bool open_input_archive();
    void close_input_archive();
    bool open_output_archive();
    void close_output_archive();

    void update_progress(uint64_t bytes, uint64_t max_bytes);
    void update_files(uint64_t files, uint64_t max_files);
    void update_details(const std::string &msg);

    void la_progress_cb(uint64_t bytes);
};

}
