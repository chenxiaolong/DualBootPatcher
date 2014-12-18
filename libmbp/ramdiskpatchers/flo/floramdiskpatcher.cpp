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

#include "ramdiskpatchers/flo/floramdiskpatcher.h"

#include "ramdiskpatchers/common/coreramdiskpatcher.h"
#include "ramdiskpatchers/qcom/qcomramdiskpatcher.h"


/*! \cond INTERNAL */
class FloBaseRamdiskPatcher::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};
/*! \endcond */


FloBaseRamdiskPatcher::FloBaseRamdiskPatcher(const PatcherConfig * const pc,
                                             const FileInfo * const info,
                                             CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

FloBaseRamdiskPatcher::~FloBaseRamdiskPatcher()
{
}

PatcherError FloBaseRamdiskPatcher::error() const
{
    return m_impl->error;
}

////////////////////////////////////////////////////////////////////////////////

const std::string FloAOSPRamdiskPatcher::Id = "flo/default";

FloAOSPRamdiskPatcher::FloAOSPRamdiskPatcher(const PatcherConfig *const pc,
                                             const FileInfo *const info,
                                             CpioFile *const cpio)
    : FloBaseRamdiskPatcher(pc, info, cpio)
{
}

std::string FloAOSPRamdiskPatcher::id() const
{
    return Id;
}

bool FloAOSPRamdiskPatcher::patchRamdisk()
{
    CoreRamdiskPatcher corePatcher(m_impl->pc, m_impl->info, m_impl->cpio);
    QcomRamdiskPatcher qcomPatcher(m_impl->pc, m_impl->info, m_impl->cpio);

    if (!corePatcher.patchRamdisk()) {
        m_impl->error = corePatcher.error();
        return false;
    }

    if (!qcomPatcher.useGeneratedFstab("init.flo.rc")) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    return true;
}
