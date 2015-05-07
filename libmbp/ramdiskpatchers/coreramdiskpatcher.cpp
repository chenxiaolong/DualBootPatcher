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

#include <regex>
#include <unordered_map>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem/path.hpp>

#include <cppformat/format.h>

#include "patcherconfig.h"


namespace mbp
{

/*! \cond INTERNAL */
class CoreRamdiskPatcher::Impl
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

static const std::string MbtoolMountServiceFmt =
        "\nservice {} /mbtool mount_fstab {}\n"
        "    class core\n"
        "    critical\n"
        "    oneshot\n"
        "    disabled\n";

static const std::string DataMediaContext =
        "/data/media(/.*)? u:object_r:media_rw_data_file:s0";

static const std::string ImportMultiBootRc = "import /init.multiboot.rc";

static const std::string InitRc = "init.rc";
static const std::string InitMultiBootRc = "init.multiboot.rc";
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
    if (!addMultiBootRc()) {
        return false;
    }
    if (!addDaemonService()) {
        return false;
    }
    if (!fixDataMediaContext()) {
        return false;
    }
    if (!removeRestoreconData()) {
        return false;
    }
    return true;
}

bool CoreRamdiskPatcher::addMultiBootRc()
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

    std::vector<std::string> lines;
    boost::split(lines, contents, boost::is_any_of("\n"));

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

    std::string strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(InitRc, std::move(contents));

    return true;
}

bool CoreRamdiskPatcher::addDaemonService()
{
    std::vector<unsigned char> multiBootRc;
    if (!m_impl->cpio->contents(InitMultiBootRc, &multiBootRc)) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    multiBootRc.insert(multiBootRc.end(),
                       MbtoolDaemonService.begin(),
                       MbtoolDaemonService.end());

    m_impl->cpio->setContents(InitMultiBootRc, std::move(multiBootRc));

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

    std::vector<std::string> lines;
    boost::split(lines, contents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (boost::starts_with(*it, "/data/media")) {
            hasDataMediaContext = true;
        }
    }

    if (!hasDataMediaContext) {
        lines.push_back(DataMediaContext);
    }

    std::string strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(FileContexts, std::move(contents));

    return true;
}

bool CoreRamdiskPatcher::removeRestoreconData()
{
    // This use to change the "/data(/.*)?" regex in file_contexts to
    // "/data(/(?!multiboot).*)?". Unfortunately, older versions of Android's
    // fork of libselinux use POSIX regex functions instead of PCRE, so
    // lookahead isn't supported. The regex will fail to compile and the system
    // will just reboot.

    if (!m_impl->cpio->exists(InitRc)) {
        return true;
    }

    std::vector<unsigned char> contents;
    m_impl->cpio->contents(InitRc, &contents);

    std::vector<std::string> lines;
    boost::split(lines, contents, boost::is_any_of("\n"));

    const std::regex re("^\\s*restorecon_recursive\\s+/data\\s*");

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (std::regex_search(*it, re)) {
            it->insert(it->begin(), '#');
        }
    }

    std::string strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(InitRc, std::move(contents));

    return true;
}

static std::string whitespace(const std::string &str) {
    auto nonSpace = std::find_if(str.begin(), str.end(),
                                 std::not1(std::ptr_fun<int, int>(isspace)));
    int count = std::distance(str.begin(), nonSpace);

    return str.substr(0, count);
}

