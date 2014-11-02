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

#include "ramdiskpatchers/falcon/falconramdiskpatcher.h"

#include "ramdiskpatchers/common/coreramdiskpatcher.h"
#include "ramdiskpatchers/qcom/qcomramdiskpatcher.h"


class FalconRamdiskPatcher::Impl
{
public:
    const PatcherPaths *pp;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};


const std::string FalconRamdiskPatcher::Id = "falcon/AOSP/AOSP";

FalconRamdiskPatcher::FalconRamdiskPatcher(const PatcherPaths * const pp,
                                       const FileInfo * const info,
                                       CpioFile * const cpio)
    : m_impl(new Impl())
{
    m_impl->pp = pp;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

FalconRamdiskPatcher::~FalconRamdiskPatcher()
{
}

PatcherError FalconRamdiskPatcher::error() const
{
    return m_impl->error;
}

std::string FalconRamdiskPatcher::id() const
{
    return Id;
}

bool FalconRamdiskPatcher::patchRamdisk()
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

    std::vector<std::string> fstabs;
    fstabs.push_back("gpe-fstab.qcom");

    QcomRamdiskPatcher::FstabArgs args;
    args[QcomRamdiskPatcher::ArgAdditionalFstabs] = std::move(fstabs);

    if (!qcomPatcher.modifyFstab(args)) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!qcomPatcher.modifyInitTargetRc()) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    return true;
}
