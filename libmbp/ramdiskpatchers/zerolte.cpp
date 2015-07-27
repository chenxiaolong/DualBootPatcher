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

#include "ramdiskpatchers/zerolte.h"

#include "patcherconfig.h"
#include "ramdiskpatchers/core.h"
#include "ramdiskpatchers/qcom.h"


namespace mbp
{

/*! \cond INTERNAL */
class ZerolteBaseRP::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};
/*! \endcond */


/*!
    \class ZerolteRP
    \brief Handles common ramdisk patching operations for the Samsung Galaxy S 6 Reg./Edge

    This patcher handles the patching of ramdisks for the Samsung Galaxy S 6 Reg./Edge.
    Starting from version 9.0.0, every Android ramdisk is supported.
 */


ZerolteBaseRP::ZerolteBaseRP(const PatcherConfig * const pc,
                             const FileInfo * const info,
                             CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

ZerolteBaseRP::~ZerolteBaseRP()
{
}

PatcherError ZerolteBaseRP::error() const
{
    return m_impl->error;
}

////////////////////////////////////////////////////////////////////////////////

const std::string ZerolteDefaultRP::Id = "zerolte/default";

ZerolteDefaultRP::ZerolteDefaultRP(const PatcherConfig * const pc,
                                   const FileInfo *const info,
                                   CpioFile *const cpio)
    : ZerolteBaseRP(pc, info, cpio)
{
}

std::string ZerolteDefaultRP::id() const
{
    return Id;
}

bool ZerolteDefaultRP::patchRamdisk()
{
    CoreRP corePatcher(m_impl->pc, m_impl->info, m_impl->cpio);
    QcomRP qcomPatcher(m_impl->pc, m_impl->info, m_impl->cpio);

    if (!corePatcher.patchRamdisk()) {
        m_impl->error = corePatcher.error();
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////////////////

const std::string ZeroltesprDefaultRP::Id = "zeroltespr/default";

ZeroltesprDefaultRP::ZeroltesprDefaultRP(const PatcherConfig * const pc,
                                         const FileInfo *const info,
                                         CpioFile *const cpio)
    : ZerolteDefaultRP(pc, info, cpio)
{
}

std::string ZeroltesprDefaultRP::id() const
{
    return Id;
}

}
