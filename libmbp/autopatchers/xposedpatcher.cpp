/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "autopatchers/xposedpatcher.h"

#include <cstring>

#include "private/fileutils.h"
#include "private/stringutils.h"


namespace mbp
{

/*! \cond INTERNAL */
class XposedPatcher::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
};
/*! \endcond */


const std::string XposedPatcher::Id = "XposedPatcher";

static const std::string FlashScript =
        "META-INF/com/google/android/flash-script.sh";


XposedPatcher::XposedPatcher(const PatcherConfig * const pc,
                             const FileInfo * const info) :
    m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
}

XposedPatcher::~XposedPatcher()
{
}

ErrorCode XposedPatcher::error() const
{
    return ErrorCode();
}

std::string XposedPatcher::id() const
{
    return Id;
}

std::vector<std::string> XposedPatcher::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> XposedPatcher::existingFiles() const
{
    return { FlashScript };
}

bool XposedPatcher::patchFiles(const std::string &directory)
{
    std::string contents;

    ErrorCode ret = FileUtils::readToString(
            directory + "/" + FlashScript, &contents);
    if (ret != ErrorCode::NoError) {
        // Don't fail if it doesn't exist
        return true;
    }

    std::vector<std::string> lines = StringUtils::split(contents, '\n');

    for (std::string &line : lines) {
        const char *ptr = line.data();

        // Skip whitespace
        for (; *ptr && isspace(*ptr); ++ptr);

        if (strncmp(ptr, "mount", 5) == 0 || strncmp(ptr, "umount", 6) == 0) {
            line.insert(ptr - line.data(), "/sbin/");
        }
    }

    contents = StringUtils::join(lines, "\n");
    FileUtils::writeFromString(directory + "/" + FlashScript, contents);

    return true;
}

}
