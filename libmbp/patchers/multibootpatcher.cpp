/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#include "patchers/multibootpatcher.h"

#include <cassert>

#include <unordered_set>

#include <archive.h>
#include <archive_entry.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>

#include "bootimage.h"
#include "cpiofile.h"
#include "patcherconfig.h"
#include "private/fileutils.h"
#include "private/logging.h"


#define RETURN_IF_CANCELLED \
    if (cancelled) { \
        return false; \
    }

#define RETURN_ERROR_IF_CANCELLED \
    if (m_impl->cancelled) { \
        m_impl->error = PatcherError::createCancelledError( \
                MBP::ErrorCode::PatchingCancelled); \
        return false; \
    }

#define MAX_PROGRESS_CB(x) \
    if (maxProgressCb != nullptr) { \
        maxProgressCb(x, userData); \
    }
#define PROGRESS_CB(x) \
    if (progressCb != nullptr) { \
        progressCb(x, userData); \
    }
#define DETAILS_CB(x) \
    if (detailsCb != nullptr) { \
        detailsCb(x, userData); \
    }

// TODO TODO TODO
#define tr(x) (x)
// TODO TODO TODO


/*! \cond INTERNAL */
class MultiBootPatcher::Impl
{
public:
    Impl(MultiBootPatcher *parent) : m_parent(parent) {}

    PatcherConfig *pc;
    const FileInfo *info;

    int progress;
    volatile bool cancelled;

    PatcherError error;

    // Callbacks
    MaxProgressUpdatedCallback maxProgressCb;
    ProgressUpdatedCallback progressCb;
    DetailsUpdatedCallback detailsCb;
    void *userData;

    // Patching
    archive *aInput = nullptr;
    archive *aOutput = nullptr;
    std::vector<AutoPatcher *> autoPatchers;

    bool patchBootImage(std::vector<unsigned char> *data);
    bool patchZip();

    bool pass1(archive * const aOutput,
               const std::string &temporaryDir,
               const std::unordered_set<std::string> &exclude);
    bool pass2(archive * const aOutput,
               const std::string &temporaryDir,
               const std::unordered_set<std::string> &files);
    bool openInputArchive();
    void closeInputArchive();
    bool openOutputArchive();
    void closeOutputArchive();

private:
    MultiBootPatcher *m_parent;
};
/*! \endcond */


const std::string MultiBootPatcher::Id("MultiBootPatcher");
const std::string MultiBootPatcher::Name = tr("Multi Boot Patcher");


MultiBootPatcher::MultiBootPatcher(PatcherConfig * const pc)
    : m_impl(new Impl(this))
{
    m_impl->pc = pc;
}

MultiBootPatcher::~MultiBootPatcher()
{
}

PatcherError MultiBootPatcher::error() const
{
    return m_impl->error;
}

std::string MultiBootPatcher::id() const
{
    return Id;
}

std::string MultiBootPatcher::name() const
{
    return Name;
}

bool MultiBootPatcher::usesPatchInfo() const
{
    return true;
}

void MultiBootPatcher::setFileInfo(const FileInfo * const info)
{
    m_impl->info = info;
}

std::string MultiBootPatcher::newFilePath()
{
    assert(m_impl->info != nullptr);

    boost::filesystem::path path(m_impl->info->filename());
    boost::filesystem::path fileName = path.stem();
    fileName += "_patched";
    fileName += path.extension();

    if (path.has_parent_path()) {
        return (path.parent_path() / fileName).string();
    } else {
        return fileName.string();
    }
}

void MultiBootPatcher::cancelPatching()
{
    m_impl->cancelled = true;
}

