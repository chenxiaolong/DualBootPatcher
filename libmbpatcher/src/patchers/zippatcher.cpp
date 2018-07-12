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

#include "mbpatcher/patchers/zippatcher.h"

#include <algorithm>
#include <unordered_set>

#include <cassert>
#include <cstring>

#include "mbcommon/string.h"
#include "mbcommon/version.h"
#include "mbdevice/json.h"
#include "mblog/logging.h"
#include "mbpio/delete.h"

#include "mbpatcher/autopatchers/magiskpatcher.h"
#include "mbpatcher/autopatchers/mountcmdpatcher.h"
#include "mbpatcher/autopatchers/standardpatcher.h"
#include "mbpatcher/patcherconfig.h"
#include "mbpatcher/private/fileutils.h"
#include "mbpatcher/private/miniziputils.h"

// minizip
#include "mz_zip.h"

#define LOG_TAG "mbpatcher/patchers/zippatcher"


namespace mb::patcher
{

const std::string ZipPatcher::Id("ZipPatcher");


ZipPatcher::ZipPatcher(PatcherConfig &pc)
    : m_pc(pc)
    , m_info(nullptr)
    , m_bytes(0)
    , m_max_bytes(0)
    , m_files(0)
    , m_max_files(0)
    , m_cancelled(false)
    , m_error()
    , m_progress_cb(nullptr)
    , m_files_cb(nullptr)
    , m_details_cb(nullptr)
    , m_z_input(nullptr)
    , m_z_output(nullptr)
{
}

ZipPatcher::~ZipPatcher() = default;

ErrorCode ZipPatcher::error() const
{
    return m_error;
}

std::string ZipPatcher::id() const
{
    return Id;
}

void ZipPatcher::set_file_info(const FileInfo * const info)
{
    m_info = info;
}

void ZipPatcher::cancel_patching()
{
    m_cancelled = true;
}

bool ZipPatcher::patch_file(const ProgressUpdatedCallback &progress_cb,
                            const FilesUpdatedCallback &files_cb,
                            const DetailsUpdatedCallback &details_cb)
{
    m_cancelled = false;

    assert(m_info != nullptr);

    m_progress_cb = &progress_cb;
    m_files_cb = &files_cb;
    m_details_cb = &details_cb;

    m_bytes = 0;
    m_max_bytes = 0;
    m_files = 0;
    m_max_files = 0;

    bool ret = patch_zip();

    m_progress_cb = nullptr;
    m_files_cb = nullptr;
    m_details_cb = nullptr;

    for (auto *p : m_auto_patchers) {
        m_pc.destroy_auto_patcher(p);
    }
    m_auto_patchers.clear();

    if (m_z_input != nullptr) {
        close_input_archive();
    }
    if (m_z_output != nullptr) {
        close_output_archive();
    }

    if (m_cancelled) {
        m_error = ErrorCode::PatchingCancelled;
        return false;
    }

    return ret;
}

struct CopySpec
{
    std::string source;
    std::string target;
};

bool ZipPatcher::patch_zip()
{
    std::unordered_set<std::string> exclude_from_pass1;

    for (auto const &id : {
        StandardPatcher::Id,
        MountCmdPatcher::Id,
        MagiskPatcher::Id,
    }) {
        auto *ap = m_pc.create_auto_patcher(id, *m_info);
        if (!ap) {
            m_error = ErrorCode::AutoPatcherCreateError;
            return false;
        }

        m_auto_patchers.push_back(ap);

        // AutoPatcher files should be excluded from the first pass
        for (auto const &file : ap->existing_files()) {
            exclude_from_pass1.insert(file);
        }
    }

    // Unlike the old patcher, we'll write directly to the new file
    if (!open_output_archive()) {
        return false;
    }

    void *handle = MinizipUtils::ctx_get_zip_handle(m_z_output);

    if (m_cancelled) return false;

    MinizipUtils::ArchiveStats stats;
    auto result = MinizipUtils::archive_stats(m_info->input_path(), stats, {});
    if (result != ErrorCode::NoError) {
        m_error = result;
        return false;
    }

    m_max_bytes = stats.total_size;

    if (m_cancelled) return false;

    std::string arch_dir(m_pc.data_directory());
    arch_dir += "/binaries/android/";
    arch_dir += m_info->device().architecture();

    std::vector<CopySpec> to_copy {
        {
            arch_dir + "/mbtool_recovery",
            "META-INF/com/google/android/update-binary"
        }, {
            arch_dir + "/mbtool_recovery.sig",
            "META-INF/com/google/android/update-binary.sig"
        }, {
            m_pc.data_directory() + "/scripts/bb-wrapper.sh",
            "multiboot/bb-wrapper.sh"
        }, {
            m_pc.data_directory() + "/scripts/bb-wrapper.sh.sig",
            "multiboot/bb-wrapper.sh.sig"
        }
    };

    std::vector<std::string> binaries{
        "file-contexts-tool",
        "file-contexts-tool.sig",
        "fsck-wrapper",
        "fsck-wrapper.sig",
        "mbtool",
        "mbtool.sig",
        "mount.exfat",
        "mount.exfat.sig",
    };

    for (auto const &binary : binaries) {
        to_copy.push_back({arch_dir + "/" + binary,
                          "multiboot/binaries/" + binary});
    }

    // +1 for info.prop
    // +1 for device.json
    m_max_files = stats.files + to_copy.size() + 2;
    update_files(m_files, m_max_files);

    if (!open_input_archive()) {
        return false;
    }

    // Create temporary dir for extracted files for autopatchers
    std::string temp_dir =
            FileUtils::create_temporary_dir(m_pc.temp_directory());

    if (!pass1(temp_dir, exclude_from_pass1)) {
        (void) io::delete_recursively(temp_dir);
        return false;
    }

    if (m_cancelled) return false;

    // On the second pass, run the autopatchers on the rest of the files

    if (!pass2(temp_dir, exclude_from_pass1)) {
        (void) io::delete_recursively(temp_dir);
        return false;
    }

    (void) io::delete_recursively(temp_dir);

    for (const CopySpec &spec : to_copy) {
        if (m_cancelled) return false;

        update_files(++m_files, m_max_files);
        update_details(spec.target);

        result = MinizipUtils::add_file_from_path(
                handle, spec.target, spec.source);
        if (result != ErrorCode::NoError) {
            m_error = result;
            return false;
        }
    }

    if (m_cancelled) return false;

    update_files(++m_files, m_max_files);
    update_details("multiboot/info.prop");

    result = MinizipUtils::add_file_from_data(
            handle, "multiboot/info.prop",
            create_info_prop(m_info->rom_id()));
    if (result != ErrorCode::NoError) {
        m_error = result;
        return false;
    }

    if (m_cancelled) return false;

    update_files(++m_files, m_max_files);
    update_details("multiboot/device.json");

    std::string json;
    if (!device::device_to_json(m_info->device(), json)) {
        m_error = ErrorCode::MemoryAllocationError;
        return false;
    }

    result = MinizipUtils::add_file_from_data(
            handle, "multiboot/device.json", json);
    if (result != ErrorCode::NoError) {
        m_error = result;
        return false;
    }

    if (m_cancelled) return false;

    return true;
}

/*!
 * \brief First pass of patching operation
 *
 * This performs the following operations:
 *
 * - Files needed by an AutoPatcher are extracted to the temporary directory.
 * - Otherwise, the file is copied directly to the output zip.
 */
bool ZipPatcher::pass1(const std::string &temporary_dir,
                       const std::unordered_set<std::string> &exclude)
{
    using namespace std::placeholders;

    void *h_in = MinizipUtils::ctx_get_zip_handle(m_z_input);
    void *h_out = MinizipUtils::ctx_get_zip_handle(m_z_output);

    int ret = mz_zip_goto_first_entry(h_in);
    if (ret != MZ_OK && ret != MZ_END_OF_LIST) {
        m_error = ErrorCode::ArchiveReadHeaderError;
        return false;
    }

    if (ret != MZ_END_OF_LIST) {
        do {
            if (m_cancelled) return false;

            mz_zip_file *file_info;

            ret = mz_zip_entry_get_info(h_in, &file_info);
            if (ret != MZ_OK) {
                m_error = ErrorCode::ArchiveReadHeaderError;
                return false;
            }

            std::string cur_file{file_info->filename, file_info->filename_size};

            update_files(++m_files, m_max_files);
            update_details(cur_file);

            // Skip files that should be patched and added in pass 2
            if (exclude.find(cur_file) != exclude.end()) {
                if (!MinizipUtils::extract_file(h_in, temporary_dir)) {
                    m_error = ErrorCode::ArchiveReadDataError;
                    return false;
                }
                continue;
            }

            // Rename the installer for mbtool
            if (cur_file == "META-INF/com/google/android/update-binary") {
                cur_file = "META-INF/com/google/android/update-binary.orig";
            }

            if (!MinizipUtils::copy_file_raw(h_in, h_out, cur_file,
                    std::bind(&ZipPatcher::la_progress_cb, this, _1))) {
                LOGW("minizip: Failed to copy raw data: %s", cur_file.c_str());
                m_error = ErrorCode::ArchiveWriteDataError;
                return false;
            }

            m_bytes += file_info->uncompressed_size;
        } while ((ret = mz_zip_goto_next_entry(h_in)) == MZ_OK);

        if (ret != MZ_END_OF_LIST) {
            m_error = ErrorCode::ArchiveReadHeaderError;
            return false;
        }
    }

    if (m_cancelled) return false;

    return true;
}

/*!
 * \brief Second pass of patching operation
 *
 * This performs the following operations:
 *
 * - Patch files in the temporary directory using the AutoPatchers and add the
 *   resulting files to the output zip
 */
bool ZipPatcher::pass2(const std::string &temporary_dir,
                       const std::unordered_set<std::string> &files)
{
    void *handle = MinizipUtils::ctx_get_zip_handle(m_z_output);

    for (auto *ap : m_auto_patchers) {
        if (m_cancelled) return false;
        if (!ap->patch_files(temporary_dir)) {
            m_error = ap->error();
            return false;
        }
    }

    // TODO Headers are being discarded

    for (auto const &file : files) {
        if (m_cancelled) return false;

        ErrorCode ret;

        if (file == "META-INF/com/google/android/update-binary") {
            ret = MinizipUtils::add_file_from_path(
                    handle,
                    "META-INF/com/google/android/update-binary.orig",
                    temporary_dir + "/" + file);
        } else {
            ret = MinizipUtils::add_file_from_path(
                    handle,
                    file,
                    temporary_dir + "/" + file);
        }

        if (ret == ErrorCode::FileOpenError) {
            LOGW("File does not exist in temporary directory: %s", file.c_str());
        } else if (ret != ErrorCode::NoError) {
            m_error = ret;
            return false;
        }
    }

    if (m_cancelled) return false;

    return true;
}

bool ZipPatcher::open_input_archive()
{
    assert(m_z_input == nullptr);

    m_z_input = MinizipUtils::open_zip_file(m_info->input_path(),
                                            ZipOpenMode::Read);

    if (!m_z_input) {
        LOGE("minizip: Failed to open for reading: %s",
             m_info->input_path().c_str());
        m_error = ErrorCode::ArchiveReadOpenError;
        return false;
    }

    return true;
}

void ZipPatcher::close_input_archive()
{
    assert(m_z_input != nullptr);

    int ret = MinizipUtils::close_zip_file(m_z_input);
    if (ret != MZ_OK) {
        LOGW("minizip: Failed to close archive (error code: %d)", ret);
    }

    m_z_input = nullptr;
}

bool ZipPatcher::open_output_archive()
{
    assert(m_z_output == nullptr);

    m_z_output = MinizipUtils::open_zip_file(m_info->output_path(),
                                             ZipOpenMode::Write);

    if (!m_z_output) {
        LOGE("minizip: Failed to open for writing: %s",
             m_info->output_path().c_str());
        m_error = ErrorCode::ArchiveWriteOpenError;
        return false;
    }

    return true;
}

void ZipPatcher::close_output_archive()
{
    assert(m_z_output != nullptr);

    int ret = MinizipUtils::close_zip_file(m_z_output);
    if (ret != MZ_OK) {
        LOGW("minizip: Failed to close archive (error code: %d)", ret);
    }

    m_z_output = nullptr;
}

void ZipPatcher::update_progress(uint64_t bytes, uint64_t max_bytes)
{
    if (m_progress_cb && *m_progress_cb) {
        (*m_progress_cb)(bytes, max_bytes);
    }
}

void ZipPatcher::update_files(uint64_t files, uint64_t max_files)
{
    if (m_files_cb && *m_files_cb) {
        (*m_files_cb)(files, max_files);
    }
}

void ZipPatcher::update_details(const std::string &msg)
{
    if (m_details_cb && *m_details_cb) {
        (*m_details_cb)(msg);
    }
}

void ZipPatcher::la_progress_cb(uint64_t bytes)
{
    update_progress(m_bytes + bytes, m_max_bytes);
}

std::string ZipPatcher::create_info_prop(const std::string &rom_id)
{
    std::string out;

    out +=
"# [Autogenerated by libmbpatcher]\n"
"#\n"
"# Blank lines are ignored. Lines beginning with '#' are comments and are also\n"
"# ignored. Before changing any fields, please read its description. Invalid\n"
"# values may lead to unexpected behavior when this zip file is installed.\n"
"\n"
"\n"
"# mbtool.installer.version\n"
"# ------------------------\n"
"# This field is the version of libmbpatcher and mbtool used to patch and install\n"
"# this file, respectively.\n"
"#\n";

    out += "mbtool.installer.version=";
    out += version();
    out += "\n";

    out +=
"\n"
"\n"
"# mbtool.installer.install-location\n"
"# ---------------------------------\n"
"# This field should be set to the desired installation location for the ROM.\n"
"# It is okay to change this value after the file has already been patched.\n"
"#\n"
"# Valid values: primary, dual, multi-slot-[1-3], data-slot-<id>, extsd-slot-<id>\n"
"#\n";

    out += "mbtool.installer.install-location=";
    out += rom_id;
    out += "\n";

    return out;
}

}
