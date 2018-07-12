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

#include "mbpatcher/patchers/ramdiskupdater.h"

#include <cassert>
#include <cstring>

#include "mbdevice/device.h"
#include "mbdevice/json.h"

#include "mblog/logging.h"

#include "mbpatcher/patcherconfig.h"
#include "mbpatcher/patchers/zippatcher.h"
#include "mbpatcher/private/miniziputils.h"

#define LOG_TAG "mbpatcher/patchers/ramdiskupdater"


namespace mb::patcher
{

const std::string RamdiskUpdater::Id("RamdiskUpdater");


RamdiskUpdater::RamdiskUpdater(PatcherConfig &pc)
    : m_pc(pc)
    , m_info(nullptr)
    , m_cancelled(false)
    , m_error()
    , m_z_output(nullptr)
{
}

RamdiskUpdater::~RamdiskUpdater() = default;

ErrorCode RamdiskUpdater::error() const
{
    return m_error;
}

std::string RamdiskUpdater::id() const
{
    return Id;
}

void RamdiskUpdater::set_file_info(const FileInfo * const info)
{
    m_info = info;
}

void RamdiskUpdater::cancel_patching()
{
    m_cancelled = true;
}

bool RamdiskUpdater::patch_file(const ProgressUpdatedCallback &progress_cb,
                                const FilesUpdatedCallback &files_cb,
                                const DetailsUpdatedCallback &details_cb)
{
    (void) progress_cb;
    (void) files_cb;
    (void) details_cb;

    m_cancelled = false;

    assert(m_info != nullptr);

    bool ret = create_zip();

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

bool RamdiskUpdater::create_zip()
{
    ErrorCode result;

    // Unlike the old patcher, we'll write directly to the new file
    if (!open_output_archive()) {
        return false;
    }

    void *handle = MinizipUtils::ctx_get_zip_handle(m_z_output);

    if (m_cancelled) return false;

    std::string arch_dir(m_pc.data_directory());
    arch_dir += "/binaries/android/";
    arch_dir += m_info->device().architecture();

    std::vector<CopySpec> toCopy{
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
        toCopy.push_back({arch_dir + "/" + binary,
                          "multiboot/binaries/" + binary});
    }

    for (const CopySpec &spec : toCopy) {
        if (m_cancelled) return false;

        result = MinizipUtils::add_file_from_path(
                handle, spec.target, spec.source);
        if (result != ErrorCode::NoError) {
            m_error = result;
            return false;
        }
    }

    if (m_cancelled) return false;

    result = MinizipUtils::add_file_from_data(
            handle, "multiboot/info.prop",
            ZipPatcher::create_info_prop(m_info->rom_id()));
    if (result != ErrorCode::NoError) {
        m_error = result;
        return false;
    }

    if (m_cancelled) return false;

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

    // Create dummy "installer"
    result = MinizipUtils::add_file_from_data(
            handle, "META-INF/com/google/android/update-binary.orig",
            "#!/sbin/sh");
    if (result != ErrorCode::NoError) {
        m_error = result;
        return false;
    }

    if (m_cancelled) return false;

    return true;
}

bool RamdiskUpdater::open_output_archive()
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

void RamdiskUpdater::close_output_archive()
{
    assert(m_z_output != nullptr);

    int ret = MinizipUtils::close_zip_file(m_z_output);
    if (ret != MZ_OK) {
        LOGW("minizip: Failed to close archive (error code: %d)", ret);
    }

    m_z_output = nullptr;
}

}
