/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbp/ramdiskpatchers/default.h"

#include "mbp/patcherconfig.h"
#include "mbp/ramdiskpatchers/core.h"


namespace mbp
{

/*! \cond INTERNAL */
class DefaultRP::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    ErrorCode error;
};
/*! \endcond */


/*!
 * \class DefaultRP
 * \brief Handles common ramdisk patching operations for most devices
 */

const std::string DefaultRP::Id = "default";

DefaultRP::DefaultRP(const PatcherConfig * const pc,
                     const FileInfo * const info,
                     CpioFile * const cpio)
    : m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

DefaultRP::~DefaultRP()
{
}

ErrorCode DefaultRP::error() const
{
    return m_impl->error;
}

std::string DefaultRP::id() const
{
    return Id;
}

bool DefaultRP::patchRamdisk()
{
    CpioFile cpioInCpio;
    CpioFile *target;

    bool hasCombined = mb_device_flags(m_impl->info->device())
            & FLAG_HAS_COMBINED_BOOT_AND_RECOVERY;

    const unsigned char *data;
    std::size_t size;
    if (hasCombined
            && m_impl->cpio->contentsC("sbin/ramdisk.cpio", &data, &size)) {
        // Mess with the Android cpio archive for ramdisks on Sony devices with
        // combined boot/recovery partitions
        if (!cpioInCpio.load(data, size)) {
            m_impl->error = cpioInCpio.error();
            return false;
        }

        target = &cpioInCpio;
    } else {
        target = m_impl->cpio;
    }

    CoreRP corePatcher(m_impl->pc, m_impl->info, target);

    if (!corePatcher.patchRamdisk()) {
        m_impl->error = corePatcher.error();
        return false;
    }

    if (target == &cpioInCpio) {
        // Store new internal cpio archive
        std::vector<unsigned char> newContents;
        if (!cpioInCpio.createData(&newContents)) {
            m_impl->error = cpioInCpio.error();
            return false;
        }
        m_impl->cpio->setContents("sbin/ramdisk.cpio", std::move(newContents));
    }

    return true;
}

}
