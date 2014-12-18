/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#include "autopatchers/standard/standardpatcher.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/format.hpp>

#include "private/fileutils.h"
#include "private/regex.h"


/*! \cond INTERNAL */
class StandardPatcher::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
};
/*! \endcond */


const std::string StandardPatcher::Id
        = "StandardPatcher";

const std::string StandardPatcher::UpdaterScript
        = "META-INF/com/google/android/updater-script";
static const std::string Mount
        = "run_program(\"/tmp/dualboot.sh\", \"mount-%1%\");";
static const std::string Unmount
        = "run_program(\"/tmp/dualboot.sh\", \"unmount-%1%\");";
static const std::string Format
        = "run_program(\"/tmp/dualboot.sh\", \"format-%1%\");";

static const std::string System = "system";
static const std::string Cache = "cache";
static const std::string Data = "data";


StandardPatcher::StandardPatcher(const PatcherConfig * const pc,
                                 const FileInfo * const info,
                                 const PatchInfo::AutoPatcherArgs &args) :
    m_impl(new Impl())
{
    (void) args;

    m_impl->pc = pc;
    m_impl->info = info;
}

StandardPatcher::~StandardPatcher()
{
}

PatcherError StandardPatcher::error() const
{
    return PatcherError();
}

std::string StandardPatcher::id() const
{
    return Id;
}

std::vector<std::string> StandardPatcher::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> StandardPatcher::existingFiles() const
{
    std::vector<std::string> files;
    files.push_back(UpdaterScript);
    return files;
}

