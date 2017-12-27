/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbpatcher/autopatchers/mountcmdpatcher.h"

#include <cstring>

#include "mbcommon/string.h"

#include "mbpatcher/private/fileutils.h"


namespace mb::patcher
{

const std::string MountCmdPatcher::Id = "MountCmdPatcher";

// Xposed
static const std::string FlashScript =
        "META-INF/com/google/android/flash-script.sh";
// OpenGapps
static const std::string InstallerScript =
        "installer.sh";


MountCmdPatcher::MountCmdPatcher(const PatcherConfig &pc, const FileInfo &info)
{
    (void) pc;
    (void) info;
}

MountCmdPatcher::~MountCmdPatcher() = default;

ErrorCode MountCmdPatcher::error() const
{
    return {};
}

std::string MountCmdPatcher::id() const
{
    return Id;
}

std::vector<std::string> MountCmdPatcher::new_files() const
{
    return {};
}

std::vector<std::string> MountCmdPatcher::existing_files() const
{
    return { FlashScript, InstallerScript };
}

static bool space_or_end(const char *ptr)
{
    return !*ptr || isspace(*ptr);
}

static bool patch_file(const std::string &path)
{
    std::string contents;

    ErrorCode ret = FileUtils::read_to_string(path, &contents);
    if (ret != ErrorCode::NoError) {
        return false;
    }

    auto lines = split(contents, '\n');

    for (auto &line : lines) {
        const char *ptr = line.data();

        // Skip whitespace
        for (; *ptr && isspace(*ptr); ++ptr);

        if ((strncmp(ptr, "mount", 5) == 0 && space_or_end(ptr + 5))
                || (strncmp(ptr, "umount", 6) == 0 && space_or_end(ptr + 6))) {
            line.insert(static_cast<size_t>(ptr - line.data()), "/sbin/");
        }
    }

    contents = join(lines, "\n");
    FileUtils::write_from_string(path, contents);

    return true;
}

bool MountCmdPatcher::patch_files(const std::string &directory)
{
    patch_file(directory + "/" + FlashScript);
    patch_file(directory + "/" + InstallerScript);

    // Don't fail if an error occurs
    return true;
}

}