bool MultiBootPatcher::patchFile(MaxProgressUpdatedCallback maxProgressCb,
                                 ProgressUpdatedCallback progressCb,
                                 DetailsUpdatedCallback detailsCb,
                                 void *userData)
{
    m_impl->cancelled = false;

    assert(m_impl->info != nullptr);

    if (!boost::iends_with(m_impl->info->filename(), ".zip")) {
        m_impl->error = PatcherError::createSupportedFileError(
                MBP::ErrorCode::OnlyZipSupported, Id);
        return false;
    }

    m_impl->maxProgressCb = maxProgressCb;
    m_impl->progressCb = progressCb;
    m_impl->detailsCb = detailsCb;
    m_impl->userData = userData;

    bool ret = m_impl->patchZip();

    m_impl->maxProgressCb = nullptr;
    m_impl->progressCb = nullptr;
    m_impl->detailsCb = nullptr;
    m_impl->userData = nullptr;

    for (auto *p : m_impl->autoPatchers) {
        m_impl->pc->destroyAutoPatcher(p);
    }
    m_impl->autoPatchers.clear();

    if (m_impl->aInput != nullptr) {
        m_impl->closeInputArchive();
    }
    if (m_impl->aOutput != nullptr) {
        m_impl->closeOutputArchive();
    }

    RETURN_ERROR_IF_CANCELLED

    return ret;
}

bool MultiBootPatcher::Impl::patchBootImage(std::vector<unsigned char> *data)
{
    // Determine patchinfo key for the current file
    const std::string key = info->patchInfo()->keyFromFilename(info->filename());

    BootImage bi;
    if (!bi.load(*data)) {
        error = bi.error();
        return false;
    }

    // Load the ramdisk cpio
    CpioFile cpio;
    if (!cpio.load(bi.ramdiskImage())) {
        error = cpio.error();
        return false;
    }

    RETURN_IF_CANCELLED

    auto *rp = pc->createRamdiskPatcher(
            info->patchInfo()->ramdisk(key), info, &cpio);
    if (!rp) {
        error = PatcherError::createPatcherCreationError(
                MBP::ErrorCode::RamdiskPatcherCreateError,
                info->patchInfo()->ramdisk(key));
        return false;
    }

    if (!rp->patchRamdisk()) {
        error = rp->error();
        pc->destroyRamdiskPatcher(rp);
        return false;
    }

    pc->destroyRamdiskPatcher(rp);

    RETURN_IF_CANCELLED

    // Add mbtool
    const std::string mbtool("mbtool");

    if (cpio.exists(mbtool)) {
        cpio.remove(mbtool);
    }

    if (!cpio.addFile(pc->dataDirectory() + "/binaries/android/"
            + info->device()->architecture() + "/mbtool", mbtool, 0750)) {
        error = cpio.error();
        return false;
    }

    RETURN_IF_CANCELLED

    std::vector<unsigned char> newRamdisk;
    if (!cpio.createData(&newRamdisk, true)) {
        error = cpio.error();
        return false;
    }
    bi.setRamdiskImage(std::move(newRamdisk));

    *data = bi.create();

    RETURN_IF_CANCELLED

    return true;
}

static std::string createTemporaryDir(const std::string &directory)
{
#ifdef __ANDROID__
    // For whatever reason boost::filesystem::unique_path() returns an empty
    // path on pre-lollipop ROMs

    boost::filesystem::path dir(directory);
    dir /= "mbp-XXXXXX";
    const std::string dirTemplate = dir.string();

    // mkdtemp modifies buffer
    std::vector<char> buf(dirTemplate.begin(), dirTemplate.end());
    buf.push_back('\0');

    if (mkdtemp(buf.data())) {
        return std::string(buf.data());
    }

    return std::string();
#else
    int count = 256;

    while (count > 0) {
        boost::filesystem::path dir(directory);
        dir /= "mbp-%%%%%%";
        auto path = boost::filesystem::unique_path(dir);
        if (boost::filesystem::create_directory(path)) {
            return path.string();
        }
        count--;
    }

    // Like Qt, we'll assume that 256 tries is enough ...
    return std::string();
#endif
}

