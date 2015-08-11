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

#include "ramdiskpatchers/core.h"

#include "patcherconfig.h"


namespace mbp
{

/*! \cond INTERNAL */
class CoreRP::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    ErrorCode error;

    std::vector<std::string> fstabs;
};
/*! \endcond */


CoreRP::CoreRP(const PatcherConfig * const pc,
               const FileInfo * const info,
               CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

CoreRP::~CoreRP()
{
}

ErrorCode CoreRP::error() const
{
    return m_impl->error;
}

std::string CoreRP::id() const
{
    return std::string();
}

bool CoreRP::patchRamdisk()
{
    if (!addMbtool()) {
        return false;
    }
    if (!setUpInitWrapper()) {
        return false;
    }
    return true;
}

bool CoreRP::addMbtool()
{
    const std::string mbtool("mbtool");
    std::string mbtoolPath(m_impl->pc->dataDirectory());
    mbtoolPath += "/binaries/android/";
    mbtoolPath += m_impl->info->device()->architecture();
    mbtoolPath += "/mbtool";

    if (m_impl->cpio->exists(mbtool)) {
        m_impl->cpio->remove(mbtool);
    }

    if (!m_impl->cpio->addFile(mbtoolPath, mbtool, 0750)) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    return true;
}

bool CoreRP::setUpInitWrapper()
{
    if (m_impl->cpio->exists("init.orig")) {
        // NOTE: If this assumption ever becomes an issue, we'll check the
        //       /init -> /mbtool symlink
        return true;
    }

    if (!m_impl->cpio->rename("init", "init.orig")) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    if (!m_impl->cpio->addSymlink("mbtool", "init")) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    return true;
}

}
