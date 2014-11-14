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

#include "patchers/primaryupgrade/primaryupgradepatcher.h"

#include <cassert>

#include <archive.h>
#include <archive_entry.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>
#include <boost/regex.hpp>

#include "patcherconfig.h"
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

#define RETURN_IF_CANCELLED_AND_FREE_READ_WRITE(x, y) \
    if (cancelled) { \
        archive_read_free(x); \
        archive_write_free(y); \
        return false; \
    }

#define RETURN_ERROR_IF_CANCELLED \
    if (m_impl->cancelled) { \
        m_impl->error = PatcherError::createCancelledError( \
                MBP::ErrorCode::PatchingCancelled); \
        return false; \
    }


// TODO TODO TODO
#define tr(x) (x)
// TODO TODO TODO


/*! \cond INTERNAL */
class PrimaryUpgradePatcher::Impl
{
public:
    Impl(PrimaryUpgradePatcher *parent) : m_parent(parent) {}

    const PatcherConfig *pc;
    const FileInfo *info;

    int progress;
    bool cancelled;

    std::vector<std::string> ignoreFiles;

    PatcherError error;

    bool patchZip(MaxProgressUpdatedCallback maxProgressCb,
                  ProgressUpdatedCallback progressCb,
                  DetailsUpdatedCallback detailsCb,
                  void *userData);
    bool patchUpdaterScript(std::vector<unsigned char> *contents);

    std::vector<std::string>::iterator
    insertFormatSystem(std::vector<std::string>::iterator position,
                       std::vector<std::string> *lines, bool mount) const;

    std::vector<std::string>::iterator
    insertFormatCache(std::vector<std::string>::iterator position,
                      std::vector<std::string> *lines, bool mount) const;

private:
    PrimaryUpgradePatcher *m_parent;
};
/*! \endcond */


const std::string PrimaryUpgradePatcher::Id = "PrimaryUpgradePatcher";
const std::string PrimaryUpgradePatcher::Name = tr("Primary Upgrade Patcher");

const std::string UpdaterScript =
        "META-INF/com/google/android/updater-script";
const std::string Format =
        "run_program(\"/sbin/busybox\", \"find\", \"%1%\", \"-maxdepth\", \"1\", \"!\", \"-name\", \"multi-slot-*\", \"!\", \"-name\", \"dual\", \"-delete\");";
const std::string Mount =
        "run_program(\"/sbin/busybox\", \"mount\", \"%1%\");";
const std::string Unmount =
        "run_program(\"/sbin/busybox\", \"umount\", \"%1%\");";
const std::string TempDir = "/tmp/";
const std::string DualBootTool = "dualboot.sh";
const std::string SetFacl = "setfacl";
const std::string GetFacl = "getfacl";
const std::string SetFattr = "setfattr";
const std::string GetFattr = "getfattr";
const std::string PermTool = "backuppermtool.sh";
const std::string PermsBackup = "run_program(\"/tmp/%1%\", \"backup\", \"%2%\");";
const std::string PermsRestore = "run_program(\"/tmp/%1%\", \"restore\", \"%2%\");";
const std::string Extract = "package_extract_file(\"%1%\", \"%2%\");";
//const std::string MakeExecutable = "set_perm(0, 0, 0777, \"%1%\");";
//const std::string MakeExecutable =
//        "set_metadata(\"%1%\", \"uid\", 0, \"gid\", 0, \"mode\", 0777);";
const std::string MakeExecutable =
        "run_program(\"/sbin/busybox\", \"chmod\", \"0777\", \"%1%\");";
const std::string SetKernel =
        "run_program(\"/tmp/dualboot.sh\", \"set-multi-kernel\");";

static const std::string System = "/system";
static const std::string Cache = "/cache";

static const std::string AndroidBins = "android/%1%";


