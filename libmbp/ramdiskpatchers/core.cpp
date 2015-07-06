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

static const char *MbtoolMountServiceFmt =
        "\nservice %s /mbtool mount_fstab %s\n"
        "    class core\n"
        "    critical\n"
        "    oneshot\n"
        "    disabled\n";

static const std::string MbtoolAppsyncService =
        "\nservice appsync /mbtool appsync\n"
        "    class main\n"
        "    socket installd stream 600 system system\n";


static const std::string DataMediaContext =
        "/data/media(/.*)? u:object_r:media_rw_data_file:s0";

static const std::string ImportMultiBootRc = "import /init.multiboot.rc";

static const std::string InitRc = "init.rc";
static const std::string InitMultiBootRc = "init.multiboot.rc";
static const std::string FileContexts = "file_contexts";

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
    if (!fixDataMediaContext()) {
        return false;
    }
    if (!removeRestorecon()) {
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

/*!
 * Some ROMs omit the line in /file_contexts that sets the context of
 * /data/media/* to u:object_r:media_rw_data_file:s0. This is fine if SELinux
 * is set to permissive mode or if the SELinux policy has no restriction on
 * the u:object_r:device:s0 context (inherited from /data), but after restorecon
 * is run, the incorrect context may affect ROMs that have a stricter policy.
 */
bool CoreRP::fixDataMediaContext()
{
    if (!m_impl->cpio->exists(FileContexts)) {
        return true;
    }

    bool hasDataMediaContext = false;

    std::vector<unsigned char> contents;
    m_impl->cpio->contents(FileContexts, &contents);

    std::vector<std::string> lines = StringUtils::splitData(contents, '\n');

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (StringUtils::starts_with(*it, "/data/media")) {
            hasDataMediaContext = true;
        }
    }

    if (!hasDataMediaContext) {
        lines.push_back(DataMediaContext);
    }

    contents = StringUtils::joinData(lines, '\n');
    m_impl->cpio->setContents(FileContexts, std::move(contents));

    return true;
}

bool CoreRP::removeRestorecon()
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

    std::vector<std::string> lines = StringUtils::splitData(contents, '\n');

    const std::regex re1("^\\s*restorecon_recursive\\s+/data\\s*$");
    const std::regex re2("^\\s*restorecon_recursive\\s+/cache\\s*$");

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (std::regex_search(*it, re1) || std::regex_search(*it, re2)) {
            it->insert(it->begin(), '#');
        }
    }

    contents = StringUtils::joinData(lines, '\n');
    m_impl->cpio->setContents(InitRc, std::move(contents));

    return true;
}

static std::string whitespace(const std::string &str) {
    auto nonSpace = std::find_if(str.begin(), str.end(),
                                 std::not1(std::ptr_fun<int, int>(isspace)));
    int count = std::distance(str.begin(), nonSpace);

    return str.substr(0, count);
}

bool CoreRP::useGeneratedFstab(const std::string &filename)
{
    std::vector<unsigned char> contents;
    if (!m_impl->cpio->contents(filename, &contents)) {
        m_impl->error = m_impl->cpio->error();
        return false;
    }

    static auto const reMountAll =
            std::regex("^\\s+mount_all\\s+([^\\s]+)\\s*(#.*)?$");

    std::vector<std::string> lines = StringUtils::splitData(contents, '\n');

    std::vector<std::string> newFstabs;

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        std::smatch what;

        if (std::regex_search(*it, what, reMountAll)) {
            // Use fstab generated by mbtool
            std::string spaces = whitespace(*it);

            std::string fstab = what[1];
            std::string dir_name = io::dirName(fstab);
            std::string base_name = io::baseName(fstab);
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

            std::string serviceName =
                    StringUtils::format("mbtool-mount-%03d", index);

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

    contents = StringUtils::joinData(lines, '\n');
    m_impl->cpio->setContents(filename, std::move(contents));

    // Add mount services for the new fstab files to init.multiboot.rc
    if (!newFstabs.empty()) {
        std::vector<unsigned char> multiBootRc;

        if (!m_impl->cpio->contents("init.multiboot.rc", &multiBootRc)) {
            m_impl->error = m_impl->cpio->error();
            return false;
        }

        for (const std::string fstab : newFstabs) {
            std::string serviceName = StringUtils::format(
                    "mbtool-mount-%03" PRIzu, m_impl->fstabs.size());
            std::string service = StringUtils::format(
                    MbtoolMountServiceFmt, serviceName.c_str(), fstab.c_str());
            multiBootRc.insert(multiBootRc.end(), service.begin(), service.end());
            m_impl->fstabs.push_back(std::move(fstab));
        }

        m_impl->cpio->setContents("init.multiboot.rc", std::move(multiBootRc));
    }

    return true;
}

bool CoreRP::useGeneratedFstabAuto()
{
    for (const std::string &file : m_impl->cpio->filenames()) {
        if (file.find('/') == std::string::npos
                && StringUtils::starts_with(file, "init.")
                && StringUtils::ends_with(file, ".rc")) {
            if (!useGeneratedFstab(file)) {
                return false;
            }
        }
    }

    return true;
}

bool CoreRP::fixChargerMount(const std::string &filename)
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
    std::string dirName = io::dirName(m_impl->fstabs[0]);
    std::string baseName = io::baseName(m_impl->fstabs[0]);
    std::string completed = dirName + "/." + baseName + ".completed";

    std::string previousLine;

    std::vector<std::string> lines = StringUtils::splitData(contents, '\n');

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

    contents = StringUtils::joinData(lines, '\n');
    m_impl->cpio->setContents(filename, std::move(contents));

    return true;
}

bool CoreRP::fixChargerMountAuto()
{
    for (const std::string &file : m_impl->cpio->filenames()) {
        if (file.find('/') == std::string::npos
                && StringUtils::starts_with(file, "init.")
                && StringUtils::ends_with(file, ".rc")) {
            if (!fixChargerMount(file)) {
                return false;
            }
        }
    }

    return true;
}

}
