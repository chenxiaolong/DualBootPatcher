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

#include "ramdiskpatchers/coreramdiskpatcher.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>

#include "patcherconfig.h"


/*! \cond INTERNAL */
class CoreRamdiskPatcher::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;
};
/*! \endcond */


static const std::string MbtoolDaemonService
        = "\nservice mbtooldaemon /mbtool daemon\n"
        "    class main\n"
        "    user root\n";

static const std::string DataMediaContext =
        "/data/media(/.*)? u:object_r:media_rw_data_file:s0";

static const std::string InitRc = "init.rc";
static const std::string FileContexts = "file_contexts";

CoreRamdiskPatcher::CoreRamdiskPatcher(const PatcherConfig * const pc,
                                       const FileInfo * const info,
                                       CpioFile * const cpio) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->cpio = cpio;
}

CoreRamdiskPatcher::~CoreRamdiskPatcher()
{
}

PatcherError CoreRamdiskPatcher::error() const
{
    return m_impl->error;
}

std::string CoreRamdiskPatcher::id() const
{
    return std::string();
}

bool CoreRamdiskPatcher::patchRamdisk()
{
    if (!addDaemonService()) {
        return false;
    }
    if (!fixDataMediaContext()) {
        return false;
    }
    return true;
}

bool CoreRamdiskPatcher::addDaemonService()
{
    std::vector<unsigned char> initRc;
    if (!m_impl->cpio->contents(InitRc, &initRc)) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    initRc.insert(initRc.end(),
                  MbtoolDaemonService.begin(), MbtoolDaemonService.end());

    m_impl->cpio->setContents(InitRc, std::move(initRc));

    return true;
}

/*!
 * Some ROMs omit the line in /file_contexts that sets the context of
 * /data/media/* to u:object_r:media_rw_data_file:s0. This is fine if SELinux
 * is set to permissive mode or if the SELinux policy has no restriction on
 * the u:object_r:device:s0 context (inherited from /data), but after restorecon
 * is run, the incorrect context may affect ROMs that have a stricter policy.
 */
bool CoreRamdiskPatcher::fixDataMediaContext()
{
    if (!m_impl->cpio->exists(FileContexts)) {
        return true;
    }

    bool hasDataMediaContext = false;

    std::vector<unsigned char> contents;
    m_impl->cpio->contents(FileContexts, &contents);

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (boost::starts_with(*it, "/data/media")) {
            hasDataMediaContext = true;
        }
    }

    if (!hasDataMediaContext) {
        lines.push_back(DataMediaContext);
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(FileContexts, std::move(contents));

    return true;
}
