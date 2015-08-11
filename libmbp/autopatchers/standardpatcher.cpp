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

#include "autopatchers/standardpatcher.h"

#include <regex>

#include "private/fileutils.h"
#include "private/stringutils.h"


namespace mbp
{

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

#define MOUNT_FMT \
        "run_program(\"/update-binary-tool\", \"mount\", \"%s\");"
#define UNMOUNT_FMT \
        "run_program(\"/update-binary-tool\", \"unmount\", \"%s\");"
#define FORMAT_FMT \
        "run_program(\"/update-binary-tool\", \"format\", \"%s\");"


StandardPatcher::StandardPatcher(const PatcherConfig * const pc,
                                 const FileInfo * const info) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
}

StandardPatcher::~StandardPatcher()
{
}

ErrorCode StandardPatcher::error() const
{
    return ErrorCode();
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
    return { UpdaterScript };
}

bool StandardPatcher::patchFiles(const std::string &directory)
{
    std::string contents;

    FileUtils::readToString(directory + "/" + UpdaterScript, &contents);

    if (contents.size() >= 2 && std::memcmp(contents.data(), "#!", 2) == 0) {
        // Ignore any script with a shebang line
        return true;
    }

    std::vector<std::string> lines = StringUtils::split(contents, '\n');

    replaceMountLines(&lines, m_impl->info->device());
    replaceUnmountLines(&lines, m_impl->info->device());
    replaceFormatLines(&lines, m_impl->info->device());
    fixBlockUpdateLines(&lines, m_impl->info->device());
    fixImageExtractLines(&lines, m_impl->info->device());

    contents = StringUtils::join(lines, "\n");
    FileUtils::writeFromString(directory + "/" + UpdaterScript, contents);

    return true;
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

#define RE_FUNC(name, args)     "(^|[^a-z])" name "\\s*\\(\\s*" args "[^\\)]*\\)"
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

    static auto const re1 = std::regex(
            RE_FUNC("mount", ""));
    static auto const re2 = std::regex(
            RE_FUNC("run_program",
                    RE_ARG(RE_ARG_ANY "busybox")
                    RE_ARG_SEP
                    RE_ARG("mount")));
    static auto const re3 = std::regex(
            RE_FUNC("run_program",
                    RE_ARG(RE_ARG_ANY "/mount")));

    for (auto it = lines->begin(); it != lines->end(); ++it) {
        bool isMountLine = std::regex_search(*it, re1)
                || std::regex_search(*it, re2)
                || std::regex_search(*it, re3);

        if (isMountLine) {
            bool isSystem = it->find("/system") != std::string::npos
                    || findItemsInString(*it, systemDevs);
            bool isCache = it->find("/cache") != std::string::npos
                    || findItemsInString(*it, cacheDevs);
            bool isData = it->find("/data") != std::string::npos
                    || it->find("/userdata") != std::string::npos
                    || findItemsInString(*it, dataDevs);

            if (isSystem) {
                *it = StringUtils::format(MOUNT_FMT, "/system");
            } else if (isCache) {
                *it = StringUtils::format(MOUNT_FMT, "/cache");
            } else if (isData) {
                *it = StringUtils::format(MOUNT_FMT, "/data");
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

    static auto const re1 = std::regex(
            RE_FUNC("unmount", ""));
    static auto const re2 = std::regex(
            RE_FUNC("run_program",
                    RE_ARG(RE_ARG_ANY "busybox")
                    RE_ARG_SEP
                    RE_ARG("umount")));

    for (auto it = lines->begin(); it != lines->end(); ++it) {
        // Quick hack for CM12
        if (it->find("ifelse(is_mounted(\"/system\"), unmount(\"/system\"));")
                != std::string::npos) {
            it = lines->erase(it);
            continue;
        }

        bool isUnmountLine = std::regex_search(*it, re1)
                || std::regex_search(*it, re2);

        if (isUnmountLine) {
            bool isSystem = it->find("/system") != std::string::npos
                    || findItemsInString(*it, systemDevs);
            bool isCache = it->find("/cache") != std::string::npos
                    || findItemsInString(*it, cacheDevs);
            bool isData = it->find("/data") != std::string::npos
                    || it->find("/userdata") != std::string::npos
                    || findItemsInString(*it, dataDevs);

            if (isSystem) {
                *it = StringUtils::format(UNMOUNT_FMT, "/system");
            } else if (isCache) {
                *it = StringUtils::format(UNMOUNT_FMT, "/cache");
            } else if (isData) {
                *it = StringUtils::format(UNMOUNT_FMT, "/data");
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

    static auto const re1 = std::regex(RE_FUNC("format", ""));
    static auto const re2 = std::regex(
            RE_FUNC("delete_recursive", RE_ARG("/system")));
    static auto const re3 = std::regex(
            RE_FUNC("delete_recursive", RE_ARG("/cache")));
    static auto const re4 = std::regex(
            RE_FUNC("run_program", RE_ARG(RE_ARG_ANY "/format.sh")));
    static auto const re5 = std::regex(
            RE_FUNC("run_program", RE_ARG(RE_ARG_ANY "/mke2fs")));

    for (auto it = lines->begin(); it != lines->end(); ++it) {
        if (std::regex_search(*it, re1) || std::regex_search(*it, re5)) {
            bool isSystem = it->find("/system") != std::string::npos
                    || findItemsInString(*it, systemDevs);
            bool isCache = it->find("/cache") != std::string::npos
                    || findItemsInString(*it, cacheDevs);
            bool isData = it->find("/data") != std::string::npos
                    || it->find("/userdata") != std::string::npos
                    || findItemsInString(*it, dataDevs);

            if (isSystem) {
                *it = StringUtils::format(FORMAT_FMT, "/system");
            } else if (isCache) {
                *it = StringUtils::format(FORMAT_FMT, "/cache");
            } else if (isData) {
                *it = StringUtils::format(FORMAT_FMT, "/data");
            }
        } else if (std::regex_search(*it, re2)) {
            *it = StringUtils::format(FORMAT_FMT, "/system");
        } else if (std::regex_search(*it, re3)) {
            *it = StringUtils::format(FORMAT_FMT, "/cache");
        } else if (std::regex_search(*it, re4)) {
            *it = StringUtils::format(FORMAT_FMT, "/data");
        }
    }
}

void StandardPatcher::fixBlockUpdateLines(std::vector<std::string> *lines,
                                          Device *device)
{
    auto const systemDevs = device->systemBlockDevs();

    for (auto it = lines->begin(); it != lines->end(); ++it) {
        if (it->find("block_image_update") != std::string::npos) {
            // References to the system partition should become /mb/system.img
            for (auto const &dev : systemDevs) {
                StringUtils::replace_all(&(*it), dev, "/mb/system.img");
            }
        }
    }
}

void StandardPatcher::fixImageExtractLines(std::vector<std::string> *lines,
                                           Device *device)
{
    auto const systemDevs = device->systemBlockDevs();

    for (auto it = lines->begin(); it != lines->end(); ++it) {
        if (it->find("package_extract_file") != std::string::npos) {
            // References to the system partition should become /mb/system.img
            for (auto const &dev : systemDevs) {
                StringUtils::replace_all(&(*it), dev, "/mb/system.img");
            }
        }
    }
}

}
