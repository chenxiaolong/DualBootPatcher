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

#include "patchers/multiboot/multibootpatcher.h"

#include <archive.h>
#include <archive_entry.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>

#include "bootimage.h"
#include "cpiofile.h"
#include "patcherpaths.h"
#include "private/fileutils.h"
#include "private/logging.h"


#define RETURN_IF_CANCELLED \
    if (cancelled) { \
        return false; \
    }

#define RETURN_IF_CANCELLED_AND_FREE_READ(x) \
    if (cancelled) { \
        archive_read_free(x); \
        return false; \
    }

#define RETURN_IF_CANCELLED_AND_FREE_WRITE(x) \
    if (cancelled) { \
        archive_write_free(x); \
        return false; \
    }

#define RETURN_ERROR_IF_CANCELLED \
    if (m_impl->cancelled) { \
        m_impl->error = PatcherError::createCancelledError( \
                PatcherError::PatchingCancelled); \
        return false; \
    }

// TODO TODO TODO
#define tr(x) (x)
// TODO TODO TODO


class MultiBootPatcher::Impl
{
public:
    Impl(MultiBootPatcher *parent) : m_parent(parent) {}

    const PatcherPaths *pp;
    const FileInfo *info;

    int progress;
    volatile bool cancelled;

    PatcherError error;

    bool patchBootImage(std::vector<unsigned char> *data);
    bool patchZip(MaxProgressUpdatedCallback maxProgressCb,
                  ProgressUpdatedCallback progressCb,
                  DetailsUpdatedCallback detailsCb,
                  void *userData);

    bool scanAndPatchBootImages(archive * const aOutput,
                                std::vector<std::string> *bootImages,
                                MaxProgressUpdatedCallback maxProgressCb,
                                ProgressUpdatedCallback progressCb,
                                DetailsUpdatedCallback detailsCb,
                                void *userData);
    bool scanAndPatchRemaining(archive * const aOutput,
                               const std::vector<std::string> &bootImages,
                               MaxProgressUpdatedCallback maxProgressCb,
                               ProgressUpdatedCallback progressCb,
                               DetailsUpdatedCallback detailsCb,
                               void *userData);

private:
    MultiBootPatcher *m_parent;
};


const std::string MultiBootPatcher::Id("MultiBootPatcher");
const std::string MultiBootPatcher::Name = tr("Multi Boot Patcher");


MultiBootPatcher::MultiBootPatcher(const PatcherPaths * const pp)
    : m_impl(new Impl(this))
{
    m_impl->pp = pp;
}

MultiBootPatcher::~MultiBootPatcher()
{
}

