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

#include "ramdiskpatchers/hammerhead.h"

#include "ramdiskpatchers/core.h"


namespace mbp
{

/*! \cond INTERNAL */
class HammerheadBaseRP::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};
/*! \endcond */


HammerheadBaseRP::HammerheadBaseRP(const PatcherConfig * const pc,
                                   const FileInfo * const info,
                                   CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

HammerheadBaseRP::~HammerheadBaseRP()
{
}

PatcherError HammerheadBaseRP::error() const
{
    return m_impl->error;
}

////////////////////////////////////////////////////////////////////////////////

const std::string HammerheadDefaultRP::Id = "hammerhead/default";

HammerheadDefaultRP::HammerheadDefaultRP(const PatcherConfig *const pc,
                                         const FileInfo *const info,
                                         CpioFile *const cpio)
    : HammerheadBaseRP(pc, info, cpio)
{
}

std::string HammerheadDefaultRP::id() const
{
    return Id;
}

bool HammerheadDefaultRP::patchRamdisk()
{
    CoreRP corePatcher(m_impl->pc, m_impl->info, m_impl->cpio);

    if (!corePatcher.patchRamdisk()) {
        m_impl->error = corePatcher.error();
        return false;
    }

    return true;
}

}
