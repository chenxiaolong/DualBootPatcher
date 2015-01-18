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

#include "autopatchers/jfltepatcher.h"

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

#include "autopatchers/standardpatcher.h"
#include "private/fileutils.h"
#include "private/regex.h"


/*! \cond INTERNAL */
class JflteBasePatcher::Impl
{
public:
    const PatcherConfig *pc;
    std::string id;
    const FileInfo *info;
};
/*! \endcond */


static const std::string BuildProp = "system/build.prop";


JflteBasePatcher::JflteBasePatcher(const PatcherConfig * const pc,
                                   const FileInfo * const info)
    : m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
}

JflteBasePatcher::~JflteBasePatcher()
{
}

PatcherError JflteBasePatcher::error() const
{
    return PatcherError();
}

////////////////////////////////////////////////////////////////////////////////

const std::string JflteDalvikCachePatcher::Id = "DalvikCachePatcher";

JflteDalvikCachePatcher::JflteDalvikCachePatcher(const PatcherConfig * const pc,
                                                 const FileInfo* const info)
    : JflteBasePatcher(pc, info)
{
}

std::string JflteDalvikCachePatcher::id() const
{
    return Id;
}

std::vector<std::string> JflteDalvikCachePatcher::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> JflteDalvikCachePatcher::existingFiles() const
{
    std::vector<std::string> files;
    files.push_back(BuildProp);
    return files;
}

bool JflteDalvikCachePatcher::patchFiles(const std::string &directory)
{
    std::vector<unsigned char> contents;

    // BuildProp begin
    FileUtils::readToMemory(directory + "/" + BuildProp, &contents);
    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        if (it->find("dalvik.vm.dexopt-data-only") != std::string::npos) {
            lines.erase(it);
            break;
        }
    }

    strContents = boost::join(lines, "\n");
    contents.assign(strContents.begin(), strContents.end());
    FileUtils::writeFromMemory(directory + "/" + BuildProp, contents);
    // BuildProp end

    return true;
}
