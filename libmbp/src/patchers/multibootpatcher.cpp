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

#include "mbp/patchers/multibootpatcher.h"

#include <algorithm>
#include <unordered_set>

#include <cassert>
#include <cstring>

#include "mbcommon/string.h"
#include "mbcommon/version.h"
#include "mbdevice/json.h"
#include "mblog/logging.h"
#include "mbpio/delete.h"

#include "mbp/patcherconfig.h"
#include "mbp/private/fileutils.h"
#include "mbp/private/miniziputils.h"
#include "mbp/private/stringutils.h"

// minizip
#include "minizip/unzip.h"
#include "minizip/zip.h"


namespace mbp
{

/*! \cond INTERNAL */
class MultiBootPatcher::Impl
{
public:
    PatcherConfig *pc;
    const FileInfo *info;

    uint64_t bytes;
    uint64_t maxBytes;
    uint64_t files;
    uint64_t maxFiles;

    volatile bool cancelled;

    ErrorCode error;

    // Callbacks
    ProgressUpdatedCallback progressCb;
    FilesUpdatedCallback filesCb;
    DetailsUpdatedCallback detailsCb;
    void *userData;

    // Patching
    MinizipUtils::UnzCtx *zInput = nullptr;
    MinizipUtils::ZipCtx *zOutput = nullptr;
    std::vector<AutoPatcher *> autoPatchers;

    bool patchZip();

    bool pass1(const std::string &temporaryDir,
               const std::unordered_set<std::string> &exclude);
    bool pass2(const std::string &temporaryDir,
               const std::unordered_set<std::string> &files);
    bool openInputArchive();
    void closeInputArchive();
    bool openOutputArchive();
    void closeOutputArchive();

    void updateProgress(uint64_t bytes, uint64_t maxBytes);
    void updateFiles(uint64_t files, uint64_t maxFiles);
    void updateDetails(const std::string &msg);

