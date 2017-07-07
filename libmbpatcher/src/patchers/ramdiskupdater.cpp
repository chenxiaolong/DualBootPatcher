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

    MinizipUtils::ZipCtx *zOutput = nullptr;

    bool createZip();

    bool openOutputArchive();
    void closeOutputArchive();
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

void RamdiskUpdater::setFileInfo(const FileInfo * const info)
{
    MB_PRIVATE(RamdiskUpdater);
    priv->info = info;
}

void RamdiskUpdater::cancelPatching()
{
    MB_PRIVATE(RamdiskUpdater);
    priv->cancelled = true;
}

bool RamdiskUpdater::patchFile(ProgressUpdatedCallback progressCb,
                               FilesUpdatedCallback filesCb,
                               DetailsUpdatedCallback detailsCb,
                               void *userData)
{
    (void) progressCb;
    (void) filesCb;
    (void) detailsCb;
    (void) userData;

    MB_PRIVATE(RamdiskUpdater);

    priv->cancelled = false;

    assert(priv->info != nullptr);

    bool ret = priv->createZip();

    if (priv->zOutput != nullptr) {
        priv->closeOutputArchive();
    }

    if (priv->cancelled) {
        priv->error = ErrorCode::PatchingCancelled;
        return false;
    }

    return ret;
}

struct CopySpec {
    std::string source;
    std::string target;
};

bool RamdiskUpdaterPrivate::createZip()
{
    ErrorCode result;

    // Unlike the old patcher, we'll write directly to the new file
    if (!openOutputArchive()) {
        return false;
    }

    zipFile zf = MinizipUtils::ctxGetZipFile(zOutput);

    if (cancelled) return false;

    std::string archDir(pc->dataDirectory());
    archDir += "/binaries/android/";
    archDir += mb_device_architecture(info->device());

    std::vector<CopySpec> toCopy{
        {
            archDir + "/mbtool_recovery",
            "META-INF/com/google/android/update-binary"
        }, {
            archDir + "/mbtool_recovery.sig",
            "META-INF/com/google/android/update-binary.sig"
        }, {
            pc->dataDirectory() + "/scripts/bb-wrapper.sh",
            "multiboot/bb-wrapper.sh"
        }, {
            pc->dataDirectory() + "/scripts/bb-wrapper.sh.sig",
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
        toCopy.push_back({archDir + "/" + binary,
                          "multiboot/binaries/" + binary});
    }

    for (const CopySpec &spec : toCopy) {
        if (cancelled) return false;

        result = MinizipUtils::addFile(zf, spec.target, spec.source);
        if (result != ErrorCode::NoError) {
            error = result;
            return false;
        }
    }

    if (cancelled) return false;

    const std::string infoProp =
            ZipPatcher::createInfoProp(pc, info->romId(), true);
    result = MinizipUtils::addFile(
            zf, "multiboot/info.prop",
            std::vector<unsigned char>(infoProp.begin(), infoProp.end()));
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    char *json = mb_device_to_json(info->device());
    if (!json) {
        error = ErrorCode::MemoryAllocationError;
        return false;
    }

    result = MinizipUtils::addFile(
            zf, "multiboot/device.json",
            std::vector<unsigned char>(json, json + strlen(json)));
    free(json);

    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    // Create dummy "installer"
    std::string installer("#!/sbin/sh");

    result = MinizipUtils::addFile(
            zf, "META-INF/com/google/android/update-binary.orig",
            std::vector<unsigned char>(installer.begin(), installer.end()));

    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    return true;
}

bool RamdiskUpdaterPrivate::openOutputArchive()
{
    assert(zOutput == nullptr);

    zOutput = MinizipUtils::openOutputFile(info->outputPath());

    if (!zOutput) {
        LOGE("minizip: Failed to open for writing: %s",
             info->outputPath().c_str());
        error = ErrorCode::ArchiveWriteOpenError;
        return false;
    }

    return true;
}

void RamdiskUpdaterPrivate::closeOutputArchive()
{
    assert(zOutput != nullptr);

    int ret = MinizipUtils::closeOutputFile(zOutput);
    if (ret != ZIP_OK) {
        LOGW("minizip: Failed to close archive (error code: %d)", ret);
    }

    zOutput = nullptr;
}

}
}
