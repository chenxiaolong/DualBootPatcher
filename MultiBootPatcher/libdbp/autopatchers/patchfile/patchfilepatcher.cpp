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

#include "autopatchers/patchfile/patchfilepatcher.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <stdio.h>
#endif

#include <fstream>
#include <unordered_map>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/regex.hpp>

#include "private/fileutils.h"
#include "private/logging.h"


/*! \cond INTERNAL */
class PatchFilePatcher::Impl
{
public:
    const PatcherPaths *pp;
    const FileInfo *info;
    PatchInfo::AutoPatcherArgs args;

    std::string patchFile;
    std::vector<std::string> files;

    PatcherError error;

    bool runProcess(std::string program, std::vector<std::string> &progArgs,
                    std::string *out, int *exitCode);
};
/*! \endcond */


const std::string PatchFilePatcher::Id("PatchFile");

static const std::string ArgFile("file");


PatchFilePatcher::PatchFilePatcher(const PatcherPaths * const pp,
                                   const FileInfo * const info,
                                   const PatchInfo::AutoPatcherArgs &args)
    : m_impl(new Impl())
{
    m_impl->pp = pp;
    m_impl->info = info;
    m_impl->args = args;

    // The arguments should have the patch to the file
    if (args.find(ArgFile) == args.end()) {
        Log::log(Log::Warning, "Arguments does not contain the path to a patch file");
        return;
    }

    m_impl->patchFile = pp->patchesDirectory() + "/" + args.at(ArgFile);

    std::vector<unsigned char> contents;
    auto ret = FileUtils::readToMemory(m_impl->patchFile, &contents);
    if (ret.errorCode() != MBP::ErrorCode::NoError) {
        Log::log(Log::Warning, "Failed to read patch file");
        return;
    }

    std::string strContents(contents.begin(), contents.end());
    std::vector<std::string> lines;
    boost::split(lines, strContents, boost::is_any_of("\n"));

    const boost::regex reOrigFile("---\\s+(.+?)(?:$|\t)");

    for (auto it = lines.begin(); it != lines.end(); ++it) {
        boost::smatch what;

        if (boost::regex_search(*it, what, reOrigFile)) {
            std::string file = what.str(1);
            if (boost::starts_with(file, "\"") && boost::ends_with(file, "\"")) {
                file.erase(file.begin());
                file.erase(file.end() - 1);
            }

            // Skip files containing escaped characters
            if (file.find("\\") != std::string::npos) {
                Log::log(Log::Warning,
                         "Skipping file with escaped characters in filename: %s",
                         file);
                continue;
            }

            // Strip leading slash (-p1)
            auto it = file.find("/");
            if (it != std::string::npos) {
                file.erase(file.begin(), file.begin() + it + 1);
            }

            Log::log(Log::Debug, "Found file in patch: %s", file);

            m_impl->files.push_back(file);
        }
    }
}

PatchFilePatcher::~PatchFilePatcher()
{
}

PatcherError PatchFilePatcher::error() const
{
    return m_impl->error;
}

std::string PatchFilePatcher::id() const
{
    return Id;
}

std::vector<std::string> PatchFilePatcher::newFiles() const
{
    return std::vector<std::string>();
}

std::vector<std::string> PatchFilePatcher::existingFiles() const
{
    return m_impl->files;
}