    static void laProgressCb(uint64_t bytes, void *userData);
};
/*! \endcond */


const std::string MultiBootPatcher::Id("MultiBootPatcher");


MultiBootPatcher::MultiBootPatcher(PatcherConfig * const pc)
    : m_impl(new Impl())
{
    m_impl->pc = pc;
}

MultiBootPatcher::~MultiBootPatcher()
{
}

ErrorCode MultiBootPatcher::error() const
{
    return m_impl->error;
}

std::string MultiBootPatcher::id() const
{
    return Id;
}

void MultiBootPatcher::setFileInfo(const FileInfo * const info)
{
    m_impl->info = info;
}

void MultiBootPatcher::cancelPatching()
{
    m_impl->cancelled = true;
}

bool MultiBootPatcher::patchFile(ProgressUpdatedCallback progressCb,
                                 FilesUpdatedCallback filesCb,
                                 DetailsUpdatedCallback detailsCb,
                                 void *userData)
{
    m_impl->cancelled = false;

    assert(m_impl->info != nullptr);

    m_impl->progressCb = progressCb;
    m_impl->filesCb = filesCb;
    m_impl->detailsCb = detailsCb;
    m_impl->userData = userData;

    m_impl->bytes = 0;
    m_impl->maxBytes = 0;
    m_impl->files = 0;
    m_impl->maxFiles = 0;

    bool ret = m_impl->patchZip();

    m_impl->progressCb = nullptr;
    m_impl->filesCb = nullptr;
    m_impl->detailsCb = nullptr;
    m_impl->userData = nullptr;

    for (auto *p : m_impl->autoPatchers) {
        m_impl->pc->destroyAutoPatcher(p);
    }
    m_impl->autoPatchers.clear();

    if (m_impl->zInput != nullptr) {
        m_impl->closeInputArchive();
    }
    if (m_impl->zOutput != nullptr) {
        m_impl->closeOutputArchive();
    }

    if (m_impl->cancelled) {
        m_impl->error = ErrorCode::PatchingCancelled;
        return false;
    }

    return ret;
}

struct CopySpec {
    std::string source;
    std::string target;
};

bool MultiBootPatcher::Impl::patchZip()
{
    std::unordered_set<std::string> excludeFromPass1;

    auto *standardAp = pc->createAutoPatcher("StandardPatcher", info);
    if (!standardAp) {
        error = ErrorCode::AutoPatcherCreateError;
        return false;
    }

    auto *mountCmdAp = pc->createAutoPatcher("MountCmdPatcher", info);
    if (!mountCmdAp) {
        error = ErrorCode::AutoPatcherCreateError;
        return false;
    }

    autoPatchers.push_back(standardAp);
    autoPatchers.push_back(mountCmdAp);

    for (auto *ap : autoPatchers) {
        // AutoPatcher files should be excluded from the first pass
        for (auto const &file : ap->existingFiles()) {
            excludeFromPass1.insert(file);
        }
    }

    // Unlike the old patcher, we'll write directly to the new file
    if (!openOutputArchive()) {
        return false;
    }

    zipFile zf = MinizipUtils::ctxGetZipFile(zOutput);

    if (cancelled) return false;

    MinizipUtils::ArchiveStats stats;
    auto result = MinizipUtils::archiveStats(info->inputPath(), &stats,
                                             std::vector<std::string>());
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    maxBytes = stats.totalSize;

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

    // +1 for info.prop
    // +1 for device.json
    maxFiles = stats.files + toCopy.size() + 2;
    updateFiles(files, maxFiles);

    if (!openInputArchive()) {
        return false;
    }

    // Create temporary dir for extracted files for autopatchers
    std::string tempDir = FileUtils::createTemporaryDir(pc->tempDirectory());

    if (!pass1(tempDir, excludeFromPass1)) {
        io::deleteRecursively(tempDir);
        return false;
    }

    if (cancelled) return false;

    // On the second pass, run the autopatchers on the rest of the files

    if (!pass2(tempDir, excludeFromPass1)) {
        io::deleteRecursively(tempDir);
        return false;
    }

    io::deleteRecursively(tempDir);

    for (const CopySpec &spec : toCopy) {
        if (cancelled) return false;

        updateFiles(++files, maxFiles);
        updateDetails(spec.target);

        result = MinizipUtils::addFile(zf, spec.target, spec.source);
        if (result != ErrorCode::NoError) {
            error = result;
            return false;
        }
    }

    if (cancelled) return false;

    updateFiles(++files, maxFiles);
    updateDetails("multiboot/info.prop");

    const std::string infoProp = createInfoProp(pc, info->romId(), false);
    result = MinizipUtils::addFile(
            zf, "multiboot/info.prop",
            std::vector<unsigned char>(infoProp.begin(), infoProp.end()));
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    updateFiles(++files, maxFiles);
    updateDetails("multiboot/device.json");

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
bool MultiBootPatcher::Impl::pass1(const std::string &temporaryDir,
                                   const std::unordered_set<std::string> &exclude)
{
    unzFile uf = MinizipUtils::ctxGetUnzFile(zInput);
    zipFile zf = MinizipUtils::ctxGetZipFile(zOutput);

    int ret = unzGoToFirstFile(uf);
    if (ret != UNZ_OK) {
        error = ErrorCode::ArchiveReadHeaderError;
        return false;
    }

    do {
        if (cancelled) return false;

        unz_file_info64 fi;
        std::string curFile;

        if (!MinizipUtils::getInfo(uf, &fi, &curFile)) {
            error = ErrorCode::ArchiveReadHeaderError;
            return false;
        }

        updateFiles(++files, maxFiles);
        updateDetails(curFile);

        // Skip files that should be patched and added in pass 2
        if (exclude.find(curFile) != exclude.end()) {
            if (!MinizipUtils::extractFile(uf, temporaryDir)) {
                error = ErrorCode::ArchiveReadDataError;
                return false;
            }
            continue;
        }

        // Rename the installer for mbtool
        if (curFile == "META-INF/com/google/android/update-binary") {
            curFile = "META-INF/com/google/android/update-binary.orig";
        }

        if (!MinizipUtils::copyFileRaw(uf, zf, curFile, &laProgressCb, this)) {
            LOGW("minizip: Failed to copy raw data: %s", curFile.c_str());
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
bool MultiBootPatcher::Impl::pass2(const std::string &temporaryDir,
                                   const std::unordered_set<std::string> &files)
{
    zipFile zf = MinizipUtils::ctxGetZipFile(zOutput);

    for (auto *ap : autoPatchers) {
        if (cancelled) return false;
        if (!ap->patchFiles(temporaryDir)) {
            error = ap->error();
            return false;
        }
    }

    // TODO Headers are being discarded

    for (auto const &file : files) {
        if (cancelled) return false;

        ErrorCode ret;

        if (file == "META-INF/com/google/android/update-binary") {
            ret = MinizipUtils::addFile(
                    zf,
                    "META-INF/com/google/android/update-binary.orig",
                    temporaryDir + "/" + file);
        } else {
            ret = MinizipUtils::addFile(
                    zf,
                    file,
                    temporaryDir + "/" + file);
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

bool MultiBootPatcher::Impl::openInputArchive()
{
    assert(zInput == nullptr);

    zInput = MinizipUtils::openInputFile(info->inputPath());

    if (!zInput) {
        LOGE("minizip: Failed to open for reading: %s",
             info->inputPath().c_str());
        error = ErrorCode::ArchiveReadOpenError;
        return false;
    }

    return true;
}

void MultiBootPatcher::Impl::closeInputArchive()
{
    assert(zInput != nullptr);

    int ret = MinizipUtils::closeInputFile(zInput);
    if (ret != UNZ_OK) {
        LOGW("minizip: Failed to close archive (error code: %d)", ret);
    }

    zInput = nullptr;
}

bool MultiBootPatcher::Impl::openOutputArchive()
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

void MultiBootPatcher::Impl::closeOutputArchive()
{
    assert(zOutput != nullptr);

    int ret = MinizipUtils::closeOutputFile(zOutput);
    if (ret != ZIP_OK) {
        LOGW("minizip: Failed to close archive (error code: %d)", ret);
    }

    zOutput = nullptr;
}

void MultiBootPatcher::Impl::updateProgress(uint64_t bytes, uint64_t maxBytes)
{
    if (progressCb) {
        progressCb(bytes, maxBytes, userData);
    }
}

void MultiBootPatcher::Impl::updateFiles(uint64_t files, uint64_t maxFiles)
{
    if (filesCb) {
        filesCb(files, maxFiles, userData);
    }
}

void MultiBootPatcher::Impl::updateDetails(const std::string &msg)
{
    if (detailsCb) {
        detailsCb(msg, userData);
    }
}

void MultiBootPatcher::Impl::laProgressCb(uint64_t bytes, void *userData)
{
    Impl *impl = static_cast<Impl *>(userData);
    impl->updateProgress(impl->bytes + bytes, impl->maxBytes);
}

template<typename SomeType, typename Predicate>
inline std::size_t insertAndFindMax(const std::vector<SomeType> &list1,
                                    std::vector<std::string> &list2,
                                    Predicate pred)
{
    std::size_t max = 0;
    for (const SomeType &st : list1) {
        std::size_t temp = pred(st, list2);
        if (temp > max) {
            max = temp;
        }
    }
    return max;
}

std::string MultiBootPatcher::createInfoProp(const PatcherConfig * const pc,
                                             const std::string &romId,
                                             bool always_patch_ramdisk)
{
    (void) pc;

    std::string out;

    out +=
"# [Autogenerated by libmbp]\n"
"#\n"
"# Blank lines are ignored. Lines beginning with '#' are comments and are also\n"
"# ignored. Before changing any fields, please read its description. Invalid\n"
"# values may lead to unexpected behavior when this zip file is installed.\n"
"\n"
"\n"
"# mbtool.installer.version\n"
"# ------------------------\n"
"# This field is the version of libmbp and mbtool used to patch and install this\n"
"# file, respectively.\n"
"#\n";

    out += "mbtool.installer.version=";
    out += mb::version();
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
    out += romId;
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
