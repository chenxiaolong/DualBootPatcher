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

#include "ramdiskpatchers/hlteramdiskpatcher.h"

#include "patcherconfig.h"
#include "ramdiskpatchers/coreramdiskpatcher.h"
#include "ramdiskpatchers/qcomramdiskpatcher.h"


namespace mbp
{

/*! \cond INTERNAL */
class HlteBaseRamdiskPatcher::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};
/*! \endcond */


/*!
    \class HlteRamdiskPatcher
    \brief Handles common ramdisk patching operations for the Samsung Galaxy
           Note 3

    This patcher handles the patching of ramdisks for the Samsung Galaxy Note 3.
    Starting from version 9.0.0, every Android ramdisk is supported.
 */


HlteBaseRamdiskPatcher::HlteBaseRamdiskPatcher(const PatcherConfig * const pc,
                                               const FileInfo * const info,
                                               CpioFile * const cpio)
    : m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

HlteBaseRamdiskPatcher::~HlteBaseRamdiskPatcher()
{
}

PatcherError HlteBaseRamdiskPatcher::error() const
{
    return m_impl->error;
}

////////////////////////////////////////////////////////////////////////////////

const std::string HlteDefaultRamdiskPatcher::Id = "hlte/default";

HlteDefaultRamdiskPatcher::HlteDefaultRamdiskPatcher(const PatcherConfig * const pc,
                                                     const FileInfo *const info,
                                                     CpioFile *const cpio)
    : HlteBaseRamdiskPatcher(pc, info, cpio)
{
}

std::string HlteDefaultRamdiskPatcher::id() const
{
    return Id;
}

bool HlteDefaultRamdiskPatcher::patchRamdisk()
{
    CoreRamdiskPatcher corePatcher(m_impl->pc, m_impl->info, m_impl->cpio);
    QcomRamdiskPatcher qcomPatcher(m_impl->pc, m_impl->info, m_impl->cpio);

    if (!corePatcher.patchRamdisk()) {
        m_impl->error = corePatcher.error();
        return false;
    }

    if (!qcomPatcher.addMissingCacheInFstab(std::vector<std::string>())) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!qcomPatcher.stripManualCacheMounts("init.target.rc")) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!qcomPatcher.useGeneratedFstab("init.target.rc")) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    return true;
}

}