bool StandardPatcher::patchFiles(const std::string &directory,
                                 const std::vector<std::string> &bootImages)
{
    std::vector<unsigned char> contents;

    // UpdaterScript begin
    FileUtils::readToMemory(directory + "/" + UpdaterScript, &contents);
    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    insertDualBootSh(&lines);
    replaceMountLines(&lines, m_impl->info->device());
    replaceUnmountLines(&lines, m_impl->info->device());
    replaceFormatLines(&lines, m_impl->info->device());

    if (!bootImages.empty()) {
        for (auto const &bootImage : bootImages) {
            insertWriteKernel(&lines, bootImage);
        }
    }

    // Too many ROMs don't unmount partitions after installation
    insertUnmountEverything(lines.end(), &lines);

    // Remove device check if requested
    std::string key = m_impl->info->patchInfo()->keyFromFilename(
            m_impl->info->filename());
    if (!m_impl->info->patchInfo()->deviceCheck(key)) {
        removeDeviceChecks(&lines);
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    FileUtils::writeFromMemory(directory + "/" + UpdaterScript, contents);
    // UpdaterScript end

    return true;
}

/*!
    \brief Disable assertions for device model/name in updater-script

    \param lines Container holding strings of lines in updater-script file
 */
void StandardPatcher::removeDeviceChecks(std::vector<std::string> *lines)
{
    MBP_regex reLine("^\\s*assert\\s*\\(.*getprop\\s*\\(.*(ro.product.device|ro.build.product)");

    for (auto &line : *lines) {
        if (MBP_regex_search(line, reLine)) {
            MBP_regex_replace(line, MBP_regex("^(\\s*assert\\s*\\()"),
                              "\\1\"true\" == \"true\" || ");
        }
    }
}

/*!
    \brief Insert boilerplate updater-script lines to initialize helper scripts

    \param lines Container holding strings of lines in updater-script file
 */
void StandardPatcher::insertDualBootSh(std::vector<std::string> *lines)
{
    auto it = lines->begin();

    it = lines->insert(it, "package_extract_file(\"dualboot.sh\", \"/tmp/dualboot.sh\");");
    ++it;
    it = insertSetPerms(it, lines, "/tmp/dualboot.sh", 0777);
    it = lines->insert(it, "ui_print(\"NOT INSTALLING AS PRIMARY\");");
    ++it;
    it = insertUnmountEverything(it, lines);
}

/*!
    \brief Insert line(s) to set multiboot kernel for a specific boot image

    \param lines Container holding strings of lines in updater-script file
    \param bootImage Filename of the boot image
 */
void StandardPatcher::insertWriteKernel(std::vector<std::string> *lines,
                                        const std::string &bootImage)
{
    const std::string setKernelLine(
            "run_program(\"/tmp/dualboot.sh\", \"set-multi-kernel\");");
    std::vector<std::string> searchItems;
    searchItems.push_back("loki.sh");
    searchItems.push_back("flash_kernel.sh");
    searchItems.push_back(bootImage);

    // Look for the last line containing the boot image string and insert
    // after that
    for (auto const &item : searchItems) {
        for (auto it = lines->rbegin(); it != lines->rend(); ++it) {
            if (it->find(item) != std::string::npos) {
                // Statements can be on multiple lines, so insert after a
                // semicolon is found
                auto fwdIt = (++it).base();
                while (fwdIt != lines->end()) {
                    if (fwdIt->find(";") != std::string::npos) {
                        lines->insert(++fwdIt, setKernelLine);
                        return;
                    } else {
                        ++fwdIt;
                    }
                }

                break;
            }
        }
    }
}

/*!
    \brief Change partition mounting lines to be multiboot-compatible

    \param lines Container holding strings of lines in updater-script file
    \param device Target device (needed for /dev names)
 */
void StandardPatcher::replaceMountLines(std::vector<std::string> *lines,
                                        Device *device)
{
    auto const pSystem = device->partition(System);
    auto const pCache = device->partition(Cache);
    auto const pData = device->partition(Data);

    for (auto it = lines->begin(); it != lines->end();) {
        auto const &line = *it;

        if (MBP_regex_search(line, MBP_regex("^\\s*mount\\s*\\(.*$"))
                || MBP_regex_search(line, MBP_regex(
                "^\\s*run_program\\s*\\(\\s*\"[^\"]*busybox\"\\s*,\\s*\"mount\".*$"))) {
            if (line.find(System) != std::string::npos
                    || (!pSystem.empty() && line.find(pSystem) != std::string::npos)) {
                it = lines->erase(it);
                it = insertMountSystem(it, lines);
            } else if (line.find(Cache) != std::string::npos
                    || (!pCache.empty() && line.find(pCache) != std::string::npos)) {
                it = lines->erase(it);
                it = insertMountCache(it, lines);
            } else if (line.find(Data) != std::string::npos
                    || line.find("userdata") != std::string::npos
                    || (!pData.empty() && line.find(pData) != std::string::npos)) {
                it = lines->erase(it);
                it = insertMountData(it, lines);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
}

/*!
    \brief Change partition unmounting lines to be multiboot-compatible

    \param lines Container holding strings of lines in updater-script file
    \param device Target device (needed for /dev names)
 */
void StandardPatcher::replaceUnmountLines(std::vector<std::string> *lines,
                                          Device *device)
{
    auto const pSystem = device->partition(System);
    auto const pCache = device->partition(Cache);
    auto const pData = device->partition(Data);

    for (auto it = lines->begin(); it != lines->end();) {
        auto const &line = *it;

        if (MBP_regex_search(line, MBP_regex("^\\s*unmount\\s*\\(.*$"))
                || MBP_regex_search(line, MBP_regex(
                "^\\s*run_program\\s*\\(\\s*\"[^\"]*busybox\"\\s*,\\s*\"umount\".*$"))) {
            if (line.find(System) != std::string::npos
                    || (!pSystem.empty() && line.find(pSystem) != std::string::npos)) {
                it = lines->erase(it);
                it = insertUnmountSystem(it, lines);
            } else if (line.find(Cache) != std::string::npos
                    || (!pCache.empty() && line.find(pCache) != std::string::npos)) {
                it = lines->erase(it);
                it = insertUnmountCache(it, lines);
            } else if (line.find(Data) != std::string::npos
                    || line.find("userdata") != std::string::npos
                    || (!pData.empty() && line.find(pData) != std::string::npos)) {
                it = lines->erase(it);
                it = insertUnmountData(it, lines);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
}

/*!
    \brief Change partition formatting lines to be multiboot-compatible

    \param lines Container holding strings of lines in updater-script file
    \param device Target device (needed for /dev names)
 */
void StandardPatcher::replaceFormatLines(std::vector<std::string> *lines,
                                         Device *device)
{
    auto const pSystem = device->partition(System);
    auto const pCache = device->partition(Cache);
    auto const pData = device->partition(Data);

    for (auto it = lines->begin(); it != lines->end();) {
        auto const &line = *it;

        if (MBP_regex_search(line, MBP_regex("^\\s*format\\s*\\(.*$"))) {
            if (line.find(System) != std::string::npos
                    || (!pSystem.empty() && line.find(pSystem) != std::string::npos)) {
                it = lines->erase(it);
                it = insertFormatSystem(it, lines);
            } else if (line.find(Cache) != std::string::npos
                    || (!pCache.empty() && line.find(pCache) != std::string::npos)) {
                it = lines->erase(it);
                it = insertFormatCache(it, lines);
            } else if (line.find("userdata") != std::string::npos
                    || (!pData.empty() && line.find(pData) != std::string::npos)) {
                it = lines->erase(it);
                it = insertFormatData(it, lines);
            } else {
                ++it;
            }
        } else if (MBP_regex_search(line, MBP_regex(
                "delete_recursive\\s*\\([^\\)]*\"/system\""))) {
            it = lines->erase(it);
            it = insertFormatSystem(it, lines);
        } else if (MBP_regex_search(line, MBP_regex(
                "delete_recursive\\s*\\([^\\)]*\"/cache\""))) {
            it = lines->erase(it);
            it = insertFormatCache(it, lines);
        } else {
            ++it;
        }
    }
}

/*!
    \brief Insert updater-script line to mount the ROM's /system

    \param position Position to insert at
    \param lines Container holding strings of lines in updater-script file

    \return Iterator to position after the inserted line(s)
 */
std::vector<std::string>::iterator
StandardPatcher::insertMountSystem(std::vector<std::string>::iterator position,
                                   std::vector<std::string> *lines)
{
    position = lines->insert(
            position, (boost::format(Mount) % System).str());
    return ++position;
}

/*!
    \brief Insert updater-script line to mount the ROM's /cache

    \param position Position to insert at
    \param lines Container holding strings of lines in updater-script file

    \return Iterator to position after the inserted line(s)
 */
std::vector<std::string>::iterator
StandardPatcher::insertMountCache(std::vector<std::string>::iterator position,
                                  std::vector<std::string> *lines)
{
    position = lines->insert(
            position, (boost::format(Mount) % Cache).str());
    return ++position;
}

/*!
    \brief Insert updater-script line to mount the ROM's /data

    \param position Position to insert at
    \param lines Container holding strings of lines in updater-script file

    \return Iterator to position after the inserted line(s)
 */
std::vector<std::string>::iterator
StandardPatcher::insertMountData(std::vector<std::string>::iterator position,
                                 std::vector<std::string> *lines)
{
    position = lines->insert(
            position, (boost::format(Mount) % Data).str());
    return ++position;
}

/*!
    \brief Insert updater-script line to unmount the ROM's /system

    \param position Position to insert at
    \param lines Container holding strings of lines in updater-script file

    \return Iterator to position after the inserted line(s)
 */
std::vector<std::string>::iterator
StandardPatcher::insertUnmountSystem(std::vector<std::string>::iterator position,
                                     std::vector<std::string> *lines)
{
    position = lines->insert(
            position, (boost::format(Unmount) % System).str());
    return ++position;
}

/*!
    \brief Insert updater-script line to unmount the ROM's /cache

    \param position Position to insert at
    \param lines Container holding strings of lines in updater-script file

    \return Iterator to position after the inserted line(s)
 */
std::vector<std::string>::iterator
StandardPatcher::insertUnmountCache(std::vector<std::string>::iterator position,
                                    std::vector<std::string> *lines)
{
    position = lines->insert(
            position, (boost::format(Unmount) % Cache).str());
    return ++position;
}

/*!
    \brief Insert updater-script line to unmount the ROM's /data

    \param position Position to insert at
    \param lines Container holding strings of lines in updater-script file

    \return Iterator to position after the inserted line(s)
 */
std::vector<std::string>::iterator
StandardPatcher::insertUnmountData(std::vector<std::string>::iterator position,
                                   std::vector<std::string> *lines)
{
    position = lines->insert(
            position, (boost::format(Unmount) % Data).str());
    return ++position;
}

/*!
    \brief Insert updater-script line to unmount all partitions

    \param position Position to insert at
    \param lines Container holding strings of lines in updater-script file

    \return Iterator to position after the inserted line(s)
 */
std::vector<std::string>::iterator
StandardPatcher::insertUnmountEverything(std::vector<std::string>::iterator position,
                                         std::vector<std::string> *lines)
{
    position = lines->insert(
            position, (boost::format(Unmount) % "everything").str());
    return ++position;
}

/*!
    \brief Insert updater-script line to format the ROM's /system

    \param position Position to insert at
    \param lines Container holding strings of lines in updater-script file

    \return Iterator to position after the inserted line(s)
 */
std::vector<std::string>::iterator
StandardPatcher::insertFormatSystem(std::vector<std::string>::iterator position,
                                    std::vector<std::string> *lines)
{
    position = lines->insert(
            position, (boost::format(Format) % System).str());
    return ++position;
}

/*!
    \brief Insert updater-script line to format the ROM's /cache

    \param position Position to insert at
    \param lines Container holding strings of lines in updater-script file

    \return Iterator to position after the inserted line(s)
 */
std::vector<std::string>::iterator
StandardPatcher::insertFormatCache(std::vector<std::string>::iterator position,
                                   std::vector<std::string> *lines)
{
    position = lines->insert(
            position, (boost::format(Format) % Cache).str());
    return ++position;
}

/*!
    \brief Insert updater-script line to format the ROM's /data

    \param position Position to insert at
    \param lines Container holding strings of lines in updater-script file

    \return Iterator to position after the inserted line(s)
 */
std::vector<std::string>::iterator
StandardPatcher::insertFormatData(std::vector<std::string>::iterator position,
                                  std::vector<std::string> *lines)
{
    position = lines->insert(
            position, (boost::format(Format) % Data).str());
    return ++position;
}

/*!
    \brief Insert updater-script line for setting permissions for a file

    \param position Position to insert at
    \param lines Container holding strings of lines in updater-script file
    \param file File to change the permissions form
    \param mode Octal unix permissions

    \return Iterator to position after the inserted line(s)
 */
std::vector<std::string>::iterator
StandardPatcher::insertSetPerms(std::vector<std::string>::iterator position,
                                std::vector<std::string> *lines,
                                const std::string &file,
                                unsigned int mode)
{
    position = lines->insert(position, (boost::format(
            "run_program(\"/sbin/busybox\", \"chmod\", \"0%2$o\", \"%1%\");")
            % file % mode).str());
    return ++position;
}