bool CoreRamdiskPatcher::useGeneratedFstab(const std::string &filename)
{
    std::vector<unsigned char> contents;
    if (!m_impl->cpio->contents(filename, &contents)) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    static auto const reMountAll =
            std::regex("^\\s+mount_all\\s+([^\\s]+)\\s*(#.*)?$");

    std::vector<std::string> lines;
    boost::split(lines, contents, boost::is_any_of("\n"));

    std::vector<std::string> newFstabs;

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        std::smatch what;

        if (std::regex_search(*it, what, reMountAll)) {
            // Use fstab generated by mbtool
            std::string spaces = whitespace(*it);

            std::string fstab = what[1];
            boost::filesystem::path fstab_path(fstab);
            std::string dir_name = fstab_path.parent_path().string();
            std::string base_name = fstab_path.filename().string();
            std::string completed = dir_name + "/." + base_name + ".completed";
            std::string generated = dir_name + "/." + base_name + ".gen";

            if (fstab.find("goldfish") != std::string::npos) {
                // Skip over files for the Android emulator
                continue;
            }

            // Debugging this: "- exec '/system/bin/sh' failed: No such file or directory (2) -"
            // sure was fun... Turns out service names > 16 characters are rejected
            // See valid_name() in https://android.googlesource.com/platform/system/core/+/master/init/init_parser.c

            int index;

            auto fstab_it = std::find(m_impl->fstabs.begin(),
                                      m_impl->fstabs.end(), fstab);
            if (fstab_it != m_impl->fstabs.end()) {
                index = fstab_it - m_impl->fstabs.begin();
            } else {
                index = m_impl->fstabs.size() + newFstabs.size();
                newFstabs.push_back(fstab);
            }

            std::string serviceName = fmt::format("mbtool-mount-{:03d}", index);

            // Start mounting service
            it = lines.insert(it, spaces + "start " + serviceName);
            // Wait for mount to complete (this sucks, but only custom ROMs
            // implement the 'exec' init script command)
            it = lines.insert(++it, spaces + "wait " + completed + " 15");
            // Mount generated fstab
            ++it;
            *it = spaces + "mount_all " + generated;
        }
    }

    std::string strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(filename, std::move(contents));

    // Add mount services for the new fstab files to init.multiboot.rc
    if (!newFstabs.empty()) {
        std::vector<unsigned char> multiBootRc;

        if (!m_impl->cpio->contents("init.multiboot.rc", &multiBootRc)) {
            m_impl->error = m_impl->cpio->error();
            return false;
        }

        for (const std::string fstab : newFstabs) {
            std::string serviceName =
                    fmt::format("mbtool-mount-{:03d}", m_impl->fstabs.size());
            std::string service = fmt::format(
                    MbtoolMountServiceFmt, serviceName, fstab);
            multiBootRc.insert(multiBootRc.end(), service.begin(), service.end());
            m_impl->fstabs.push_back(std::move(fstab));
        }

        m_impl->cpio->setContents("init.multiboot.rc", std::move(multiBootRc));
    }

    return true;
}

bool CoreRamdiskPatcher::useGeneratedFstabAuto()
{
    for (const std::string &file : m_impl->cpio->filenames()) {
        if (file.find('/') == std::string::npos
                && boost::starts_with(file, "init.")
                && boost::ends_with(file, ".rc")) {
            if (!useGeneratedFstab(file)) {
                return false;
            }
        }
    }

    return true;
}

bool CoreRamdiskPatcher::fixChargerMount(const std::string &filename)
{
    std::vector<unsigned char> contents;
    if (!m_impl->cpio->contents(filename, &contents)) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    if (m_impl->fstabs.empty()) {
        // NOTE: This function should be called after useGeneratedFstab()
        return true;
    }

    // Paths
    boost::filesystem::path fstab_path(m_impl->fstabs[0]);
    std::string dirName = fstab_path.parent_path().string();
    std::string baseName = fstab_path.filename().string();
    std::string completed = dirName + "/." + baseName + ".completed";

    std::string previousLine;

    std::vector<std::string> lines;
    boost::split(lines, contents, boost::is_any_of("\n"));

    static auto const reMountSystem = std::regex("mount.*/system");
    static auto const reOnCharger = std::regex("on\\s+charger");

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (std::regex_search(*it, reMountSystem)
                && (std::regex_search(previousLine, reOnCharger)
                || previousLine.find("ro.bootmode=charger") != std::string::npos)) {
            std::string spaces = whitespace(*it);
            *it = spaces + "start mbtool-mount-000";
            it = lines.insert(++it, spaces + "wait " + completed + " 15");
        }

        previousLine = *it;
    }

    std::string strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    m_impl->cpio->setContents(filename, std::move(contents));

    return true;
}

bool CoreRamdiskPatcher::fixChargerMountAuto()
{
    for (const std::string &file : m_impl->cpio->filenames()) {
        if (file.find('/') == std::string::npos
                && boost::starts_with(file, "init.")
                && boost::ends_with(file, ".rc")) {
            if (!fixChargerMount(file)) {
                return false;
            }
        }
    }

    return true;
}

}
