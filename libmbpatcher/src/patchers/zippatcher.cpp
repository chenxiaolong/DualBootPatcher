/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbpatcher/patcherconfig.h"
#include "mbpatcher/private/fileutils.h"
#include "mbpatcher/private/miniziputils.h"
#include "mbpatcher/private/stringutils.h"

// minizip
#include "minizip/unzip.h"
#include "minizip/zip.h"

#define LOG_TAG "mbpatcher/patchers/zippatcher"


namespace mb
{
namespace patcher
{

/*! \cond INTERNAL */
class ZipPatcherPrivate
{
public:
    PatcherConfig *pc;
    const FileInfo *info;

    uint64_t bytes;
    uint64_t max_bytes;
    uint64_t files;
    uint64_t max_files;

    volatile bool cancelled;

    ErrorCode error;

    // Callbacks
    ZipPatcher::ProgressUpdatedCallback progress_cb;
    ZipPatcher::FilesUpdatedCallback files_cb;
    ZipPatcher::DetailsUpdatedCallback details_cb;
    void *userdata;

    // Patching
    MinizipUtils::UnzCtx *z_input = nullptr;
    MinizipUtils::ZipCtx *z_output = nullptr;
    std::vector<AutoPatcher *> auto_patchers;

    bool patch_zip();

    bool pass1(const std::string &temporary_dir,
               const std::unordered_set<std::string> &exclude);
    bool pass2(const std::string &temporary_dir,
               const std::unordered_set<std::string> &files);
    bool open_input_archive();
    void close_input_archive();
    bool open_output_archive();
    void close_output_archive();

    void update_progress(uint64_t bytes, uint64_t maxBytes);
    void update_files(uint64_t files, uint64_t maxFiles);
    void update_details(const std::string &msg);

