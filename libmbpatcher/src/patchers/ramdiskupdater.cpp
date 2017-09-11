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


namespace mb
{
namespace patcher
{

/*! \cond INTERNAL */
class RamdiskUpdaterPrivate
{
public:
    PatcherConfig *pc;
    const FileInfo *info;

    volatile bool cancelled;

    ErrorCode error;

    MinizipUtils::ZipCtx *z_output = nullptr;

    bool create_zip();

    bool open_output_archive();
    void close_output_archive();
};
/*! \endcond */


const std::string RamdiskUpdater::Id("RamdiskUpdater");


RamdiskUpdater::RamdiskUpdater(PatcherConfig * const pc)
    : _priv_ptr(new RamdiskUpdaterPrivate())
{
    MB_PRIVATE(RamdiskUpdater);
    priv->pc = pc;
}

RamdiskUpdater::~RamdiskUpdater()
{
}

ErrorCode RamdiskUpdater::error() const
{
    MB_PRIVATE(const RamdiskUpdater);
    return priv->error;
}

std::string RamdiskUpdater::id() const
{
    return Id;
}

void RamdiskUpdater::set_file_info(const FileInfo * const info)
{
    MB_PRIVATE(RamdiskUpdater);
    priv->info = info;
}

void RamdiskUpdater::cancel_patching()
{
    MB_PRIVATE(RamdiskUpdater);
    priv->cancelled = true;
}

bool RamdiskUpdater::patch_file(ProgressUpdatedCallback progress_cb,
                                FilesUpdatedCallback files_cb,
                                DetailsUpdatedCallback details_cb,
                                void *userdata)
{
    (void) progress_cb;
    (void) files_cb;
    (void) details_cb;
    (void) userdata;

    MB_PRIVATE(RamdiskUpdater);

    priv->cancelled = false;

    assert(priv->info != nullptr);

    bool ret = priv->create_zip();

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

bool RamdiskUpdaterPrivate::create_zip()
{
    ErrorCode result;

    // Unlike the old patcher, we'll write directly to the new file
    if (!open_output_archive()) {
        return false;
    }

    zipFile zf = MinizipUtils::ctx_get_zip_file(z_output);

    if (cancelled) return false;

    std::string arch_dir(pc->data_directory());
    arch_dir += "/binaries/android/";
    arch_dir += info->device().architecture();

    std::vector<CopySpec> toCopy{
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
        toCopy.push_back({arch_dir + "/" + binary,
                          "multiboot/binaries/" + binary});
    }

    for (const CopySpec &spec : toCopy) {
        if (cancelled) return false;

        result = MinizipUtils::add_file(zf, spec.target, spec.source);
        if (result != ErrorCode::NoError) {
            error = result;
            return false;
        }
    }

    if (cancelled) return false;

    const std::string info_prop =
            ZipPatcher::create_info_prop(pc, info->rom_id(), true);
    result = MinizipUtils::add_file(
            zf, "multiboot/info.prop",
            std::vector<unsigned char>(info_prop.begin(), info_prop.end()));
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

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

    // Create dummy "installer"
    std::string installer("#!/sbin/sh");

    result = MinizipUtils::add_file(
            zf, "META-INF/com/google/android/update-binary.orig",
            std::vector<unsigned char>(installer.begin(), installer.end()));

    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    return true;
}

bool RamdiskUpdaterPrivate::open_output_archive()
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

void RamdiskUpdaterPrivate::close_output_archive()
{
    assert(z_output != nullptr);

    int ret = MinizipUtils::close_output_file(z_output);
    if (ret != ZIP_OK) {
        LOGW("minizip: Failed to close archive (error code: %d)", ret);
    }

    z_output = nullptr;
}

}
}
