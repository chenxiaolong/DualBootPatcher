/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "ramdiskpatchers/jflteramdiskpatcher.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

#include "patcherconfig.h"
#include "private/regex.h"
#include "ramdiskpatchers/coreramdiskpatcher.h"
#include "ramdiskpatchers/galaxyramdiskpatcher.h"
#include "ramdiskpatchers/qcomramdiskpatcher.h"


/*! \cond INTERNAL */
class JflteBaseRamdiskPatcher::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};
/*! \endcond */


static const std::string InitRc = "init.rc";


/*!
    \class JflteRamdiskPatcher
    \brief Handles common ramdisk patching operations for the Samsung Galaxy S 4

    This patcher handles the patching of ramdisks for the Samsung Galaxy S 4.
    Starting from version 9.0.0, every Android ramdisk is supported.
 */


JflteBaseRamdiskPatcher::JflteBaseRamdiskPatcher(const PatcherConfig * const pc,
                                                 const FileInfo * const info,
                                                 CpioFile * const cpio)
    : m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

JflteBaseRamdiskPatcher::~JflteBaseRamdiskPatcher()
{
}

PatcherError JflteBaseRamdiskPatcher::error() const
{
    return m_impl->error;
}

////////////////////////////////////////////////////////////////////////////////

const std::string JflteDefaultRamdiskPatcher::Id = "jflte/default";

JflteDefaultRamdiskPatcher::JflteDefaultRamdiskPatcher(const PatcherConfig * const pc,
                                                       const FileInfo *const info,
                                                       CpioFile *const cpio)
    : JflteBaseRamdiskPatcher(pc, info, cpio)
{
}

std::string JflteDefaultRamdiskPatcher::id() const
{
    return Id;
}

bool JflteDefaultRamdiskPatcher::patchRamdisk()
{
    CoreRamdiskPatcher corePatcher(m_impl->pc, m_impl->info, m_impl->cpio);
    QcomRamdiskPatcher qcomPatcher(m_impl->pc, m_impl->info, m_impl->cpio);
    GalaxyRamdiskPatcher galaxyPatcher(m_impl->pc, m_impl->info, m_impl->cpio);

    if (!corePatcher.patchRamdisk()) {
        m_impl->error = corePatcher.error();
        return false;
    }

    if (!geChargerModeMount()) {
        return false;
    }

    if (!qcomPatcher.addMissingCacheInFstab(std::vector<std::string>())) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!qcomPatcher.stripManualCacheMounts("init.target.rc")) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!qcomPatcher.useGeneratedFstab("init.target.rc")) {
        m_impl->error = qcomPatcher.error();
        return false;
    }

    if (!galaxyPatcher.getwModifyMsm8960LpmRc()) {
        m_impl->error = galaxyPatcher.error();
        return false;
    }

    return true;
}

bool JflteDefaultRamdiskPatcher::geChargerModeMount()
{
    std::vector<unsigned char> contents;
    if (!m_impl->cpio->contents(InitRc, &contents)) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    std::string previousLine;

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (MBP_regex_search(*it, MBP_regex("mount.*/system"))
                && MBP_regex_search(previousLine, MBP_regex("on\\s+charger"))) {
            it = lines.insert(it, "    start mbtool-charger");
            ++it;
            *it = "    wait /.fstab.jgedlte.completed 15";
        }

        previousLine = *it;
    }

    lines.push_back("service mbtool-charger /mbtool mount_fstab /fstab.jgedlte");
    lines.push_back("    class core");
    lines.push_back("    critical");
    lines.push_back("    oneshot");

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(InitRc, std::move(contents));

    return true;
}