PrimaryUpgradePatcher::PrimaryUpgradePatcher(const PatcherConfig * const pc)
    : Patcher(), m_impl(new Impl(this))
{
    m_impl->pc = pc;

    m_impl->ignoreFiles.push_back(DualBootTool);
    m_impl->ignoreFiles.push_back(PermTool);
    m_impl->ignoreFiles.push_back(GetFacl);
    m_impl->ignoreFiles.push_back(SetFacl);
    m_impl->ignoreFiles.push_back(GetFattr);
    m_impl->ignoreFiles.push_back(SetFattr);
}

PrimaryUpgradePatcher::~PrimaryUpgradePatcher()
{
}

std::vector<PartitionConfig *> PrimaryUpgradePatcher::partConfigs()
{
    // Add primary upgrade partition configuration
    PartitionConfig *config = new PartitionConfig();
    config->setId("primaryupgrade");
    config->setKernel("primary");
    config->setName(tr("Primary ROM Upgrade"));
    config->setDescription(
        tr("Upgrade primary ROM without wiping other ROMs"));

    config->setTargetSystem("/system");
    config->setTargetCache("/cache");
    config->setTargetData("/data");

    config->setTargetSystemPartition(PartitionConfig::System);
    config->setTargetCachePartition(PartitionConfig::Cache);
    config->setTargetDataPartition(PartitionConfig::Data);

    std::vector<PartitionConfig *> configs;
    configs.push_back(config);

    return configs;
}

PatcherError PrimaryUpgradePatcher::error() const
{
    return m_impl->error;
}

std::string PrimaryUpgradePatcher::id() const
{
    return Id;
}

std::string PrimaryUpgradePatcher::name() const
{
    return Name;
}

bool PrimaryUpgradePatcher::usesPatchInfo() const
{
    return false;
}

std::vector<std::string> PrimaryUpgradePatcher::supportedPartConfigIds() const
{
    std::vector<std::string> configs;
    configs.push_back("primaryupgrade");
    return configs;
}

void PrimaryUpgradePatcher::setFileInfo(const FileInfo * const info)
{
    m_impl->info = info;
}

std::string PrimaryUpgradePatcher::newFilePath()
{
    assert(m_impl->info != nullptr);

    boost::filesystem::path path(m_impl->info->filename());
    boost::filesystem::path fileName = path.stem();
    fileName += "_primaryupgrade";
    fileName += path.extension();

    if (path.has_parent_path()) {
        return (path.parent_path() / fileName).string();
    } else {
        return fileName.string();
    }
}

void PrimaryUpgradePatcher::cancelPatching()
{
    m_impl->cancelled = true;
}

bool PrimaryUpgradePatcher::patchFile(MaxProgressUpdatedCallback maxProgressCb,
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

    bool ret = m_impl->patchZip(maxProgressCb, progressCb, detailsCb, userData);

    RETURN_ERROR_IF_CANCELLED

    return ret;
}