bool PatchFilePatcher::patchFiles(const std::string &directory,
                                  const std::vector<std::string> &bootImages)
{
    (void) bootImages;

#if defined(__ANDROID__)
    std::string patch = m_impl->pp->binariesDirectory();
    patch += "/android/";
    patch += m_impl->info->device()->architecture();
    patch += "/patch";
#elif defined(_WIN32)
    std::string patch = m_impl->pp->binariesDirectory();
    patch += "/windows/hctap.exe";
#else
    std::string patch = "patch";
#endif

    std::vector<std::string> args;
    args.push_back("--no-backup-if-mismatch");
    args.push_back("-p1");
    args.push_back("-i");
    args.push_back(m_impl->patchFile);
    args.push_back("-d");
    args.push_back(directory);

    std::string output;
    int exitCode = -1;

    std::vector<std::string> lines;
    boost::split(lines, output, boost::is_any_of("\n"));

    if (!m_impl->runProcess(patch, args, &output, &exitCode) || exitCode != 0) {
        for (auto &line : lines) {
            Log::log(Log::Error, "patch output: %s", line);
        }

        m_impl->error = PatcherError::createPatchingError(
                MBP::ErrorCode::ApplyPatchFileError);
        return false;
    }

    for (auto &line : lines) {
        Log::log(Log::Debug, "patch output: %s", line);
    }

    return true;
}

#ifdef _WIN32

bool PatchFilePatcher::Impl::runProcess(std::string program,
                                        std::vector<std::string> &progArgs,
                                        std::string *out,
                                        int *exitCode)
{
    std::string args;
    args += "\"";
    args += program;
    args += "\"";

    for (auto &arg : progArgs) {
        args += " ";
        args += "\"";
        args += arg;
        args += "\"";
    }

    SECURITY_ATTRIBUTES secAttr;
    HANDLE hStdinRead;
    HANDLE hStdinWrite;
    HANDLE hStdoutRead;
    HANDLE hStdoutWrite;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char command[1024];

    if (args.size() >= 1024) {
        return false;
    }

    // Setup secAttr struct
    ZeroMemory(&secAttr, sizeof(secAttr));
    secAttr.nLength = sizeof(secAttr);
    secAttr.bInheritHandle = TRUE;
    secAttr.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hStdoutRead, &hStdoutWrite, &secAttr, 0)) {
        return false;
    }

    if (!SetHandleInformation(hStdoutRead, HANDLE_FLAG_INHERIT, 0)) {
        return false;
    }

    if (!CreatePipe(&hStdinRead, &hStdinWrite, &secAttr, 0)) {
        return false;
    }

    if (!SetHandleInformation(hStdinWrite, HANDLE_FLAG_INHERIT, 0)) {
        return false;
    }

    ZeroMemory(&si, sizeof(si));
    si.cb  = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hStdinRead;
    si.hStdOutput = hStdoutWrite;
    si.hStdError = hStdoutWrite;

    ZeroMemory(&pi, sizeof(pi));

    strcpy(command, args.c_str());

    if (!CreateProcess(
            nullptr,
            command,
            nullptr,
            nullptr,
            TRUE,
            CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP,
            nullptr,
            nullptr,
            &si,
            &pi)) {
        return false;
    }

    if (!CloseHandle(hStdoutWrite)) {
        return false;
    }

    std::string output;
    char c[1024];
    DWORD cl;
    while (ReadFile(hStdoutRead, &c, sizeof(c), &cl, nullptr)) {
        if (!cl) {
            break;
        }

        output.insert(output.end(), c, c + cl);
    }
    out->swap(output);

    WaitForSingleObject(pi.hProcess, INFINITE);

    GetExitCodeProcess(pi.hProcess, (DWORD *) exitCode);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return true;
}

#else

bool PatchFilePatcher::Impl::runProcess(std::string program,
                                        std::vector<std::string> &progArgs,
                                        std::string *out,
                                        int *exitCode)
{
    // No need to fork and exec when popen can just use shell io redirection

    std::string args;
    args += "\"";
    args += program;
    args += "\"";

    for (auto &arg : progArgs) {
        args += " ";
        args += "\"";
        args += arg;
        args += "\"";
    }

    args += " 2>&1";

    FILE *fp = popen(args.c_str(), "r");
    if (fp == nullptr) {
        return false;
    }

    std::string output;
    char buf[1024];
    while (!feof(fp)) {
        if (fgets(buf, sizeof(buf), fp) != nullptr) {
            output += buf;
        }
    }

    out->swap(output);
    *exitCode = pclose(fp);
    return true;
}

#endif