    static void la_progress_cb(uint64_t bytes, void *userData);
};
/*! \endcond */


const std::string ZipPatcher::Id("ZipPatcher");


ZipPatcher::ZipPatcher(PatcherConfig * const pc)
    : _priv_ptr(new ZipPatcherPrivate())
{
    MB_PRIVATE(ZipPatcher);
    priv->pc = pc;
}

ZipPatcher::~ZipPatcher()
{
}

ErrorCode ZipPatcher::error() const
{
    MB_PRIVATE(const ZipPatcher);
    return priv->error;
}

std::string ZipPatcher::id() const
{
    return Id;
}

void ZipPatcher::set_file_info(const FileInfo * const info)
{
    MB_PRIVATE(ZipPatcher);
    priv->info = info;
}

void ZipPatcher::cancel_patching()
{
    MB_PRIVATE(ZipPatcher);
    priv->cancelled = true;
}

bool ZipPatcher::patch_file(ProgressUpdatedCallback progress_cb,
                            FilesUpdatedCallback files_cb,
                            DetailsUpdatedCallback details_cb,
                            void *userdata)
{
    MB_PRIVATE(ZipPatcher);
    priv->cancelled = false;

    assert(priv->info != nullptr);

    priv->progress_cb = progress_cb;
    priv->files_cb = files_cb;
    priv->details_cb = details_cb;
    priv->userdata = userdata;

    priv->bytes = 0;
    priv->max_bytes = 0;
    priv->files = 0;
    priv->max_files = 0;

    bool ret = priv->patch_zip();

    priv->progress_cb = nullptr;
    priv->files_cb = nullptr;
    priv->details_cb = nullptr;
    priv->userdata = nullptr;

    for (auto *p : priv->auto_patchers) {
        priv->pc->destroy_auto_patcher(p);
    }
    priv->auto_patchers.clear();

    if (priv->z_input != nullptr) {
        priv->close_input_archive();
    }
    if (priv->z_output != nullptr) {
        priv->close_output_archive();
    }

    if (priv->cancelled) {
        priv->error = ErrorCode::PatchingCancelled;
        return false;
    }

    return ret;
}

struct CopySpec
{
    std::string source;
    std::string target;
};

bool ZipPatcherPrivate::patch_zip()
{
    std::unordered_set<std::string> exclude_from_pass1;

    auto *standard_ap = pc->create_auto_patcher("StandardPatcher", info);
    if (!standard_ap) {
        error = ErrorCode::AutoPatcherCreateError;
        return false;
    }

    auto *mount_cmd_ap = pc->create_auto_patcher("MountCmdPatcher", info);
    if (!mount_cmd_ap) {
        error = ErrorCode::AutoPatcherCreateError;
        return false;
    }

    auto_patchers.push_back(standard_ap);
    auto_patchers.push_back(mount_cmd_ap);

    for (auto *ap : auto_patchers) {
        // AutoPatcher files should be excluded from the first pass
        for (auto const &file : ap->existing_files()) {
            exclude_from_pass1.insert(file);
        }
    }

    // Unlike the old patcher, we'll write directly to the new file
    if (!open_output_archive()) {
        return false;
    }

    zipFile zf = MinizipUtils::ctx_get_zip_file(z_output);

    if (cancelled) return false;

    MinizipUtils::ArchiveStats stats;
    auto result = MinizipUtils::archive_stats(info->input_path(), &stats,
                                              std::vector<std::string>());
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    max_bytes = stats.total_size;

    if (cancelled) return false;

    std::string arch_dir(pc->data_directory());
    arch_dir += "/binaries/android/";
    arch_dir += info->device().architecture();

    std::vector<CopySpec> to_copy {
        {
            arch_dir + "/mbtool_recovery",
            "META-INF/com/google/android/update-binary"
        }, {
            arch_dir + "/mbtool_recovery.sig",
            "META-INF/com/google/android/update-binary.sig"
        }, {
            pc->data_directory() + "/scripts/bb-wrapper.sh",
            "multiboot/bb-wrapper.sh"
        }, {
            pc->data_directory() + "/scripts/bb-wrapper.sh.sig",
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
    max_files = stats.files + to_copy.size() + 2;
    update_files(files, max_files);

    if (!open_input_archive()) {
        return false;
    }

    // Create temporary dir for extracted files for autopatchers
    std::string temp_dir =
            FileUtils::create_temporary_dir(pc->temp_directory());

    if (!pass1(temp_dir, exclude_from_pass1)) {
        io::deleteRecursively(temp_dir);
        return false;
    }

    if (cancelled) return false;

    // On the second pass, run the autopatchers on the rest of the files

    if (!pass2(temp_dir, exclude_from_pass1)) {
        io::deleteRecursively(temp_dir);
        return false;
    }

    io::deleteRecursively(temp_dir);

    for (const CopySpec &spec : to_copy) {
        if (cancelled) return false;

        update_files(++files, max_files);
        update_details(spec.target);

        result = MinizipUtils::add_file(zf, spec.target, spec.source);
        if (result != ErrorCode::NoError) {
            error = result;
            return false;
        }
    }

    if (cancelled) return false;

    update_files(++files, max_files);
    update_details("multiboot/info.prop");

    const std::string info_prop =
            ZipPatcher::create_info_prop(pc, info->rom_id(), false);
    result = MinizipUtils::add_file(
            zf, "multiboot/info.prop",
            std::vector<unsigned char>(info_prop.begin(), info_prop.end()));
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    update_files(++files, max_files);
    update_details("multiboot/device.json");

    std::string json;
    if (!device::device_to_json(info->device(), json)) {
        error = ErrorCode::MemoryAllocationError;
        return false;
    }

    result = MinizipUtils::add_file(
            zf, "multiboot/device.json",
            std::vector<unsigned char>(json.begin(), json.end()));
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

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
bool ZipPatcherPrivate::pass1(const std::string &temporary_dir,
                              const std::unordered_set<std::string> &exclude)
{
    unzFile uf = MinizipUtils::ctx_get_unz_file(z_input);
    zipFile zf = MinizipUtils::ctx_get_zip_file(z_output);

    int ret = unzGoToFirstFile(uf);
    if (ret != UNZ_OK) {
        error = ErrorCode::ArchiveReadHeaderError;
        return false;
    }

    do {
        if (cancelled) return false;

        unz_file_info64 fi;
        std::string cur_file;

        if (!MinizipUtils::get_info(uf, &fi, &cur_file)) {
            error = ErrorCode::ArchiveReadHeaderError;
            return false;
        }

        update_files(++files, max_files);
        update_details(cur_file);

        // Skip files that should be patched and added in pass 2
        if (exclude.find(cur_file) != exclude.end()) {
            if (!MinizipUtils::extract_file(uf, temporary_dir)) {
                error = ErrorCode::ArchiveReadDataError;
                return false;
            }
            continue;
        }

        // Rename the installer for mbtool
        if (cur_file == "META-INF/com/google/android/update-binary") {
            cur_file = "META-INF/com/google/android/update-binary.orig";
        }

        if (!MinizipUtils::copy_file_raw(uf, zf, cur_file, &la_progress_cb, this)) {
            LOGW("minizip: Failed to copy raw data: %s", cur_file.c_str());
            error = ErrorCode::ArchiveWriteDataError;
            return false;
        }

        bytes += fi.uncompressed_size;
    } while ((ret = unzGoToNextFile(uf)) == UNZ_OK);

    if (ret != UNZ_END_OF_LIST_OF_FILE) {
        error = ErrorCode::ArchiveReadHeaderError;
        return false;
    }

    if (cancelled) return false;

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
bool ZipPatcherPrivate::pass2(const std::string &temporary_dir,
                              const std::unordered_set<std::string> &files_)
{
    zipFile zf = MinizipUtils::ctx_get_zip_file(z_output);

    for (auto *ap : auto_patchers) {
        if (cancelled) return false;
        if (!ap->patch_files(temporary_dir)) {
            error = ap->error();
            return false;
        }
    }

    // TODO Headers are being discarded

    for (auto const &file : files_) {
        if (cancelled) return false;

        ErrorCode ret;

        if (file == "META-INF/com/google/android/update-binary") {
            ret = MinizipUtils::add_file(
                    zf,
                    "META-INF/com/google/android/update-binary.orig",
                      temporary_dir + "/" + file);
        } else {
            ret = MinizipUtils::add_file(
                    zf,
                    file,
                      temporary_dir + "/" + file);
        }

        if (ret == ErrorCode::FileOpenError) {
            LOGW("File does not exist in temporary directory: %s", file.c_str());
        } else if (ret != ErrorCode::NoError) {
            error = ret;
            return false;
        }
    }

    if (cancelled) return false;

    return true;
}

bool ZipPatcherPrivate::open_input_archive()
{
    assert(z_input == nullptr);

    z_input = MinizipUtils::open_input_file(info->input_path());

    if (!z_input) {
        LOGE("minizip: Failed to open for reading: %s",
             info->input_path().c_str());
        error = ErrorCode::ArchiveReadOpenError;
        return false;
    }

    return true;
}

void ZipPatcherPrivate::close_input_archive()
{
    assert(z_input != nullptr);

    int ret = MinizipUtils::close_input_file(z_input);
    if (ret != UNZ_OK) {
        LOGW("minizip: Failed to close archive (error code: %d)", ret);
    }

    z_input = nullptr;
}

bool ZipPatcherPrivate::open_output_archive()
{
    assert(z_output == nullptr);

    z_output = MinizipUtils::open_output_file(info->output_path());

    if (!z_output) {
        LOGE("minizip: Failed to open for writing: %s",
             info->output_path().c_str());
        error = ErrorCode::ArchiveWriteOpenError;
        return false;
    }

    return true;
}

void ZipPatcherPrivate::close_output_archive()
{
    assert(z_output != nullptr);

    int ret = MinizipUtils::close_output_file(z_output);
    if (ret != ZIP_OK) {
        LOGW("minizip: Failed to close archive (error code: %d)", ret);
    }

    z_output = nullptr;
}

void ZipPatcherPrivate::update_progress(uint64_t bytes_, uint64_t max_bytes_)
{
    if (progress_cb) {
        progress_cb(bytes_, max_bytes_, userdata);
    }
}

void ZipPatcherPrivate::update_files(uint64_t files_, uint64_t max_files_)
{
    if (files_cb) {
        files_cb(files_, max_files_, userdata);
    }
}

void ZipPatcherPrivate::update_details(const std::string &msg)
{
    if (details_cb) {
        details_cb(msg, userdata);
    }
}

void ZipPatcherPrivate::la_progress_cb(uint64_t bytes, void *userdata)
{
    auto *priv = static_cast<ZipPatcherPrivate *>(userdata);
    priv->update_progress(priv->bytes + bytes, priv->max_bytes);
}

std::string ZipPatcher::create_info_prop(const PatcherConfig * const pc,
                                         const std::string &rom_id,
                                         bool always_patch_ramdisk)
{
    (void) pc;

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

    out +=
"\n"
"\n"
"# mbtool.installer.always-patch-ramdisk\n"
"# -------------------------------------\n"
"# By default, the ramdisk is only patched if the boot partition is modified\n"
"# during the installation process. If this property is enabled, it will always\n"
"# be patched, regardless if the boot partition is modified.\n"
"#\n";

    out += "mbtool.installer.always-patch-ramdisk=";
    out += always_patch_ramdisk ? "true" : "false";
    out += "\n";

    return out;
}

}
}