bool MultiBootPatcher::Impl::patchZip()
{
    progress = 0;

    auto const key = info->patchInfo()->keyFromFilename(info->filename());

    std::unordered_set<std::string> excludeFromPass1;

    for (auto const &item : info->patchInfo()->autoPatchers(key)) {
        auto args = info->patchInfo()->autoPatcherArgs(key, item);

        auto *ap = pc->createAutoPatcher(item, info, args);
        if (!ap) {
            error = PatcherError::createPatcherCreationError(
                    MBP::ErrorCode::AutoPatcherCreateError, item);
            return false;
        }

        // TODO: AutoPatcher::newFiles() is not supported yet

        std::vector<std::string> existingFiles = ap->existingFiles();
        if (existingFiles.empty()) {
            pc->destroyAutoPatcher(ap);
            continue;
        }

        autoPatchers.push_back(ap);

        // AutoPatcher files should be excluded from the first pass
        for (auto const &file : existingFiles) {
            excludeFromPass1.insert(file);
        }
    }

    // Unlike the old patcher, we'll write directly to the new file
    if (!openOutputArchive()) {
        return false;
    }

    RETURN_IF_CANCELLED

    DETAILS_CB(tr("Counting number of files in zip file ..."));

    unsigned int count;
    auto result = FileUtils::laCountFiles(info->filename(), &count);
    if (result.errorCode() != MBP::ErrorCode::NoError) {
        error = result;
        return false;
    }

    RETURN_IF_CANCELLED

    // +1 for mbtool_recovery (update-binary)
    // +1 for mbtool
    // +1 for aromawrapper.sh
    // +1 for e2fsck
    // +1 for resize2fs
    // +1 for unzip
    // +1 for device
    // +1 for libmbp-mini.so
    MAX_PROGRESS_CB(count + 8);
    PROGRESS_CB(progress);

    if (!openInputArchive()) {
        return false;
    }

    // Create temporary dir for extracted files for autopatchers
    std::string tempDir = createTemporaryDir(pc->tempDirectory());

    if (!pass1(aOutput, tempDir, excludeFromPass1)) {
        boost::filesystem::remove_all(tempDir);
        return false;
    }

    RETURN_IF_CANCELLED

    // On the second pass, run the autopatchers on the rest of the files

    if (!pass2(aOutput, tempDir, excludeFromPass1)) {
        boost::filesystem::remove_all(tempDir);
        return false;
    }

    boost::filesystem::remove_all(tempDir);

    RETURN_IF_CANCELLED

    PROGRESS_CB(++progress);
    DETAILS_CB("META-INF/com/google/android/update-binary");

    // Add mbtool_recovery
    result = FileUtils::laAddFile(
            aOutput, "META-INF/com/google/android/update-binary",
            pc->dataDirectory() + "/binaries/android/"
                    + info->device()->architecture() + "/mbtool_recovery");
    if (result.errorCode() != MBP::ErrorCode::NoError) {
        error = result;
        return false;
    }

    RETURN_IF_CANCELLED

    PROGRESS_CB(++progress);
    DETAILS_CB("multiboot/mbtool");

    // Add mbtool
    result = FileUtils::laAddFile(
            aOutput, "multiboot/mbtool",
            pc->dataDirectory() + "/binaries/android/"
                    + info->device()->architecture() + "/mbtool");
    if (result.errorCode() != MBP::ErrorCode::NoError) {
        error = result;
        return false;
    }

    RETURN_IF_CANCELLED

    PROGRESS_CB(++progress);
    DETAILS_CB("multiboot/aromawrapper.zip");

    // Add aromawrapper.zip
    result = FileUtils::laAddFile(
            aOutput, "multiboot/aromawrapper.zip",
            pc->dataDirectory() + "/aromawrapper.zip");
    if (result.errorCode() != MBP::ErrorCode::NoError) {
        error = result;
        return false;
    }

    RETURN_IF_CANCELLED

    PROGRESS_CB(++progress);
    DETAILS_CB("multiboot/e2fsck");

    // Add e2fsck
    result = FileUtils::laAddFile(
            aOutput, "multiboot/e2fsck",
            pc->dataDirectory() + "/binaries/android/"
                    + info->device()->architecture() + "/e2fsck");
    if (result.errorCode() != MBP::ErrorCode::NoError) {
        error = result;
        return false;
    }

    RETURN_IF_CANCELLED

    PROGRESS_CB(++progress);
    DETAILS_CB("multiboot/resize2fs");

    // Add resize2fs
    result = FileUtils::laAddFile(
            aOutput, "multiboot/resize2fs",
            pc->dataDirectory() + "/binaries/android/"
                    + info->device()->architecture() + "/resize2fs");
    if (result.errorCode() != MBP::ErrorCode::NoError) {
        error = result;
        return false;
    }

    RETURN_IF_CANCELLED

    PROGRESS_CB(++progress);
    DETAILS_CB("multiboot/unzip");

    // Add unzip.tar.xz
    result = FileUtils::laAddFile(
            aOutput, "multiboot/unzip",
            pc->dataDirectory() + "/binaries/android/"
                    + info->device()->architecture() + "/unzip");
    if (result.errorCode() != MBP::ErrorCode::NoError) {
        error = result;
        return false;
    }

    RETURN_IF_CANCELLED

    PROGRESS_CB(++progress);
    DETAILS_CB("multiboot/device");

    // In the future, we may add more to the device file, but for now, we write
    // the codename and mbtool will dlopen() libmbp to retrieve additional
    // information
    const std::string codename = info->device()->codename();
    result = FileUtils::laAddFile(
            aOutput, "multiboot/device",
            std::vector<unsigned char>(codename.begin(), codename.end()));
    if (result.errorCode() != MBP::ErrorCode::NoError) {
        error = result;
        return false;
    }

    RETURN_IF_CANCELLED

    PROGRESS_CB(++progress);
    DETAILS_CB("multiboot/libmbp-mini.so");

    result = FileUtils::laAddFile(
            aOutput, "multiboot/libmbp-mini.so",
            pc->dataDirectory() + "/libraries/"
                    + info->device()->architecture() + "/libmbp-mini.so");
    if (result.errorCode() != MBP::ErrorCode::NoError) {
        error = result;
        return false;
    }

    RETURN_IF_CANCELLED

    return true;
}

