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

#include "ramdiskpatchers/trlte.h"

#include "patcherconfig.h"
#include "ramdiskpatchers/core.h"
#include "ramdiskpatchers/qcom.h"


namespace mbp
{

/*! \cond INTERNAL */
class TrlteBaseRP::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};
/*! \endcond */


/*!
    \class TrlteRP
    \brief Handles common ramdisk patching operations for the Samsung Galaxy Note 4

    This patcher handles the patching of ramdisks for the Samsung Galaxy Note 4.
    Starting from version 9.0.0, every Android ramdisk is supported.
 */


TrlteBaseRP::TrlteBaseRP(const PatcherConfig * const pc,
                         const FileInfo * const info,
                         CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

TrlteBaseRP::~TrlteBaseRP()
{
}

PatcherError TrlteBaseRP::error() const
{
    return m_impl->error;
}

////////////////////////////////////////////////////////////////////////////////

const std::string TrlteDefaultRP::Id = "trlte/default";

TrlteDefaultRP::TrlteDefaultRP(const PatcherConfig * const pc,
                               const FileInfo *const info,
                               CpioFile *const cpio)
    : TrlteBaseRP(pc, info, cpio)
{
}

std::string TrlteDefaultRP::id() const
{
    return Id;
}

bool TrlteDefaultRP::patchRamdisk()
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

    std::string mountFile;

    if (m_impl->cpio->exists("init.target.rc")) {
        mountFile = "init.target.rc";
    } else {
        mountFile = "init.qcom.rc";
    }

    if (!qcomPatcher.stripManualMounts(mountFile)) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!qcomPatcher.stripManualMounts("init.rc")) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    return true;
}

}
