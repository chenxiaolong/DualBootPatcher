/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "patchers/mbtoolupdater.h"

#include <cassert>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include "bootimage.h"
#include "cpiofile.h"
#include "patcherconfig.h"
#include "ramdiskpatchers/coreramdiskpatcher.h"


// TODO TODO TODO
#define tr(x) (x)
// TODO TODO TODO


/*! \cond INTERNAL */
class MbtoolUpdater::Impl
{
public:
    Impl(MbtoolUpdater *parent) : m_parent(parent) {}

    PatcherConfig *pc;
    const FileInfo *info;

    PatcherError error;

    bool patchImage();
    void patchInitRc(CpioFile *cpio);

private:
    MbtoolUpdater *m_parent;
};
/*! \endcond */


const std::string MbtoolUpdater::Id("MbtoolUpdater");
const std::string MbtoolUpdater::Name = tr("mbtool updater");


MbtoolUpdater::MbtoolUpdater(PatcherConfig * const pc)
    : m_impl(new Impl(this))
{
    m_impl->pc = pc;
}

MbtoolUpdater::~MbtoolUpdater()
{
}

PatcherError MbtoolUpdater::error() const
{
    return m_impl->error;
}

std::string MbtoolUpdater::id() const
{
    return Id;
}

std::string MbtoolUpdater::name() const
{
    return Name;
}

bool MbtoolUpdater::usesPatchInfo() const
{
    return false;
}

void MbtoolUpdater::setFileInfo(const FileInfo * const info)
{
    m_impl->info = info;
}

std::string MbtoolUpdater::newFilePath()
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

void MbtoolUpdater::cancelPatching()
{
    // Ignore. This runs fast enough that canceling is not needed
}

bool MbtoolUpdater::patchFile(MaxProgressUpdatedCallback maxProgressCb,
                                 ProgressUpdatedCallback progressCb,
                                 DetailsUpdatedCallback detailsCb,
                                 void *userData)
{
    (void) maxProgressCb;
    (void) progressCb;
    (void) detailsCb;
    (void) userData;

    assert(m_impl->info != nullptr);

    bool isImg = boost::iends_with(m_impl->info->filename(), ".img");
    bool isLok = boost::iends_with(m_impl->info->filename(), ".lok");

    if (!isImg && !isLok) {
        m_impl->error = PatcherError::createSupportedFileError(
                MBP::ErrorCode::OnlyBootImageSupported, Id);
        return false;
    }

    return m_impl->patchImage();
}

bool MbtoolUpdater::Impl::patchImage()
{
    BootImage bi;
    if (!bi.load(info->filename())) {
        error = bi.error();
        return false;
    }

    // Load the ramdisk cpio
    CpioFile cpio;
    if (!cpio.load(bi.ramdiskImage())) {
        error = cpio.error();
        return false;
    }

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

    // Make sure init.rc has the mbtooldaemon service
    patchInitRc(&cpio);

    std::vector<unsigned char> newRamdisk;
    if (!cpio.createData(&newRamdisk)) {
        error = cpio.error();
        return false;
    }
    bi.setRamdiskImage(std::move(newRamdisk));

    if (!bi.createFile(m_parent->newFilePath())) {
        error = bi.error();
        return false;
    }

    return true;
}

void MbtoolUpdater::Impl::patchInitRc(CpioFile *cpio)
{
    std::vector<unsigned char> contents;
    cpio->contents("init.rc", &contents);

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    bool hasMbtoolDaemon = false;

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (boost::starts_with(*it, "service")
                && it->find("mbtooldaemon") != std::string::npos) {
            hasMbtoolDaemon = true;
            break;
        }
    }

    if (!hasMbtoolDaemon) {
        CoreRamdiskPatcher crp(pc, info, cpio);
        crp.addDaemonService();
    }
}