/*!
 * \brief First pass of patching operation
 *
 * This performs the following operations:
 *
 * - If the PatchInfo has the `hasBootImage` parameter set to true for the
 *   current file, then patch the boot images and copy them to the output file.
 * - Files needed by an AutoPatcher are extracted to the temporary directory.
 * - Otherwise, the file is copied directly to the output zip.
 */
bool MultiBootPatcher::Impl::pass1(archive * const aOutput,
                                   const std::string &temporaryDir,
                                   const std::unordered_set<std::string> &exclude)
{
    const std::string key = info->patchInfo()->keyFromFilename(info->filename());

    // Boot image params
    bool hasBootImage = info->patchInfo()->hasBootImage(key);
    auto piBootImages = info->patchInfo()->bootImages(key);

    archive_entry *entry;

    while (archive_read_next_header(aInput, &entry) == ARCHIVE_OK) {
        RETURN_IF_CANCELLED

        const std::string curFile = archive_entry_pathname(entry);

        PROGRESS_CB(++progress);
        DETAILS_CB(curFile);

        // Skip files that should be patched and added in pass 2
        if (exclude.find(curFile) != exclude.end()) {
            if (!FileUtils::laExtractFile(aInput, entry, temporaryDir)) {
                error = PatcherError::createArchiveError(
                        MBP::ErrorCode::ArchiveReadDataError, curFile);
                return false;
            }
            continue;
        }

        // Try to patch the patchinfo-defined boot images as well as
        // files that end in a common boot image extension

        bool inList = std::find(piBootImages.begin(), piBootImages.end(),
                                curFile) != piBootImages.end();
        bool isExtImg = boost::ends_with(curFile, ".img");
        bool isExtLok = boost::ends_with(curFile, ".lok");

        if (hasBootImage && (inList || isExtImg || isExtLok)) {
            // Load the file into memory
            std::vector<unsigned char> data;

            if (!FileUtils::laReadToByteArray(aInput, entry, &data)) {
                Log::log(Log::Warning, "libarchive: %s", archive_error_string(aInput));
                error = PatcherError::createArchiveError(
                        MBP::ErrorCode::ArchiveReadDataError, curFile);
                return false;
            }

            // If the file contains the boot image magic string, then
            // assume it really is a boot image and patch it
            static const char *magic = "ANDROID!";
            auto it = std::search(data.begin(), data.end(), magic, magic + 8);
            if (it != data.end() && it - data.begin() <= 512) {
                if (!patchBootImage(&data)) {
                    return false;
                }

                archive_entry_set_size(entry, data.size());
            }

            // Write header to new file
            if (archive_write_header(aOutput, entry) != ARCHIVE_OK) {
                Log::log(Log::Warning, "libarchive: %s", archive_error_string(aOutput));
                error = PatcherError::createArchiveError(
                        MBP::ErrorCode::ArchiveWriteHeaderError, curFile);
                return false;
            }

            archive_write_data(aOutput, data.data(), data.size());
        } else {
            // Directly copy other files to the output zip

            // Rename the installer for mbtool
            if (archive_entry_pathname(entry) ==
                    std::string("META-INF/com/google/android/update-binary")) {
                archive_entry_set_pathname(
                        entry, "META-INF/com/google/android/update-binary.orig");
            }

            if (archive_write_header(aOutput, entry) != ARCHIVE_OK) {
                Log::log(Log::Warning, "libarchive: %s", archive_error_string(aOutput));
                error = PatcherError::createArchiveError(
                        MBP::ErrorCode::ArchiveWriteHeaderError, curFile);
                return false;
            }

            if (!FileUtils::laCopyData(aInput, aOutput)) {
                Log::log(Log::Warning, "libarchive: %s", archive_error_string(aInput));
                error = PatcherError::createArchiveError(
                        MBP::ErrorCode::ArchiveWriteDataError, curFile);
                return false;
            }
        }
    }

    RETURN_IF_CANCELLED

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
bool MultiBootPatcher::Impl::pass2(archive * const aOutput,
                                   const std::string &temporaryDir,
                                   const std::unordered_set<std::string> &files)
{
    for (auto *ap : autoPatchers) {
        RETURN_IF_CANCELLED
        if (!ap->patchFiles(temporaryDir)) {
            error = ap->error();
            return false;
        }
    }

    // TODO Headers are being discarded

    for (auto const &file : files) {
        RETURN_IF_CANCELLED

        PatcherError ret;

        if (file == "META-INF/com/google/android/update-binary") {
            ret = FileUtils::laAddFile(
                    aOutput,
                    "META-INF/com/google/android/update-binary.orig",
                    temporaryDir + "/" + file);
        } else {
            ret = FileUtils::laAddFile(
                    aOutput,
                    file,
                    temporaryDir + "/" + file);
        }

        if (ret.errorCode() != MBP::ErrorCode::NoError) {
            error = ret;
            return false;
        }
    }

    RETURN_IF_CANCELLED

    return true;
}

bool MultiBootPatcher::Impl::openInputArchive()
{
    assert(aInput == nullptr);

    aInput = archive_read_new();

    archive_read_support_filter_none(aInput);
    archive_read_support_format_zip(aInput);

    int ret = archive_read_open_filename(
            aInput, info->filename().c_str(), 10240);
    if (ret != ARCHIVE_OK) {
        Log::log(Log::Warning, "libarchive: %s", archive_error_string(aInput));
        archive_read_free(aInput);
        aInput = nullptr;

        error = PatcherError::createArchiveError(
                MBP::ErrorCode::ArchiveReadOpenError, info->filename());
        return false;
    }

    return true;
}

void MultiBootPatcher::Impl::closeInputArchive()
{
    assert(aInput != nullptr);

    archive_read_free(aInput);
    aInput = nullptr;
}

bool MultiBootPatcher::Impl::openOutputArchive()
{
    assert(aOutput == nullptr);

    aOutput = archive_write_new();

    archive_write_set_format_zip(aOutput);
    archive_write_add_filter_none(aOutput);

    const std::string newPath = m_parent->newFilePath();
    int ret = archive_write_open_filename(aOutput, newPath.c_str());
    if (ret != ARCHIVE_OK) {
        Log::log(Log::Warning, "libarchive: %s", archive_error_string(aOutput));
        archive_write_free(aOutput);
        aOutput = nullptr;

        error = PatcherError::createArchiveError(
                MBP::ErrorCode::ArchiveWriteOpenError, newPath);
        return false;
    }

    return true;
}

void MultiBootPatcher::Impl::closeOutputArchive()
{
    assert(aOutput != nullptr);

    archive_write_free(aOutput);
    aOutput = nullptr;
}
