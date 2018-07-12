/*
 * Copyright (C) 2015-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <archive.h>
#include <archive_entry.h>

#include "mbpatcher/patcherconfig.h"
#include "mbpatcher/patcherinterface.h"

#ifdef __ANDROID__
#  include "mbcommon/file/fd.h"
#else
#  include "mbcommon/file/standard.h"
#endif


namespace mb::patcher
{

struct ZipCtx;

class OdinPatcher : public Patcher
{
public:
    OdinPatcher(PatcherConfig &pc);
    virtual ~OdinPatcher();

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

    uint64_t m_old_bytes;
    uint64_t m_bytes;
    uint64_t m_max_bytes;

    volatile bool m_cancelled;

    ErrorCode m_error;

    unsigned char m_la_buf[10240];
#ifdef __ANDROID__
    FdFile m_la_file;
    int m_fd;
#else
    StandardFile m_la_file;
#endif

    std::unordered_set<std::string> m_added_files;

    // Callbacks
    const ProgressUpdatedCallback *m_progress_cb;
    const DetailsUpdatedCallback *m_details_cb;

    // Patching
    archive *m_a_input;
    ZipCtx *m_z_output;

    bool patch_tar();

    bool process_file(archive *a, archive_entry *entry, bool sparse);
    bool process_contents(archive *a, unsigned int depth,
                          const char *raw_entry_path);
    bool open_input_archive();
    bool close_input_archive();
    bool open_output_archive();
    bool close_output_archive();

    void update_progress(uint64_t bytes, uint64_t max_bytes);
    void update_details(const std::string &msg);

    static la_ssize_t la_nested_read_cb(archive *a, void *userdata,
                                        const void **buffer);

    static la_ssize_t la_read_cb(archive *a, void *userdata,
                                 const void **buffer);
    static la_int64_t la_skip_cb(archive *a, void *userdata,
                                 la_int64_t request);
    static int la_open_cb(archive *a, void *userdata);
    static int la_close_cb(archive *a, void *userdata);
};

}
