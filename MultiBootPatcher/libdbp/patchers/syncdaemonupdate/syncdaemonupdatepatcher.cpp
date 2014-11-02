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

#include "patchers/syncdaemonupdate/syncdaemonupdatepatcher.h"

#include <fstream>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/regex.hpp>

#include "bootimage.h"
#include "patcherpaths.h"
#include "private/fileutils.h"
#include "private/logging.h"
#include "ramdiskpatchers/common/coreramdiskpatcher.h"


#define RETURN_IF_CANCELLED \
    if (cancelled) { \
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


class SyncdaemonUpdatePatcher::Impl
{
public:
    Impl(SyncdaemonUpdatePatcher *parent) : m_parent(parent) {}

    const PatcherPaths *pp;
    const FileInfo *info;

    bool cancelled;

    PatcherError error;

    bool patchImage();
    std::string findPartConfigId(const CpioFile * const cpio) const;

private:
    SyncdaemonUpdatePatcher *m_parent;
};


const std::string SyncdaemonUpdatePatcher::Id = "SyncdaemonUpdatePatcher";
const std::string SyncdaemonUpdatePatcher::Name = tr("Update syncdaemon");


SyncdaemonUpdatePatcher::SyncdaemonUpdatePatcher(const PatcherPaths * const pp)
    : m_impl(new Impl(this))
{
    m_impl->pp = pp;
}

SyncdaemonUpdatePatcher::~SyncdaemonUpdatePatcher()
{
}

std::vector<PartitionConfig *> SyncdaemonUpdatePatcher::partConfigs()
{
    return std::vector<PartitionConfig *>();
}

PatcherError SyncdaemonUpdatePatcher::error() const
{
    return m_impl->error;
}

std::string SyncdaemonUpdatePatcher::id() const
{
    return Id;
}

std::string SyncdaemonUpdatePatcher::name() const
{
    return Name;
}

bool SyncdaemonUpdatePatcher::usesPatchInfo() const
{
    return false;
}

std::vector<std::string> SyncdaemonUpdatePatcher::supportedPartConfigIds() const
{
    return std::vector<std::string>();
}

void SyncdaemonUpdatePatcher::setFileInfo(const FileInfo * const info)
{
    m_impl->info = info;
}

std::string SyncdaemonUpdatePatcher::newFilePath()
{
    if (m_impl->info == nullptr) {
        Log::log(Log::Warning, "d->info is null!");
        m_impl->error = PatcherError::createGenericError(
                PatcherError::ImplementationError);
        return std::string();
    }

    boost::filesystem::path path(m_impl->info->filename());
    boost::filesystem::path fileName = path.stem();
    fileName += "_syncdaemon";
    fileName += path.extension();

    if (path.has_parent_path()) {
        return (path.parent_path() / fileName).string();
    } else {
        return fileName.string();
    }
}

void SyncdaemonUpdatePatcher::cancelPatching()
{
    m_impl->cancelled = true;
}

bool SyncdaemonUpdatePatcher::patchFile(MaxProgressUpdatedCallback maxProgressCb,
                                        ProgressUpdatedCallback progressCb,
                                        DetailsUpdatedCallback detailsCb,
                                        void *userData)
{
    (void) maxProgressCb;
    (void) progressCb;
    (void) detailsCb;
    (void) userData;

    if (m_impl->info == nullptr) {
        Log::log(Log::Warning, "d->info is null!");
        m_impl->error = PatcherError::createGenericError(
                PatcherError::ImplementationError);
        return false;
    }

    bool isImg = boost::iends_with(m_impl->info->filename(), ".img");
    bool isLok = boost::iends_with(m_impl->info->filename(), ".lok");

    if (!isImg && !isLok) {
        m_impl->error = PatcherError::createSupportedFileError(
                PatcherError::OnlyBootImageSupported, Id);
        return false;
    }

    bool ret = m_impl->patchImage();

    RETURN_ERROR_IF_CANCELLED

    return ret;
}

bool SyncdaemonUpdatePatcher::Impl::patchImage()
{
    BootImage bi;
    if (!bi.load(info->filename())) {
        error = bi.error();
        return false;
    }

    CpioFile cpio;
    cpio.load(bi.ramdiskImage());

    RETURN_IF_CANCELLED

    std::string partConfigId = findPartConfigId(&cpio);
    if (!partConfigId.empty()) {
        PartitionConfig *config = nullptr;

        for (PartitionConfig *c : pp->partitionConfigs()) {
            if (c->id() == partConfigId) {
                config = c;
                break;
            }
        }

        const std::string mountScript("init.multiboot.mounting.sh");
        if (config != nullptr && cpio.exists(mountScript)) {
            const std::string filename(
                    pp->scriptsDirectory() + "/" + mountScript);
            std::vector<unsigned char> contents;
            auto ret = FileUtils::readToMemory(filename, &contents);
            if (ret.errorCode() != PatcherError::NoError) {
                error = ret;
                return false;
            }

            config->replaceShellLine(&contents, true);

            if (cpio.exists(mountScript)) {
                cpio.remove(mountScript);
            }

            cpio.addFile(contents, mountScript, 0750);
        }
    }

    RETURN_IF_CANCELLED

    // Add syncdaemon to init.rc if it doesn't already exist
    if (!cpio.exists("sbin/syncdaemon")) {
        CoreRamdiskPatcher crp(pp, info, &cpio);
        crp.addSyncdaemon();
    } else {
        // Make sure 'oneshot' (added by previous versions) is removed
        std::vector<unsigned char> initRc = cpio.contents("init.rc");

        std::string strContents(initRc.begin(), initRc.end());
        std::vector<std::string> lines;
        boost::split(lines, strContents, boost::is_any_of("\n"));

        bool inSyncdaemon = false;

        for (auto it = lines.begin(); it != lines.end(); ++it) {
            if (boost::starts_with(*it, "service")) {
                inSyncdaemon = it->find("/sbin/syncdaemon") != std::string::npos;
            } else if (inSyncdaemon && it->find("oneshot") != std::string::npos) {
                it = lines.erase(it);
                it--;
            }
        }

        strContents = boost::join(lines, "\n");
        initRc.assign(strContents.begin(), strContents.end());
        cpio.setContents("init.rc", std::move(initRc));
    }

    RETURN_IF_CANCELLED

    const std::string syncdaemon("sbin/syncdaemon");

    if (cpio.exists(syncdaemon)) {
        cpio.remove(syncdaemon);
    }

    cpio.addFile(pp->binariesDirectory() + "/" + "android" + "/"
            + info->device()->architecture() + "/" + "syncdaemon",
            syncdaemon, 0750);

    RETURN_IF_CANCELLED

    bi.setRamdiskImage(cpio.createData(true));

    std::ofstream file(m_parent->newFilePath(), std::ios::binary);
    if (file.fail()) {
        error = PatcherError::createIOError(
                PatcherError::FileOpenError, m_parent->newFilePath());
        return false;
    }

    auto out = bi.create();
    file.write(reinterpret_cast<char *>(out.data()), out.size());
    file.close();

    RETURN_IF_CANCELLED

    return true;
}

std::string SyncdaemonUpdatePatcher::Impl::findPartConfigId(const CpioFile * const cpio) const
{
    std::vector<unsigned char> defaultProp = cpio->contents("default.prop");
    if (!defaultProp.empty()) {
        std::string strContents(defaultProp.begin(), defaultProp.end());
        std::vector<std::string> lines;
        boost::split(lines, strContents, boost::is_any_of("\n"));

        for (auto const &line : lines) {
            if (boost::starts_with(line, "ro.patcher.patched")) {
                std::vector<std::string> split;
                boost::split(split, line, boost::is_any_of("="));

                if (split.size() > 1) {
                    return split[1];
                }
            }
        }
    }

    std::vector<unsigned char> mountScript = cpio->contents(
            "init.multiboot.mounting.sh");
    if (!mountScript.empty()) {
        std::string strContents(mountScript.begin(), mountScript.end());
        std::vector<std::string> lines;
        boost::split(lines, strContents, boost::is_any_of("\n"));

        for (auto const &line : lines) {
            if (boost::starts_with(line, "TARGET_DATA=")) {
                boost::smatch what;

                if (boost::regex_search(line, boost::regex("/raw-data/([^/\"]+)"))) {
                    return what[1];
                }
            }
        }
    }

    return std::string();
}
