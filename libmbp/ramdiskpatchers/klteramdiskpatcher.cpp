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

#include "ramdiskpatchers/klteramdiskpatcher.h"

#include "patcherconfig.h"
#include "ramdiskpatchers/coreramdiskpatcher.h"
#include "ramdiskpatchers/qcomramdiskpatcher.h"


namespace mbp
{

/*! \cond INTERNAL */
class KlteBaseRamdiskPatcher::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};
/*! \endcond */


/*!
    \class KlteRamdiskPatcher
    \brief Handles common ramdisk patching operations for the Samsung Galaxy S 5

    This patcher handles the patching of ramdisks for the Samsung Galaxy S 5.
    Starting from version 9.0.0, every Android ramdisk is supported.
 */


KlteBaseRamdiskPatcher::KlteBaseRamdiskPatcher(const PatcherConfig * const pc,
                                               const FileInfo * const info,
                                               CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

KlteBaseRamdiskPatcher::~KlteBaseRamdiskPatcher()
{
}

PatcherError KlteBaseRamdiskPatcher::error() const
{
    return m_impl->error;
}

////////////////////////////////////////////////////////////////////////////////

const std::string KlteDefaultRamdiskPatcher::Id = "klte/default";

KlteDefaultRamdiskPatcher::KlteDefaultRamdiskPatcher(const PatcherConfig * const pc,
                                                     const FileInfo *const info,
                                                     CpioFile *const cpio)
    : KlteBaseRamdiskPatcher(pc, info, cpio)
{
}

std::string KlteDefaultRamdiskPatcher::id() const
{
    return Id;
}

bool KlteDefaultRamdiskPatcher::patchRamdisk()
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

    if (m_impl->cpio->exists("init.target.rc")) {
        if (!qcomPatcher.stripManualCacheMounts("init.target.rc")) {
            m_impl->error = qcomPatcher.error();
            return false;
        }

        if (!qcomPatcher.useGeneratedFstab("init.target.rc")) {
            m_impl->error = qcomPatcher.error();
            return false;
        }
    } else {
        if (!qcomPatcher.stripManualCacheMounts("init.qcom.rc")) {
            m_impl->error = qcomPatcher.error();
            return false;
        }

        if (!qcomPatcher.useGeneratedFstab("init.qcom.rc")) {
            m_impl->error = qcomPatcher.error();
            return false;
        }
    }

    return true;
}

}