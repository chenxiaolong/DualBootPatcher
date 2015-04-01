/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "ramdiskpatchers/lgg3ramdiskpatcher.h"

#include "ramdiskpatchers/coreramdiskpatcher.h"
#include "ramdiskpatchers/qcomramdiskpatcher.h"


namespace mbp
{

/*! \cond INTERNAL */
class LGG3RamdiskPatcher::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};
/*! \endcond */


const std::string LGG3RamdiskPatcher::Id = "lgg3/default";

LGG3RamdiskPatcher::LGG3RamdiskPatcher(const PatcherConfig * const pc,
                                       const FileInfo * const info,
                                       CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

LGG3RamdiskPatcher::~LGG3RamdiskPatcher()
{
}

PatcherError LGG3RamdiskPatcher::error() const
{
    return m_impl->error;
}

std::string LGG3RamdiskPatcher::id() const
{
    return Id;
}

bool LGG3RamdiskPatcher::patchRamdisk()
{
    CoreRamdiskPatcher corePatcher(m_impl->pc, m_impl->info, m_impl->cpio);
    QcomRamdiskPatcher qcomPatcher(m_impl->pc, m_impl->info, m_impl->cpio);

    if (!corePatcher.patchRamdisk()) {
        m_impl->error = corePatcher.error();
        return false;
    }

    if (!qcomPatcher.useGeneratedFstab("init.g3.rc")) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    return true;
}

}