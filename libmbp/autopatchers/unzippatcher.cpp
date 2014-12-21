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

#include "autopatchers/unzippatcher.h"

#include <unordered_map>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>

#include "private/fileutils.h"
#include "private/logging.h"
#include "private/regex.h"


/*! \cond INTERNAL */
class UnzipPatcher::Impl
{
public:
    const PatcherConfig *pc;
    const FileInfo *info;
    PatchInfo::AutoPatcherArgs args;

    std::vector<std::string> commands;
    std::vector<std::string> files;

    PatcherError error;
};
/*! \endcond */


const std::string UnzipPatcher::Id("UnzipPatcher");

static const std::string ArgFile("file");
static const std::string ArgCommand("command");

static const std::string ExtractCommand =
        "sed '1,/^UNZIP_MAGIC$/d' \"${0}\" | base64 -d | xz -d > /tmp/unzip;"
        "chmod 777 /tmp/unzip";
static const std::string Exit = "exit 0";
static const std::string Magic = "UNZIP_MAGIC";


UnzipPatcher::UnzipPatcher(const PatcherConfig * const pc,
                           const FileInfo * const info,
                           const PatchInfo::AutoPatcherArgs &args)
    : m_impl(new Impl())
{
    m_impl->pc = pc;
    m_impl->info = info;
    m_impl->args = args;

    if (args.find(ArgFile) != args.end()) {
        std::vector<std::string> files;
        boost::split(files, args.at(ArgFile), boost::is_any_of(";"));
        for (auto const &file : files) {
            if (!file.empty()) {
                m_impl->files.push_back(file);
            }
        }
    } else {
        Log::log(Log::Warning, "UnzipPatcher: 'file' argument not provided");
        return;
    }

    if (args.find(ArgCommand) != args.end()) {
        std::vector<std::string> commands;
        boost::split(commands, args.at(ArgCommand), boost::is_any_of(";"));
        for (auto const &command : commands) {
            if (!command.empty()) {
                m_impl->commands.push_back(command);
            }
        }
    }
}

UnzipPatcher::~UnzipPatcher()
{
}

PatcherError UnzipPatcher::error() const
{
    return m_impl->error;
}

std::string UnzipPatcher::id() const
{
    return Id;
}

std::vector<std::string> UnzipPatcher::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> UnzipPatcher::existingFiles() const
{
    return m_impl->files;
}

bool UnzipPatcher::patchFiles(const std::string &directory,
                              const std::vector<std::string> &bootImages)
{
    (void) bootImages;

    if (m_impl->files.empty()) {
        return true;
    }

    std::string unzipPath = m_impl->pc->binariesDirectory()
            + "/android/unzip.xz.base64";

    std::vector<unsigned char> unzip;
    auto ret = FileUtils::readToMemory(unzipPath, &unzip);
    if (ret.errorCode() != MBP::ErrorCode::NoError) {
        m_impl->error = ret;
        return false;
    }

    for (const std::string &file : m_impl->files) {
        std::vector<unsigned char> contents;

        FileUtils::readToMemory(directory + "/" + file, &contents);
        std::string strContents(contents.begin(), contents.end());
        std::vector<std::string> lines;
        boost::split(lines, strContents, boost::is_any_of("\n"));

        if (lines.empty()) {
            continue;
        }

        // Replace unzip commands
        for (auto const &command : m_impl->commands) {
            for (auto it = lines.begin(); it != lines.end(); ++it) {
                if (MBP_regex_search(*it, MBP_regex("^\\s*" + command))) {
                    // Just do the simplest match. Anything else probably
                    // belongs in a patch file anyway
                    boost::replace_first(*it, command, "/tmp/unzip");
                }
            }
        }

        // Add the line to extract miniunzip after the shebang line if found
        if (boost::starts_with(lines[0], "#!")) {
            lines.insert(lines.begin() + 1, ExtractCommand);
        } else {
            lines.insert(lines.begin(), ExtractCommand);
        }

        // Add exit command to not mess up the interpreter and the magic string
        // for sed
        lines.push_back(Exit);
        lines.push_back(Magic);
        // Need newline before base64 data
        lines.push_back(std::string());

        strContents = boost::join(lines, "\n");
        contents.assign(strContents.begin(), strContents.end());

        // Insert base64-encoded miniunzip program
        contents.insert(contents.end(), unzip.begin(), unzip.end());

        FileUtils::writeFromMemory(directory + "/" + file, contents);
    }

    return true;
}
