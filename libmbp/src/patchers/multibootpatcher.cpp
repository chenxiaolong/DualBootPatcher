/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mbp/patchers/multibootpatcher.h"

#include <algorithm>
#include <unordered_set>

#include <cassert>

#include "mblog/logging.h"
#include "mbpio/delete.h"

#include "mbp/bootimage.h"
#include "mbp/cpiofile.h"
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

bool MultiBootPatcher::Impl::patchZip()
{
    std::unordered_set<std::string> excludeFromPass1;

    auto *standardAp = pc->createAutoPatcher("StandardPatcher", info);
    if (!standardAp) {
        error = ErrorCode::AutoPatcherCreateError;
        return false;
    }

    auto *xposedAp = pc->createAutoPatcher("XposedPatcher", info);
    if (!xposedAp) {
        error = ErrorCode::AutoPatcherCreateError;
        return false;
    }

    autoPatchers.push_back(standardAp);
    autoPatchers.push_back(xposedAp);

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

    // +1 for mbtool_recovery (update-binary)
    // +1 for bb-wrapper.sh
    // +1 for info.prop
    maxFiles = stats.files + 3;
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

    if (cancelled) return false;

    updateFiles(++files, maxFiles);
    updateDetails("META-INF/com/google/android/update-binary");

    // Add mbtool_recovery
    result = MinizipUtils::addFile(
            zf, "META-INF/com/google/android/update-binary",
            pc->dataDirectory() + "/binaries/android/"
                    + info->device()->architecture() + "/mbtool_recovery");
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    updateFiles(++files, maxFiles);
    updateDetails("multiboot/bb-wrapper.sh");

    // Add bb-wrapper.sh
    result = MinizipUtils::addFile(
        zf, "multiboot/bb-wrapper.sh",
        pc->dataDirectory() + "/scripts/bb-wrapper.sh");
    if (result != ErrorCode::NoError) {
        error = result;
        return false;
    }

    if (cancelled) return false;

    updateFiles(++files, maxFiles);
    updateDetails("multiboot/info.prop");

    const std::string infoProp =
            createInfoProp(pc, info->device(), info->romId());
    result = MinizipUtils::addFile(
            zf, "multiboot/info.prop",
            std::vector<unsigned char>(infoProp.begin(), infoProp.end()));
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
 * - Patch boot images and copy them to the output zip.
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

        // Try to patch files that end in a common boot image extension

        bool isExtImg = StringUtils::ends_with(curFile, ".img");
        bool isExtLok = StringUtils::ends_with(curFile, ".lok");
        bool isExtGz = StringUtils::ends_with(curFile, ".gz");
        // Boot images should never be over about 30 MiB. This check is here so
        // the patcher won't try to read a multi-gigabyte system image into RAM
        bool isSizeOK = fi.uncompressed_size <= 30 * 1024 * 1024;

        if ((isExtImg || isExtLok || isExtGz) && isSizeOK) {
            // Load the file into memory
            std::vector<unsigned char> data;

            if (!MinizipUtils::readToMemory(uf, &data,
                                            &laProgressCb, this)) {
                error = ErrorCode::ArchiveReadDataError;
                return false;
            }

            if (isExtGz) {
                // Some zips build the boot image at install time and the zip
                // just includes the split out parts of the boot image
                if (!patchRamdisk(pc, info, &data, &error)) {
                    // Just ignore for now
                }
            } else {
                // If the file contains the boot image magic string, then
                // assume it really is a boot image and patch it
                if (BootImage::isValid(data.data(), data.size())) {
                    if (!patchBootImage(pc, info, &data, &error)) {
                        return false;
                    }
                }
            }

            // Update total size
            maxBytes += (data.size() - fi.uncompressed_size);

            auto ret2 = MinizipUtils::addFile(zf, curFile, data);
            if (ret2 != ErrorCode::NoError) {
                error = ret2;
                return false;
            }

            bytes += data.size();
        } else {
            // Directly copy other files to the output zip

            // Rename the installer for mbtool
            if (curFile == "META-INF/com/google/android/update-binary") {
                curFile = "META-INF/com/google/android/update-binary.orig";
            }

            if (!MinizipUtils::copyFileRaw(uf, zf, curFile,
                                           &laProgressCb, this)) {
                LOGW("minizip: Failed to copy raw data: %s", curFile.c_str());
                error = ErrorCode::ArchiveWriteDataError;
                return false;
            }

            bytes += fi.uncompressed_size;
        }
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

bool MultiBootPatcher::patchRamdisk(PatcherConfig * const pc,
                                    const FileInfo * const info,
                                    std::vector<unsigned char> *data,
                                    ErrorCode *errorOut)
{
    // Load the ramdisk cpio
    CpioFile cpio;
    if (!cpio.load(*data)) {
        if (errorOut) {
            *errorOut = cpio.error();
        }
        return false;
    }

    std::string rpId = info->device()->ramdiskPatcher();
    auto *rp = pc->createRamdiskPatcher(rpId, info, &cpio);
    if (!rp) {
        if (errorOut) {
            *errorOut = ErrorCode::RamdiskPatcherCreateError;
        }
        return false;
    }

    if (!rp->patchRamdisk()) {
        if (errorOut) {
            *errorOut = rp->error();
        }
        pc->destroyRamdiskPatcher(rp);
        return false;
    }

    pc->destroyRamdiskPatcher(rp);

    std::vector<unsigned char> newRamdisk;
    if (!cpio.createData(&newRamdisk)) {
        if (errorOut) {
            *errorOut = cpio.error();
        }
        return false;
    }

    data->swap(newRamdisk);

    return true;
}

bool MultiBootPatcher::patchBootImage(PatcherConfig * const pc,
                                      const FileInfo * const info,
                                      std::vector<unsigned char> *data,
                                      ErrorCode *errorOut)
{
    BootImage bi;
    if (!bi.load(*data)) {
        if (errorOut) {
            *errorOut = bi.error();
        }
        return false;
    }

    // Release memory since BootImage keeps a copy of the separate components
    data->clear();
    data->shrink_to_fit();

    std::vector<unsigned char> ramdiskImage = bi.ramdiskImage();
    if (!patchRamdisk(pc, info, &ramdiskImage, errorOut)) {
        return false;
    }

    bi.setRamdiskImage(std::move(ramdiskImage));

    if (!bi.create(data)) {
        if (errorOut) {
            *errorOut = bi.error();
        }
        return false;
    }

    return true;
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

std::string MultiBootPatcher::createTable(const PatcherConfig * const pc)
{
    std::string out;

    auto devices = pc->devices();
    std::vector<std::string> ids;
    std::vector<std::string> codenames;
    std::vector<std::string> names;

    std::size_t maxLenId = insertAndFindMax(devices, ids,
            [](Device *d, std::vector<std::string> &list) {
                list.push_back(d->id());
                return d->id().size();
            });
    std::size_t maxLenCodenames = insertAndFindMax(devices, codenames,
            [](Device *d, std::vector<std::string> &list) {
                auto codenames = d->codenames();
                std::string out = StringUtils::join(codenames, ", ");
                std::size_t len = out.size();
                list.push_back(std::move(out));
                return len;
            });
    std::size_t maxLenName = insertAndFindMax(devices, names,
            [](Device *d, std::vector<std::string> &list) {
                list.push_back(d->name());
                return d->name().size();
            });

    const std::string titleDevice = "Device";
    const std::string titleCodenames = "Codenames";
    const std::string titleName = "Name";

    if (titleDevice.size() > maxLenId) {
        maxLenId = titleDevice.size();
    }
    if (titleCodenames.size() > maxLenCodenames) {
        maxLenCodenames = titleCodenames.size();
    }
    if (titleName.size() > maxLenName) {
        maxLenName = titleName.size();
    }

    const std::string rowFmt =
            StringUtils::format("# | %%-%" PRIzu "s | %%-%" PRIzu "s | %%-%" PRIzu "s |\n",
                                maxLenId, maxLenCodenames, maxLenName);

    // Titles
    out += StringUtils::format(rowFmt.c_str(),
                               titleDevice.c_str(), titleCodenames.c_str(),
                               titleName.c_str());

    // Separator
    out += StringUtils::format("# |%s|%s|%s|\n",
                               std::string(maxLenId + 2, '-').c_str(),
                               std::string(maxLenCodenames + 2, '-').c_str(),
                               std::string(maxLenName + 2, '-').c_str());

    // Devices
    for (std::size_t i = 0; i < devices.size(); ++i) {
        out += StringUtils::format(rowFmt.c_str(),
                                   ids[i].c_str(), codenames[i].c_str(),
                                   names[i].c_str());
    }

    return out;
}

std::string MultiBootPatcher::createInfoProp(const PatcherConfig * const pc,
                                             const Device * const device,
                                             const std::string &romId)
{
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
    out += pc->version();
    out += "\n";

    out +=
"\n"
"\n"
"# mbtool.installer.device\n"
"# -----------------------\n"
"# This field specifies the target device for this zip file. Based on the value,\n"
"# mbtool will determine the appropriate partitions to use as well as other\n"
"# device-specific operations (eg. Loki for locked Galaxy S4 and LG G2\n"
"# bootloaders). The devices supported by mbtool are specified below.\n"
"#\n"
"# WARNING: Except for debugging purposes, this value should NEVER be changed.\n"
"# An incorrect value can hard-brick the device due to differences in the\n"
"# partition table.\n"
"#\n"
"# Supported devices:\n"
"#\n";

    out += createTable(pc);
    out += "#\n";
    out += "mbtool.installer.device=";
    out += device->id();
    out += "\n";

    out +=
"\n"
"\n"
"# mbtool.installer.ignore-codename\n"
"# --------------------------------\n"
"# The installer checks the device by comparing the devices codenames to the\n"
"# valid codenames in the table above. This value is useful when the device is\n"
"# a variant of a supported device (or very similar to one).\n"
"#\n"
"# For example, if 'mbtool.installer.device' is set to 'trlte' and this field is\n"
"# set to true, then mbtool would not check to see if the device's codename is\n"
"# 'trltetmo' or 'trltexx'.\n"
"#\n"
"mbtool.installer.ignore-codename=false\n"
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
    out += "\n\n";

    return out;
}

}
