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

#include "ramdiskpatchers/klte.h"

#include "patcherconfig.h"
#include "ramdiskpatchers/core.h"
#include "ramdiskpatchers/qcom.h"


namespace mbp
{

/*! \cond INTERNAL */
class KlteBaseRP::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};
/*! \endcond */


/*!
    \class KlteRP
    \brief Handles common ramdisk patching operations for the Samsung Galaxy S 5

    This patcher handles the patching of ramdisks for the Samsung Galaxy S 5.
    Starting from version 9.0.0, every Android ramdisk is supported.
 */


KlteBaseRP::KlteBaseRP(const PatcherConfig * const pc,
                       const FileInfo * const info,
                       CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

KlteBaseRP::~KlteBaseRP()
{
}

PatcherError KlteBaseRP::error() const
{
    return m_impl->error;
}

////////////////////////////////////////////////////////////////////////////////

const std::string KlteDefaultRP::Id = "klte/default";

KlteDefaultRP::KlteDefaultRP(const PatcherConfig * const pc,
                             const FileInfo *const info,
                             CpioFile *const cpio)
    : KlteBaseRP(pc, info, cpio)
{
}

std::string KlteDefaultRP::id() const
{
    return Id;
}

bool KlteDefaultRP::patchRamdisk()
{
    CoreRP corePatcher(m_impl->pc, m_impl->info, m_impl->cpio);
    QcomRP qcomPatcher(m_impl->pc, m_impl->info, m_impl->cpio);

    if (!corePatcher.patchRamdisk()) {
        m_impl->error = corePatcher.error();
        return false;
    }

    if (!corePatcher.fixChargerMountAuto()) {
        m_impl->error = corePatcher.error();
        return false;
    }

    if (!qcomPatcher.addMissingCacheInFstab(std::vector<std::string>())) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (m_impl->cpio->exists("init.target.rc")) {
        if (!qcomPatcher.stripManualMounts("init.target.rc")) {
            m_impl->error = qcomPatcher.error();
            return false;
        }
    } else {
        if (!qcomPatcher.stripManualMounts("init.qcom.rc")) {
            m_impl->error = qcomPatcher.error();
            return false;
        }
    }

    return true;
}

}