bool PrimaryUpgradePatcher::Impl::patchZip(MaxProgressUpdatedCallback maxProgressCb,
                                           ProgressUpdatedCallback progressCb,
                                           DetailsUpdatedCallback detailsCb,
                                           void *userData)
{
    progress = 0;

    if (detailsCb != nullptr) {
        detailsCb(tr("Counting number of files in zip file ..."), userData);
    }

    unsigned int count;
    auto result = FileUtils::laCountFiles(info->filename(), &count,
                                          ignoreFiles);
    if (result.errorCode() != MBP::ErrorCode::NoError) {
        error = result;
        return false;
    }

    RETURN_IF_CANCELLED

    // 6 extra files
    if (maxProgressCb != nullptr) {
        maxProgressCb(count + 6, userData);
    }
    if (progressCb != nullptr) {
        progressCb(progress, userData);
    }

    // Open the input and output zips with libarchive
    archive *aInput;
    archive *aOutput;

    aInput = archive_read_new();
    aOutput = archive_write_new();

    archive_read_support_filter_none(aInput);
    archive_read_support_format_zip(aInput);

    archive_write_set_format_zip(aOutput);
    archive_write_add_filter_none(aOutput);

    int ret = archive_read_open_filename(
            aInput, info->filename().c_str(), 10240);
    if (ret != ARCHIVE_OK) {
        Log::log(Log::Warning, "libarchive: %s", archive_error_string(aInput));
        archive_read_free(aInput);

        error = PatcherError::createArchiveError(
                MBP::ErrorCode::ArchiveReadOpenError, info->filename());
        return false;
    }

    const std::string newPath = m_parent->newFilePath();
    ret = archive_write_open_filename(aOutput, newPath.c_str());
    if (ret != ARCHIVE_OK) {
        Log::log(Log::Warning, "libarchive: %s", archive_error_string(aOutput));
        archive_read_free(aInput);
        archive_write_free(aOutput);

        error = PatcherError::createArchiveError(
                MBP::ErrorCode::ArchiveWriteOpenError, newPath);
        return false;
    }

    archive_entry *entry;

    while (archive_read_next_header(aInput, &entry) == ARCHIVE_OK) {
        RETURN_IF_CANCELLED_AND_FREE_READ_WRITE(aInput, aOutput)

        const std::string curFile = archive_entry_pathname(entry);

        if (std::find(ignoreFiles.begin(), ignoreFiles.end(),
                curFile) != ignoreFiles.end()) {
            archive_read_data_skip(aInput);
            continue;
        }

        if (progressCb != nullptr) {
            progressCb(++progress, userData);
        }
        if (detailsCb != nullptr) {
            detailsCb(curFile, userData);
        }

        if (curFile == UpdaterScript) {
            std::vector<unsigned char> contents;
            if (!FileUtils::laReadToByteArray(aInput, entry, &contents)) {
                Log::log(Log::Warning, "libarchive: %s", archive_error_string(aInput));
                archive_read_free(aInput);
                archive_write_free(aOutput);

                error = PatcherError::createArchiveError(
                        MBP::ErrorCode::ArchiveReadDataError, curFile);
                return false;
            }

            patchUpdaterScript(&contents);
            archive_entry_set_size(entry, contents.size());

            if (archive_write_header(aOutput, entry) != ARCHIVE_OK) {
                Log::log(Log::Warning, "libarchive: %s", archive_error_string(aOutput));
                archive_read_free(aInput);
                archive_write_free(aOutput);

                error = PatcherError::createArchiveError(
                        MBP::ErrorCode::ArchiveWriteHeaderError, curFile);
                return false;
            }

            archive_write_data(aOutput, contents.data(), contents.size());
        } else {
            if (archive_write_header(aOutput, entry) != ARCHIVE_OK) {
                Log::log(Log::Warning, "libarchive: %s", archive_error_string(aOutput));
                archive_read_free(aInput);
                archive_write_free(aOutput);

                error = PatcherError::createArchiveError(
                        MBP::ErrorCode::ArchiveWriteHeaderError, curFile);
                return false;
            }

            if (!FileUtils::laCopyData(aInput, aOutput)) {
                Log::log(Log::Warning, "libarchive: %s", archive_error_string(aInput));
                archive_read_free(aInput);
                archive_write_free(aOutput);

                error = PatcherError::createArchiveError(
                        MBP::ErrorCode::ArchiveReadDataError, curFile);
                return false;
            }
        }
    }

    // Add dualboot.sh
    const std::string filename = pc->scriptsDirectory() + "/" + DualBootTool;
    std::vector<unsigned char> contents;
    auto pe = FileUtils::readToMemory(filename, &contents);
    if (pe.errorCode() != MBP::ErrorCode::NoError) {
        archive_read_free(aInput);
        archive_write_free(aOutput);

        error = pe;
        return false;
    }

    info->partConfig()->replaceShellLine(&contents);

    pe = FileUtils::laAddFile(aOutput, DualBootTool, contents);
    if (pe.errorCode() != MBP::ErrorCode::NoError) {
        archive_read_free(aInput);
        archive_write_free(aOutput);

        error = pe;
        return false;
    }

    if (progressCb != nullptr) {
        progressCb(++progress, userData);
    }
    if (detailsCb != nullptr) {
        detailsCb(DualBootTool, userData);
    }

    pe = FileUtils::laAddFile(aOutput, PermTool,
                              pc->scriptsDirectory() + "/" + PermTool);
    if (pe.errorCode() != MBP::ErrorCode::NoError) {
        archive_read_free(aInput);
        archive_write_free(aOutput);

        error = pe;
        return false;
    }

    if (progressCb != nullptr) {
        progressCb(++progress, userData);
    }
    if (detailsCb != nullptr) {
        detailsCb(PermTool, userData);
    }

    auto arch = (boost::format(AndroidBins)
            % info->device()->architecture()).str();

    pe = FileUtils::laAddFile(aOutput, SetFacl,
            pc->binariesDirectory() + "/" + arch + "/" + SetFacl);
    if (pe.errorCode() != MBP::ErrorCode::NoError) {
        archive_read_free(aInput);
        archive_write_free(aOutput);

        error = pe;
        return false;
    }

    if (progressCb != nullptr) {
        progressCb(++progress, userData);
    }
    if (detailsCb != nullptr) {
        detailsCb(SetFacl, userData);
    }

    pe = FileUtils::laAddFile(aOutput, GetFacl,
            pc->binariesDirectory() + "/" + arch + "/" + GetFacl);
    if (pe.errorCode() != MBP::ErrorCode::NoError) {
        archive_read_free(aInput);
        archive_write_free(aOutput);

        error = pe;
        return false;
    }

    if (progressCb != nullptr) {
        progressCb(++progress, userData);
    }
    if (detailsCb != nullptr) {
        detailsCb(GetFacl, userData);
    }

    pe = FileUtils::laAddFile(aOutput, SetFattr,
            pc->binariesDirectory() + "/" + arch + "/" + SetFattr);
    if (pe.errorCode() != MBP::ErrorCode::NoError) {
        archive_read_free(aInput);
        archive_write_free(aOutput);

        error = pe;
        return false;
    }

    if (progressCb != nullptr) {
        progressCb(++progress, userData);
    }
    if (detailsCb != nullptr) {
        detailsCb(SetFattr, userData);
    }

    pe = FileUtils::laAddFile(aOutput, GetFattr,
            pc->binariesDirectory() + "/" + arch + "/" + GetFattr);
    if (pe.errorCode() != MBP::ErrorCode::NoError) {
        archive_read_free(aInput);
        archive_write_free(aOutput);

        error = pe;
        return false;
    }

    if (progressCb != nullptr) {
        progressCb(++progress, userData);
    }
    if (detailsCb != nullptr) {
        detailsCb(GetFattr, userData);
    }

    archive_read_free(aInput);
    archive_write_free(aOutput);

    RETURN_IF_CANCELLED

    return true;
}

