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

#include "mbp/patchers/mbtoolupdater.h"

#include <cassert>

#include "mbp/bootimage.h"
#include "mbp/cpiofile.h"
#include "mbp/patcherconfig.h"
#include "mbp/ramdiskpatchers/core.h"
#include "mbp/private/stringutils.h"


namespace mbp
{

/*! \cond INTERNAL */
class MbtoolUpdater::Impl
{
public:
    PatcherConfig *pc;
    const FileInfo *info;

    ErrorCode error;

    bool patchImage();
    void patchInitRc(CpioFile *cpio);
};
/*! \endcond */


const std::string MbtoolUpdater::Id("MbtoolUpdater");


MbtoolUpdater::MbtoolUpdater(PatcherConfig * const pc)
    : m_impl(new Impl())
{
    m_impl->pc = pc;
}

MbtoolUpdater::~MbtoolUpdater()
{
}

ErrorCode MbtoolUpdater::error() const
{
    return m_impl->error;
}

std::string MbtoolUpdater::id() const
{
    return Id;
}

void MbtoolUpdater::setFileInfo(const FileInfo * const info)
{
    m_impl->info = info;
}

void MbtoolUpdater::cancelPatching()
{
    // Ignore. This runs fast enough that canceling is not needed
}

bool MbtoolUpdater::patchFile(ProgressUpdatedCallback progressCb,
                              FilesUpdatedCallback filesCb,
                              DetailsUpdatedCallback detailsCb,
                              void *userData)
{
    (void) progressCb;
    (void) filesCb;
    (void) detailsCb;
    (void) userData;

    assert(m_impl->info != nullptr);

    return m_impl->patchImage();
}

bool MbtoolUpdater::Impl::patchImage()
{
    BootImage bi;
    if (!bi.loadFile(info->inputPath())) {
        error = bi.error();
        return false;
    }

    CpioFile cpio;

    // Load the ramdisk cpio
    if (!cpio.load(bi.ramdiskImage())) {
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

    if (!bi.createFile(info->outputPath())) {
        error = bi.error();
        return false;
    }

    return true;
}

static bool allWhiteSpace(const char *str)
{
    for (; *str && isspace(*str); ++str);
    return *str == '\0';
}

void MbtoolUpdater::Impl::patchInitRc(CpioFile *cpio)
{
    std::vector<unsigned char> contents;
    cpio->contents("init.rc", &contents);

    std::vector<std::string> lines = StringUtils::splitData(contents, '\n');

    bool insideService = false;

    // Remove old mbtooldaemon service definition
    for (auto it = lines.begin(); it != lines.end();) {
        if (StringUtils::starts_with(*it, "service")) {
            insideService = it->find("mbtooldaemon") != std::string::npos;
        } else if (insideService && allWhiteSpace(it->c_str())) {
            insideService = false;
        }

        if (insideService) {
            it = lines.erase(it);
        } else {
            ++it;
        }
    }

    contents = StringUtils::joinData(lines, '\n');
    cpio->setContents("init.rc", std::move(contents));

    CoreRP crp(pc, info, cpio);
    crp.patchRamdisk();
}

}
