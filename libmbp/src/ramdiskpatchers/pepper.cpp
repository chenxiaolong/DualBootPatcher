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

#include "mbp/ramdiskpatchers/pepper.h"

#include "mbp/patcherconfig.h"
#include "mbp/ramdiskpatchers/core.h"


namespace mbp
{

/*! \cond INTERNAL */
class PepperBaseRP::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    ErrorCode error;
};
/*! \endcond */


/*!
    \class PepperRP
    \brief Handles common ramdisk patching operations for the Sony Xperia Sola

    This patcher handles the patching of ramdisks for the Sony Xperia Sola.
 */


PepperBaseRP::PepperBaseRP(const PatcherConfig * const pc,
                           const FileInfo * const info,
                           CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

PepperBaseRP::~PepperBaseRP()
{
}

ErrorCode PepperBaseRP::error() const
{
    return m_impl->error;
}

////////////////////////////////////////////////////////////////////////////////

const std::string PepperDefaultRP::Id = "pepper/default";

PepperDefaultRP::PepperDefaultRP(const PatcherConfig * const pc,
                                 const FileInfo *const info,
                                 CpioFile *const cpio)
    : PepperBaseRP(pc, info, cpio)
{
}

std::string PepperDefaultRP::id() const
{
    return Id;
}

bool PepperDefaultRP::patchRamdisk()
{
    const unsigned char *data;
    std::size_t size;
    if (!m_impl->cpio->contentsC("sbin/ramdisk.cpio", &data, &size)) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    CpioFile cpioInCpio;
    if (!cpioInCpio.load(data, size)) {
        m_impl->error = cpioInCpio.error();
        return false;
    }

    CoreRP corePatcher(m_impl->pc, m_impl->info, &cpioInCpio);

    if (!corePatcher.patchRamdisk()) {
        m_impl->error = corePatcher.error();
        return false;
    }

    std::vector<unsigned char> newContents;
    if (!cpioInCpio.createData(&newContents)) {
        m_impl->error = cpioInCpio.error();
        return false;
    }

    m_impl->cpio->setContents("sbin/ramdisk.cpio", std::move(newContents));

    return true;
}

}
