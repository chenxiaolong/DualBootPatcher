/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mbp/autopatchers/mountcmdpatcher.h"

#include <cstring>

#include "mbp/private/fileutils.h"
#include "mbp/private/stringutils.h"


namespace mbp
{

/*! \cond INTERNAL */
class MountCmdPatcher::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
};
/*! \endcond */


const std::string MountCmdPatcher::Id = "MountCmdPatcher";

// Xposed
static const std::string FlashScript =
        "META-INF/com/google/android/flash-script.sh";
// OpenGapps
static const std::string InstallerScript =
        "installer.sh";


MountCmdPatcher::MountCmdPatcher(const PatcherConfig * const pc,
                                 const FileInfo * const info) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
}

MountCmdPatcher::~MountCmdPatcher()
{
}

ErrorCode MountCmdPatcher::error() const
{
    return ErrorCode();
}

std::string MountCmdPatcher::id() const
{
    return Id;
}

std::vector<std::string> MountCmdPatcher::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> MountCmdPatcher::existingFiles() const
{
    return { FlashScript, InstallerScript };
}

static bool spaceOrEnd(const char *ptr)
{
    return !*ptr || isspace(*ptr);
}

static bool patchFile(const std::string &path)
{
    std::string contents;

    ErrorCode ret = FileUtils::readToString(path, &contents);
    if (ret != ErrorCode::NoError) {
        return false;
    }

    std::vector<std::string> lines = StringUtils::split(contents, '\n');

    for (std::string &line : lines) {
        const char *ptr = line.data();

        // Skip whitespace
        for (; *ptr && isspace(*ptr); ++ptr);

        if ((strncmp(ptr, "mount", 5) == 0 && spaceOrEnd(ptr + 5))
                || (strncmp(ptr, "umount", 6) == 0 && spaceOrEnd(ptr + 6))) {
            line.insert(ptr - line.data(), "/sbin/");
        }
    }

    contents = StringUtils::join(lines, "\n");
    FileUtils::writeFromString(path, contents);

    return true;
}

bool MountCmdPatcher::patchFiles(const std::string &directory)
{
    patchFile(directory + "/" + FlashScript);
    patchFile(directory + "/" + InstallerScript);

    // Don't fail if an error occurs
    return true;
}

}
