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

#include "mbp/ramdiskpatchers/xperia.h"

#include <cstring>

#include "mbp/patcherconfig.h"
#include "mbp/private/stringutils.h"
#include "mbp/ramdiskpatchers/core.h"


namespace mbp
{

/*! \cond INTERNAL */
class XperiaBaseRP::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    ErrorCode error;
};
/*! \endcond */


/*!
    \class XperiaRP
    \brief Handles common ramdisk patching operations for Sony Xperia devices

    This patcher handles the patching of ramdisks for Sony Xperia devices.
 */


#define REPLACE_FROM    "exec /init"
#define REPLACE_TO      "if [ -f /sbin/recovery ]; then\n" \
                        "    exec /init\n" \
                        "else\n" \
                        "    exec /mbtool init\n" \
                        "fi"

XperiaBaseRP::XperiaBaseRP(const PatcherConfig * const pc,
                           const FileInfo * const info,
                           CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

XperiaBaseRP::~XperiaBaseRP()
{
}

ErrorCode XperiaBaseRP::error() const
{
    return m_impl->error;
}

////////////////////////////////////////////////////////////////////////////////

const std::string XperiaDefaultRP::Id = "xperia/default";

XperiaDefaultRP::XperiaDefaultRP(const PatcherConfig * const pc,
                                 const FileInfo *const info,
                                 CpioFile *const cpio)
    : XperiaBaseRP(pc, info, cpio)
{
}

std::string XperiaDefaultRP::id() const
{
    return Id;
}

bool XperiaDefaultRP::patchRamdisk()
{
    CoreRP corePatcher(m_impl->pc, m_impl->info, m_impl->cpio);

    if (!corePatcher.patchRamdisk()) {
        m_impl->error = corePatcher.error();
        return false;
    }

    // If there ramdisk uses CM's wrapper init script, then patch it to run
    // mbtool when booting into Android (but not recovery)

    if (m_impl->cpio->isSymlink("init")) {
        std::string symlinkPath;
        if (!m_impl->cpio->symlinkPath("init", &symlinkPath)) {
            m_impl->error = m_impl->cpio->error();
            return false;
        }

        if (symlinkPath == "sbin/init.sh") {
            std::vector<unsigned char> data;

            if (!m_impl->cpio->contents("sbin/init.sh", &data)) {
                m_impl->error = m_impl->cpio->error();
                return false;
            }

            std::string str(data.begin(), data.end());
            data.clear();

            StringUtils::replace_all(&str, REPLACE_FROM, REPLACE_TO);

            if (!m_impl->cpio->setContentsC(
                    "sbin/init.sh",
                    reinterpret_cast<const unsigned char *>(str.data()),
                    str.size())) {
                m_impl->error = m_impl->cpio->error();
                return false;
            }
        }
    }

    return true;
}

}
