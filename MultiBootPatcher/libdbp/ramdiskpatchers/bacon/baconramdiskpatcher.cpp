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

#include "ramdiskpatchers/bacon/baconramdiskpatcher.h"

#include "ramdiskpatchers/common/coreramdiskpatcher.h"
#include "ramdiskpatchers/qcom/qcomramdiskpatcher.h"


/*! \cond INTERNAL */
class BaconRamdiskPatcher::Impl
{
public:
    const PatcherPaths *pp;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};
/*! \endcond */


const std::string BaconRamdiskPatcher::Id = "bacon/AOSP/AOSP";

BaconRamdiskPatcher::BaconRamdiskPatcher(const PatcherPaths * const pp,
                                         const FileInfo * const info,
                                         CpioFile * const cpio)
    : m_impl(new Impl())
{
    m_impl->pp = pp;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

BaconRamdiskPatcher::~BaconRamdiskPatcher()
{
}

PatcherError BaconRamdiskPatcher::error() const
{
    return m_impl->error;
}

std::string BaconRamdiskPatcher::id() const
{
    return Id;
}

bool BaconRamdiskPatcher::patchRamdisk()
{
    CoreRamdiskPatcher corePatcher(m_impl->pp, m_impl->info, m_impl->cpio);
    QcomRamdiskPatcher qcomPatcher(m_impl->pp, m_impl->info, m_impl->cpio);

    if (!corePatcher.patchRamdisk()) {
        m_impl->error = corePatcher.error();
        return false;
    }

    if (!qcomPatcher.modifyInitRc()) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!qcomPatcher.modifyInitQcomRc()) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!qcomPatcher.modifyFstab()) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!qcomPatcher.modifyInitTargetRc("init.bacon.rc")) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    return true;
}
