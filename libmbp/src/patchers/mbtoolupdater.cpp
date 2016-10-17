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

    CpioFile mainCpio;
    CpioFile cpioInCpio;
    CpioFile *target;

    // Load the ramdisk cpio
    if (!mainCpio.load(bi.ramdiskImage())) {
        error = mainCpio.error();
        return false;
    }

    const unsigned char *data;
    std::size_t size;
    if (mainCpio.contentsC("sbin/ramdisk.cpio", &data, &size)) {
        // Mess with the Android cpio archive for ramdisks on Sony devices with
        // combined boot/recovery partitions
        if (!cpioInCpio.load(data, size)) {
            error = cpioInCpio.error();
            return false;
        }

        target = &cpioInCpio;
    } else {
        target = &mainCpio;
    }

    CoreRP crp(pc, info, target);
    if (!crp.patchRamdisk()) {
        error = crp.error();
        return false;
    }

    if (target == &cpioInCpio) {
        // Store new internal cpio archive
        std::vector<unsigned char> newContents;
        if (!cpioInCpio.createData(&newContents)) {
            error = cpioInCpio.error();
            return false;
        }
        mainCpio.setContents("sbin/ramdisk.cpio", std::move(newContents));
    }

    std::vector<unsigned char> newRamdisk;
    if (!mainCpio.createData(&newRamdisk)) {
        error = mainCpio.error();
        return false;
    }
    bi.setRamdiskImage(std::move(newRamdisk));

    if (!bi.createFile(info->outputPath())) {
        error = bi.error();
        return false;
    }

    return true;
}

}