bool PrimaryUpgradePatcher::Impl::patchUpdaterScript(std::vector<unsigned char> *contents)
{
    std::string strContents(contents->begin(), contents->end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    auto it = lines.begin();
    it = lines.insert(it, (boost::format(Extract) % PermTool % (TempDir + PermTool)).str()); ++it;
    it = lines.insert(it, (boost::format(Extract) % DualBootTool % (TempDir + DualBootTool)).str()); ++it;
    it = lines.insert(it, (boost::format(Extract) % SetFacl % (TempDir + SetFacl)).str()); ++it;
    it = lines.insert(it, (boost::format(Extract) % GetFacl % (TempDir + GetFacl)).str()); ++it;
    it = lines.insert(it, (boost::format(Extract) % SetFattr % (TempDir + SetFattr)).str()); ++it;
    it = lines.insert(it, (boost::format(Extract) % GetFattr % (TempDir + GetFattr)).str()); ++it;
    it = lines.insert(it, (boost::format(MakeExecutable) % (TempDir + PermTool)).str()); ++it;
    it = lines.insert(it, (boost::format(MakeExecutable) % (TempDir + DualBootTool)).str()); ++it;
    it = lines.insert(it, (boost::format(MakeExecutable) % (TempDir + SetFacl)).str()); ++it;
    it = lines.insert(it, (boost::format(MakeExecutable) % (TempDir + GetFacl)).str()); ++it;
    it = lines.insert(it, (boost::format(MakeExecutable) % (TempDir + SetFattr)).str()); ++it;
    it = lines.insert(it, (boost::format(MakeExecutable) % (TempDir + GetFattr)).str()); ++it;
    it = lines.insert(it, (boost::format(Mount) % System).str()); ++it;
    it = lines.insert(it, (boost::format(PermsBackup) % PermTool % System).str()); ++it;
    it = lines.insert(it, (boost::format(Unmount) % System).str()); ++it;
    it = lines.insert(it, (boost::format(Mount) % Cache).str()); ++it;
    it = lines.insert(it, (boost::format(PermsBackup) % PermTool % Cache).str()); ++it;
    it = lines.insert(it, (boost::format(Unmount) % Cache).str()); ++it;

    const std::string pSystem = info->device()->partition("system");
    const std::string pCache = info->device()->partition("cache");
    bool replacedFormatSystem = false;
    bool replacedFormatCache = false;

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (boost::regex_search(*it, boost::regex("^\\s*format\\s*\\(.*$"))) {
            if (it->find("system") != std::string::npos
                    || (!pSystem.empty() && it->find(pSystem) != std::string::npos)) {
                replacedFormatSystem = true;
                it = lines.erase(it);
                it = insertFormatSystem(it, &lines, true);
            } else if (it->find("cache") != std::string::npos
                    || (!pCache.empty() && it->find(pCache) != std::string::npos)) {
                replacedFormatCache = true;
                it = lines.erase(it);
                it = insertFormatCache(it, &lines, true);
            } else {
                ++it;
            }
        } else if (boost::regex_search(*it, boost::regex(
                "delete_recursive\\s*\\([^\\)]*\"/system\""))) {
            replacedFormatSystem = true;
            it = lines.erase(it);
            it = insertFormatSystem(it, &lines, false);
        } else if (boost::regex_search(*it, boost::regex(
                "delete_recursive\\s*\\([^\\)]*\"/cache\""))) {
            replacedFormatCache = true;
            it = lines.erase(it);
            it = insertFormatCache(it, &lines, false);
        } else {
            ++it;
        }
    }

    if (replacedFormatSystem && replacedFormatCache) {
        error = PatcherError::createPatchingError(
                MBP::ErrorCode::SystemCacheFormatLinesNotFound);
        return false;
    }

    lines.push_back((boost::format(Mount) % System).str());
    lines.push_back((boost::format(PermsRestore) % PermTool % System).str());
    lines.push_back((boost::format(Unmount) % System).str());
    lines.push_back((boost::format(Mount) % Cache).str());
    lines.push_back((boost::format(PermsRestore) % PermTool % Cache).str());
    lines.push_back((boost::format(Unmount) % Cache).str());
    lines.push_back(SetKernel);

    strContents = boost::join(lines, "\n");
    contents->assign(strContents.begin(), strContents.end());

    return true;
}

std::vector<std::string>::iterator
PrimaryUpgradePatcher::Impl::insertFormatSystem(std::vector<std::string>::iterator position,
                                                std::vector<std::string> *lines,
                                                bool mount) const
{
    if (mount) {
        position = lines->insert(
                position, (boost::format(Mount) % System).str());
        ++position;
    }

    position = lines->insert(
            position, (boost::format(Format) % System).str());
    ++position;

    if (mount) {
        position = lines->insert(
                position, (boost::format(Unmount) % System).str());
        ++position;
    }

    return position;
}

std::vector<std::string>::iterator
PrimaryUpgradePatcher::Impl::insertFormatCache(std::vector<std::string>::iterator position,
                                               std::vector<std::string> *lines,
                                               bool mount) const
{
    if (mount) {
        position = lines->insert(
                position, (boost::format(Mount) % Cache).str());
        ++position;
    }

    position = lines->insert(
            position, (boost::format(Format) % Cache).str());
    ++position;

    if (mount) {
        position = lines->insert(
                position, (boost::format(Unmount) % Cache).str());
        ++position;
    }

    return position;
}