std::vector<PartitionConfig *> MultiBootPatcher::partConfigs()
{
    std::vector<PartitionConfig *> configs;

    // Create supported partition configurations
    std::string romInstalled = tr("ROM installed to %1%");

    PartitionConfig *dual = new PartitionConfig();

    dual->setId("dual");
    dual->setKernel("secondary");
    dual->setName(tr("Dual Boot"));
    dual->setDescription((boost::format(romInstalled) % "/system/dual").str());

    dual->setTargetSystem("/raw-system/dual");
    dual->setTargetCache("/raw-cache/dual");
    dual->setTargetData("/raw-data/dual");

    dual->setTargetSystemPartition(PartitionConfig::System);
    dual->setTargetCachePartition(PartitionConfig::Cache);
    dual->setTargetDataPartition(PartitionConfig::Data);

    configs.push_back(dual);

    // Add multi-slots
    const std::string multiSlotId("multi-slot-%1$d");
    for (int i = 0; i < 3; ++i) {
        PartitionConfig *multiSlot = new PartitionConfig();

        multiSlot->setId((boost::format(multiSlotId) % (i + 1)).str());
        multiSlot->setKernel((boost::format(multiSlotId) % (i + 1)).str());
        multiSlot->setName((boost::format(tr("Multi Boot Slot %1$d")) % (i + 1)).str());
        multiSlot->setDescription((boost::format(romInstalled)
                % (boost::format("/cache/multi-slot-%1$d/system")
                % (i + 1)).str()).str());

        multiSlot->setTargetSystem(
                (boost::format("/raw-cache/multi-slot-%1$d/system") % (i + 1)).str());
        multiSlot->setTargetCache(
                (boost::format("/raw-system/multi-slot-%1$d/cache") % (i + 1)).str());
        multiSlot->setTargetData(
                (boost::format("/raw-data/multi-slot-%1$d") % (i + 1)).str());

        multiSlot->setTargetSystemPartition(PartitionConfig::Cache);
        multiSlot->setTargetCachePartition(PartitionConfig::System);
        multiSlot->setTargetDataPartition(PartitionConfig::Data);

        configs.push_back(multiSlot);
    }

    return configs;
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

std::vector<std::string> MultiBootPatcher::supportedPartConfigIds() const
{
    // TODO: Loopify this
    std::vector<std::string> configs;
    configs.push_back("dual");
    configs.push_back("multi-slot-1");
    configs.push_back("multi-slot-2");
    configs.push_back("multi-slot-3");
    return configs;
}

void MultiBootPatcher::setFileInfo(const FileInfo * const info)
{
    m_impl->info = info;
}

std::string MultiBootPatcher::newFilePath()
{
    if (m_impl->info == nullptr) {
        Log::log(Log::Warning, "d->info cannot be null!");
        m_impl->error = PatcherError::createGenericError(
                PatcherError::ImplementationError);
        return std::string();
    }

    boost::filesystem::path path(m_impl->info->filename());
    boost::filesystem::path fileName = path.stem();
    fileName += "_";
    fileName += m_impl->info->partConfig()->id();
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

    if (m_impl->info == nullptr) {
        Log::log(Log::Warning, "d->info cannot be null!");
        m_impl->error = PatcherError::createGenericError(
                PatcherError::ImplementationError);
        return false;
    }

    if (!boost::iends_with(m_impl->info->filename(), ".zip")) {
        m_impl->error = PatcherError::createSupportedFileError(
                PatcherError::OnlyZipSupported, Id);
        return false;
    }

    bool ret = m_impl->patchZip(maxProgressCb, progressCb, detailsCb, userData);

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

    // Change the SELinux mode according to the device config
    const std::string cmdline = bi.kernelCmdline();
    if (!info->device()->selinux().empty()) {
        bi.setKernelCmdline(cmdline
                + " androidboot.selinux=" + info->device()->selinux());
    }

    // Load the ramdisk cpio
    CpioFile cpio;
    cpio.load(bi.ramdiskImage());

    RETURN_IF_CANCELLED

    std::shared_ptr<RamdiskPatcher> rp = pp->createRamdiskPatcher(
            info->patchInfo()->ramdisk(key), info, &cpio);
    if (!rp) {
        error = PatcherError::createPatcherCreationError(
                PatcherError::RamdiskPatcherCreateError,
                info->patchInfo()->ramdisk(key));
        return false;
    }

    if (!rp->patchRamdisk()) {
        error = rp->error();
        return false;
    }

    RETURN_IF_CANCELLED

    const std::string mountScript("init.multiboot.mounting.sh");
    const std::string mountScriptPath(pp->scriptsDirectory() + "/" + mountScript);

    std::vector<unsigned char> mountScriptContents;
    auto ret = FileUtils::readToMemory(mountScriptPath, &mountScriptContents);
    if (ret.errorCode() != PatcherError::NoError) {
        error = ret;
        return false;
    }

    info->partConfig()->replaceShellLine(&mountScriptContents, true);

    if (cpio.exists(mountScript)) {
        cpio.remove(mountScript);
    }

    cpio.addFile(mountScriptContents, mountScript, 0750);

    RETURN_IF_CANCELLED

    // Add busybox
    const std::string busybox("sbin/busybox-static");

    if (cpio.exists(busybox)) {
        cpio.remove(busybox);
    }

    cpio.addFile(pp->binariesDirectory() + "/busybox-static", busybox, 0750);

    RETURN_IF_CANCELLED

    // Add syncdaemon
    const std::string syncdaemon("sbin/syncdaemon");

    if (cpio.exists(syncdaemon)) {
        cpio.remove(syncdaemon);
    }

    cpio.addFile(pp->binariesDirectory() + "/"
            + "android" + "/"
            + info->device()->architecture() + "/"
            + "syncdaemon", syncdaemon, 0750);

    RETURN_IF_CANCELLED

    // Add patched init binary if it was specified by the patchinfo
    if (!info->patchInfo()->patchedInit(key).empty()) {
        if (cpio.exists("init")) {
            cpio.remove("init");
        }

        if (!cpio.addFile(pp->initsDirectory() + "/"
                + info->patchInfo()->patchedInit(key), "init", 0755)) {
            error = cpio.error();
            return false;
        }
    }

    RETURN_IF_CANCELLED

    std::vector<unsigned char> newRamdisk = cpio.createData(true);
    bi.setRamdiskImage(std::move(newRamdisk));

    *data = bi.create();

    RETURN_IF_CANCELLED

    return true;
}

bool MultiBootPatcher::Impl::patchZip(MaxProgressUpdatedCallback maxProgressCb,
                                      ProgressUpdatedCallback progressCb,
                                      DetailsUpdatedCallback detailsCb,
                                      void *userData)
{
    progress = 0;

    // Open the input and output zips with libarchive
    archive *aOutput;

    aOutput = archive_write_new();

    archive_write_set_format_zip(aOutput);
    archive_write_add_filter_none(aOutput);

    // Unlike the old patcher, we'll write directly to the new file
    const std::string newPath = m_parent->newFilePath();
    int ret = archive_write_open_filename(aOutput, newPath.c_str());
    if (ret != ARCHIVE_OK) {
        Log::log(Log::Warning, "libarchive: %s", archive_error_string(aOutput));
        archive_write_free(aOutput);

        error = PatcherError::createArchiveError(
                PatcherError::ArchiveWriteOpenError, newPath);
        return false;
    }

    RETURN_IF_CANCELLED_AND_FREE_WRITE(aOutput)

    if (detailsCb != nullptr) {
        detailsCb(tr("Counting number of files in zip file ..."), userData);
    }

    unsigned int count;
    auto result = FileUtils::laCountFiles(info->filename(), &count);
    if (result.errorCode() != PatcherError::NoError) {
        archive_write_free(aOutput);

        error = result;
        return false;
    }

    RETURN_IF_CANCELLED_AND_FREE_WRITE(aOutput)

    // +1 for dualboot.sh
    if (maxProgressCb != nullptr) {
        maxProgressCb(count + 1, userData);
    }
    if (progressCb != nullptr) {
        progressCb(progress, userData);
    }

    // Rather than using the extract->patch->repack process, we'll start
    // creating the new zip immediately and then patch the files as we
    // run into them.

    // On the initial pass, find all the boot images, patch them, and
    // write them to the new zip
    std::vector<std::string> bootImages;

    if (!scanAndPatchBootImages(aOutput, &bootImages, maxProgressCb,
                                progressCb, detailsCb, userData)) {
        archive_write_free(aOutput);
        return false;
    }

    RETURN_IF_CANCELLED_AND_FREE_WRITE(aOutput)

    // On the second pass, run the autopatchers on the rest of the files

    if (!scanAndPatchRemaining(aOutput, bootImages, maxProgressCb,
                               progressCb, detailsCb, userData)) {
        archive_write_free(aOutput);
        return false;
    }

    RETURN_IF_CANCELLED_AND_FREE_WRITE(aOutput)

    if (progressCb != nullptr) {
        progressCb(++progress, userData);
    }
    if (detailsCb != nullptr) {
        detailsCb("dualboot.sh", userData);
    }

    // Add dualboot.sh
    const std::string dualbootshPath(pp->scriptsDirectory() + "/dualboot.sh");
    std::vector<unsigned char> contents;
    auto error = FileUtils::readToMemory(dualbootshPath, &contents);
    if (error.errorCode() != PatcherError::NoError) {
        error = error;
        return false;
    }

    info->partConfig()->replaceShellLine(&contents);

    error = FileUtils::laAddFile(aOutput, "dualboot.sh", contents);
    if (error.errorCode() != PatcherError::NoError) {
        error = error;
        return false;
    }

    archive_write_free(aOutput);

    RETURN_IF_CANCELLED

    return true;
}

#include <iostream>

bool MultiBootPatcher::Impl::scanAndPatchBootImages(archive * const aOutput,
                                                    std::vector<std::string> *bootImages,
                                                    MaxProgressUpdatedCallback maxProgressCb,
                                                    ProgressUpdatedCallback progressCb,
                                                    DetailsUpdatedCallback detailsCb,
                                                    void *userData)
{
    (void) maxProgressCb;

    const std::string key = info->patchInfo()->keyFromFilename(
            info->filename());

    // If we don't expect boot images, don't scan for them
    if (!info->patchInfo()->hasBootImage(key)) {
        return true;
    }

    archive *aInput = archive_read_new();
    archive_read_support_filter_none(aInput);
    archive_read_support_format_zip(aInput);

    int ret = archive_read_open_filename(
            aInput, info->filename().c_str(), 10240);
    if (ret != ARCHIVE_OK) {
        Log::log(Log::Warning, "libarchive: %s", archive_error_string(aInput));
        archive_read_free(aInput);

        error = PatcherError::createArchiveError(
                PatcherError::ArchiveReadOpenError, info->filename());
        return false;
    }

    RETURN_IF_CANCELLED_AND_FREE_READ(aInput)

    archive_entry *entry;

    while (archive_read_next_header(aInput, &entry) == ARCHIVE_OK) {
        RETURN_IF_CANCELLED_AND_FREE_READ(aInput)

        const std::string curFile = archive_entry_pathname(entry);

        // Try to patch the patchinfo-defined boot images as well as
        // files that end in a common boot image extension
        auto list = info->patchInfo()->bootImages(key);

        if (std::find(list.begin(), list.end(), curFile) != list.end()
                || boost::ends_with(curFile, ".img")
                || boost::ends_with(curFile, ".lok")) {
            if (progressCb != nullptr) {
                progressCb(++progress, userData);
            }
            if (detailsCb != nullptr) {
                detailsCb(curFile, userData);
            }

            std::vector<unsigned char> data;

            if (!FileUtils::laReadToByteArray(aInput, entry, &data)) {
                Log::log(Log::Warning, "libarchive: %s", archive_error_string(aInput));
                archive_read_free(aInput);

                error = PatcherError::createArchiveError(
                        PatcherError::ArchiveReadDataError, curFile);
                return false;
            }

            // Make sure the file is a boot iamge and if it is, patch it
            const char *magic = "ANDROID!";
            auto it = std::search(data.begin(), data.end(), magic, magic + 8);
            if (it != data.end() && it - data.begin() <= 512) {
                bootImages->push_back(curFile);

                if (!patchBootImage(&data)) {
                    archive_read_free(aInput);
                    return false;
                }

                archive_entry_set_size(entry, data.size());
            }

            // Write header to new file
            if (archive_write_header(aOutput, entry) != ARCHIVE_OK) {
                Log::log(Log::Warning, "libarchive: %s", archive_error_string(aOutput));
                archive_read_free(aInput);

                error = PatcherError::createArchiveError(
                        PatcherError::ArchiveWriteHeaderError, curFile);
                return false;
            }

            archive_write_data(aOutput, data.data(), data.size());
        } else {
            // Don't process any non-boot-image files
            archive_read_data_skip(aInput);
        }
    }

    archive_read_free(aInput);

    RETURN_IF_CANCELLED

    return true;
}

bool MultiBootPatcher::Impl::scanAndPatchRemaining(archive * const aOutput,
                                                   const std::vector<std::string> &bootImages,
                                                   MaxProgressUpdatedCallback maxProgressCb,
                                                   ProgressUpdatedCallback progressCb,
                                                   DetailsUpdatedCallback detailsCb,
                                                   void *userData)
{
    (void) maxProgressCb;

    const std::string key = info->patchInfo()->keyFromFilename(
            info->filename());

    archive *aInput = archive_read_new();
    archive_read_support_filter_none(aInput);
    archive_read_support_format_zip(aInput);

    int ret = archive_read_open_filename(
            aInput, info->filename().c_str(), 10240);
    if (ret != ARCHIVE_OK) {
        Log::log(Log::Warning, "libarchive: %s", archive_error_string(aInput));
        archive_read_free(aInput);

        error = PatcherError::createArchiveError(
                PatcherError::ArchiveReadOpenError, info->filename());
        return false;
    }

    RETURN_IF_CANCELLED_AND_FREE_READ(aInput)

    std::unordered_map<std::string, std::vector<std::shared_ptr<AutoPatcher>>> patcherFromFile;

    for (auto const &item : info->patchInfo()->autoPatchers(key)) {
        std::shared_ptr<AutoPatcher> ap = pp->createAutoPatcher(
                item.first, info, item.second);
        if (!ap) {
            archive_read_free(aInput);

            error = PatcherError::createPatcherCreationError(
                    PatcherError::AutoPatcherCreateError, item.first);
            return false;
        }

        // Insert autopatchers in the hash table to make it easier to
        // call them later on

        std::vector<std::string> existingFiles = ap->existingFiles();
        if (existingFiles.empty()) {
            archive_read_free(aInput);

            error = PatcherError::createGenericError(
                    PatcherError::ImplementationError);
            return false;
        }

        for (auto const &file : existingFiles) {
            patcherFromFile[file].push_back(ap);
        }
    }

    RETURN_IF_CANCELLED_AND_FREE_READ(aInput)

    archive_entry *entry;

    while (archive_read_next_header(aInput, &entry) == ARCHIVE_OK) {
        RETURN_IF_CANCELLED_AND_FREE_READ(aInput)

        const std::string curFile = archive_entry_pathname(entry);

        auto list = info->patchInfo()->bootImages(key);

        if (std::find(list.begin(), list.end(), curFile) != list.end()
                || boost::ends_with(curFile, ".img")
                || boost::ends_with(curFile, ".lok")) {
            // These have already been handled by scanAndPatchBootImages()
            archive_read_data_skip(aInput);
            continue;
        }

        if (progressCb != nullptr) {
            progressCb(++progress, userData);
        }
        if (detailsCb != nullptr) {
            detailsCb(curFile, userData);
        }

        // Go through all the autopatchers
        if (patcherFromFile.find(curFile) != patcherFromFile.end()) {
            std::vector<unsigned char> contents;
            if (!FileUtils::laReadToByteArray(aInput, entry, &contents)) {
                Log::log(Log::Warning, "libarchive: %s", archive_error_string(aInput));
                archive_read_free(aInput);

                error = PatcherError::createArchiveError(
                        PatcherError::ArchiveReadDataError, curFile);
                return false;
            }

            for (auto const &ap : patcherFromFile[curFile]) {
                if (!ap->patchFile(curFile, &contents, bootImages)) {
                    archive_read_free(aInput);

                    error = ap->error();
                    return false;
                }
            }

            archive_entry_set_size(entry, contents.size());

            if (archive_write_header(aOutput, entry) != ARCHIVE_OK) {
                Log::log(Log::Warning, "libarchive: %s", archive_error_string(aOutput));
                archive_read_free(aInput);

                error = PatcherError::createArchiveError(
                        PatcherError::ArchiveWriteHeaderError, curFile);
                return false;
            }

            archive_write_data(aOutput, contents.data(), contents.size());
        } else {
            if (archive_write_header(aOutput, entry) != ARCHIVE_OK) {
                Log::log(Log::Warning, "libarchive: %s", archive_error_string(aOutput));
                archive_read_free(aInput);

                error = PatcherError::createArchiveError(
                        PatcherError::ArchiveWriteHeaderError, curFile);
                return false;
            }

            if (!FileUtils::laCopyData(aInput, aOutput)) {
                Log::log(Log::Warning, "libarchive: %s", archive_error_string(aInput));
                archive_read_free(aInput);

                error = PatcherError::createArchiveError(
                        PatcherError::ArchiveWriteDataError, curFile);
                return false;
            }
        }
    }

    archive_read_free(aInput);
    return true;
}
