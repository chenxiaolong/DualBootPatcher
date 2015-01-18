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

#include "autopatchers/standardpatcher.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
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
        = "run_program(\"/update-binary-tool\", \"mount\", \"%1%\");";
static const std::string Unmount
        = "run_program(\"/update-binary-tool\", \"unmount\", \"%1%\");";
static const std::string Format
        = "run_program(\"/update-binary-tool\", \"format\", \"%1%\");";


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

bool StandardPatcher::patchFiles(const std::string &directory)
{
    std::vector<unsigned char> contents;

    FileUtils::readToMemory(directory + "/" + UpdaterScript, &contents);
    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    replaceMountLines(&lines, m_impl->info->device());
    replaceUnmountLines(&lines, m_impl->info->device());
    replaceFormatLines(&lines, m_impl->info->device());
    fixBlockUpdateLines(&lines, m_impl->info->device());

    // Remove device check if requested
    std::string key = m_impl->info->patchInfo()->keyFromFilename(
            m_impl->info->filename());
    if (!m_impl->info->patchInfo()->deviceCheck(key)) {
        removeDeviceChecks(&lines);
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    FileUtils::writeFromMemory(directory + "/" + UpdaterScript, contents);

    return true;
}

/*!
    \brief Disable assertions for device model/name in updater-script

    \param lines Container holding strings of lines in updater-script file
 */
void StandardPatcher::removeDeviceChecks(std::vector<std::string> *lines)
{
    MBP_regex reLine("assert\\s*\\(.*getprop\\s*\\(.*(ro.product.device|ro.build.product)");

    for (auto &line : *lines) {
        if (MBP_regex_search(line, reLine)) {
            MBP_regex_replace(line, MBP_regex("^(\\s*assert\\s*\\()"),
                              "\\1\"true\" == \"true\" || ");
        }
    }
}

static bool findItemsInString(const std::string &haystack,
                              const std::vector<std::string> &needles)
{
    for (auto const &needle : needles) {
        if (haystack.find(needle) != std::string::npos) {
            return true;
        }
    }

    return false;
}

#define RE_FUNC(name, args)     "(^|[^a-z])" name "\\s*\\(\\s*" args "[^\\)]*\\)\\s*;"
#define RE_ARG(x)               "\"" x "\""
#define RE_ARG_ANY              "[^\"]*"
#define RE_ARG_SEP              "\\s*,\\s*"

/*!
    \brief Change partition mounting lines to be multiboot-compatible

    \param lines Container holding strings of lines in updater-script file
    \param device Target device (needed for /dev names)
 */
void StandardPatcher::replaceMountLines(std::vector<std::string> *lines,
                                        Device *device)
{
    auto const systemDevs = device->systemBlockDevs();
    auto const cacheDevs = device->cacheBlockDevs();
    auto const dataDevs = device->dataBlockDevs();

    static auto const re1 = MBP_regex(
            RE_FUNC("mount", ""));
    static auto const re2 = MBP_regex(
            RE_FUNC("run_program",
                    RE_ARG(RE_ARG_ANY "busybox")
                    RE_ARG_SEP
                    RE_ARG("mount")));
    static auto const re3 = MBP_regex(
            RE_FUNC("run_program",
                    RE_ARG(RE_ARG_ANY "/mount")));

    for (auto it = lines->begin(); it != lines->end(); ++it) {
        bool isMountLine = MBP_regex_search(*it, re1)
                || MBP_regex_search(*it, re2)
                || MBP_regex_search(*it, re3);

        if (isMountLine) {
            bool isSystem = it->find("/system") != std::string::npos
                    || findItemsInString(*it, systemDevs);
            bool isCache = it->find("/cache") != std::string::npos
                    || findItemsInString(*it, cacheDevs);
            bool isData = it->find("/data") != std::string::npos
                    || it->find("/userdata") != std::string::npos
                    || findItemsInString(*it, dataDevs);

            if (isSystem) {
                *it = (boost::format(Mount) % "/system").str();
            } else if (isCache) {
                *it = (boost::format(Mount) % "/cache").str();
            } else if (isData) {
                *it = (boost::format(Mount) % "/data").str();
            }
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
    auto const systemDevs = device->systemBlockDevs();
    auto const cacheDevs = device->cacheBlockDevs();
    auto const dataDevs = device->dataBlockDevs();

    static auto const re1 = MBP_regex(
            RE_FUNC("unmount", ""));
    static auto const re2 = MBP_regex(
            RE_FUNC("run_program",
                    RE_ARG(RE_ARG_ANY "busybox")
                    RE_ARG_SEP
                    RE_ARG("umount")));

    for (auto it = lines->begin(); it != lines->end(); ++it) {
        bool isUnmountLine = MBP_regex_search(*it, re1)
                || MBP_regex_search(*it, re2);

        if (isUnmountLine) {
            bool isSystem = it->find("/system") != std::string::npos
                    || findItemsInString(*it, systemDevs);
            bool isCache = it->find("/cache") != std::string::npos
                    || findItemsInString(*it, cacheDevs);
            bool isData = it->find("/data") != std::string::npos
                    || it->find("/userdata") != std::string::npos
                    || findItemsInString(*it, dataDevs);

            if (isSystem) {
                *it = (boost::format(Unmount) % "/system").str();
            } else if (isCache) {
                *it = (boost::format(Unmount) % "/cache").str();
            } else if (isData) {
                *it = (boost::format(Unmount) % "/data").str();
            }
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
    auto const systemDevs = device->systemBlockDevs();
    auto const cacheDevs = device->cacheBlockDevs();
    auto const dataDevs = device->dataBlockDevs();

    for (auto it = lines->begin(); it != lines->end(); ++it) {
        if (MBP_regex_search(*it, MBP_regex(RE_FUNC("format", "")))) {
            bool isSystem = it->find("/system") != std::string::npos
                    || findItemsInString(*it, systemDevs);
            bool isCache = it->find("/cache") != std::string::npos
                    || findItemsInString(*it, cacheDevs);
            bool isData = it->find("/data") != std::string::npos
                    || it->find("/userdata") != std::string::npos
                    || findItemsInString(*it, dataDevs);

            if (isSystem) {
                *it = (boost::format(Format) % "/system").str();
            } else if (isCache) {
                *it = (boost::format(Format) % "/cache").str();
            } else if (isData) {
                *it = (boost::format(Format) % "/data").str();
            }
        } else if (MBP_regex_search(*it, MBP_regex(
                RE_FUNC("delete_recursive", RE_ARG("/system"))))) {
            *it = (boost::format(Format) % "/system").str();
        } else if (MBP_regex_search(*it, MBP_regex(
                RE_FUNC("delete_recursive", RE_ARG("/cache"))))) {
            *it = (boost::format(Format) % "/cache").str();
        } else if (MBP_regex_search(*it, MBP_regex(
                RE_FUNC("run_program", RE_ARG(RE_ARG_ANY "/format.sh"))))) {
            *it = (boost::format(Format) % "/data").str();
        }
    }
}

void StandardPatcher::fixBlockUpdateLines(std::vector<std::string> *lines,
                                          Device *device)
{
    auto const systemDevs = device->systemBlockDevs();

    for (auto it = lines->begin(); it != lines->end(); ++it) {
        if (it->find("block_image_update") != std::string::npos) {
            // References to the system partition should become
            // /multiboot/system.img
            for (auto const &dev : systemDevs) {
                boost::replace_all(*it, dev, "/multiboot/system.img");
            }
        }
    }
}
