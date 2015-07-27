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

#include <regex>
#include <unordered_map>

#include "libmbpio/path.h"

#include "patcherconfig.h"
#include "private/stringutils.h"


namespace mbp
{

/*! \cond INTERNAL */
class CoreRP::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    CpioFile *cpio;

    PatcherError error;

    std::vector<std::string> fstabs;
};
/*! \endcond */


static const std::string MbtoolDaemonService =
        "\nservice mbtooldaemon /mbtool daemon\n"
        "    class main\n"
        "    user root\n"
        "    oneshot\n";

static const std::string MbtoolAppsyncService =
        "\nservice appsync /mbtool appsync\n"
        "    class main\n"
        "    socket installd stream 600 system system\n";


static const std::string ImportMultiBootRc = "import /init.multiboot.rc";

static const std::string InitRc = "init.rc";
static const std::string InitMultiBootRc = "init.multiboot.rc";

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

PatcherError CoreRP::error() const
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
    if (!addMultiBootRc()) {
        return false;
    }
    if (!addDaemonService()) {
        return false;
    }
    if (!addAppsyncService()) {
        return false;
    }
    if (!disableInstalldService()) {
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

bool CoreRP::addMultiBootRc()
{
    if (!m_impl->cpio->addFile(
            std::vector<unsigned char>(), InitMultiBootRc, 0750)) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    std::vector<unsigned char> contents;
    if (!m_impl->cpio->contents(InitRc, &contents)) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    std::vector<std::string> lines = StringUtils::splitData(contents, '\n');

    bool added = false;

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (!it->empty() && it->at(0) == '#') {
            continue;
        }
        lines.insert(it, ImportMultiBootRc);
        added = true;
        break;
    }

    if (!added) {
        lines.push_back(ImportMultiBootRc);
    }

    contents = StringUtils::joinData(lines, '\n');
    m_impl->cpio->setContents(InitRc, std::move(contents));

    return true;
}

bool CoreRP::addDaemonService()
{
    std::vector<unsigned char> contents;
    if (!m_impl->cpio->contents(InitMultiBootRc, &contents)) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    const std::string serviceName("mbtooldaemon");
    if (std::search(contents.begin(), contents.end(),
                    serviceName.begin(), serviceName.end()) == contents.end()) {
        contents.insert(contents.end(),
                        MbtoolDaemonService.begin(),
                        MbtoolDaemonService.end());

        m_impl->cpio->setContents(InitMultiBootRc, std::move(contents));
    }

    return true;
}

bool CoreRP::addAppsyncService()
{
    std::vector<unsigned char> contents;
    if (!m_impl->cpio->contents(InitMultiBootRc, &contents)) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    const std::string serviceName("appsync");
    if (std::search(contents.begin(), contents.end(),
                    serviceName.begin(), serviceName.end()) == contents.end()) {
        contents.insert(contents.end(),
                        MbtoolAppsyncService.begin(),
                        MbtoolAppsyncService.end());

        m_impl->cpio->setContents(InitMultiBootRc, std::move(contents));
    }

    return true;
}

bool CoreRP::disableInstalldService()
{
    std::vector<unsigned char> contents;
    if (!m_impl->cpio->contents(InitRc, &contents)) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    std::vector<std::string> lines = StringUtils::splitData(contents, '\n');

    std::regex whitespace("^\\s*$");
    bool insideService = false;
    bool isDisabled = false;
    std::vector<std::string>::iterator installdIter = lines.end();

    // Remove old mbtooldaemon service definition
    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (StringUtils::starts_with(*it, "service")) {
            insideService = it->find("installd") != std::string::npos;
            if (insideService) {
                installdIter = it;
            }
        } else if (insideService && std::regex_search(*it, whitespace)) {
            insideService = false;
        }

        if (insideService && it->find("disabled") != std::string::npos) {
            isDisabled = true;
        }
    }

    if (!isDisabled && installdIter != lines.end()) {
        lines.insert(++installdIter, "    disabled");
    }

    contents = StringUtils::joinData(lines, '\n');
    m_impl->cpio->setContents(InitRc, std::move(contents));

    return true;
}

}
