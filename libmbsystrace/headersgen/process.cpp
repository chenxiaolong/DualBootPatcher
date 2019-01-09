/*
 * Copyright (C) 2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "process.h"

#include <thread>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <sys/wait.h>
#  include <unistd.h>
#endif

#include "mbcommon/error_code.h"
#include "mbcommon/finally.h"


using namespace mb;

#ifdef _WIN32

// https://blogs.msdn.microsoft.com/twistylittlepassagesallalike/2011/04/23/everyone-quotes-command-line-arguments-the-wrong-way/
// https://docs.microsoft.com/en-us/cpp/cpp/parsing-cpp-command-line-arguments
static NativeString create_command(const std::vector<NativeString> &args)
{
    NativeString cmd;
    bool first = true;

    for (auto const &arg : args) {
        if (first) {
            first = false;
        } else {
            cmd += L' ';
        }

        if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == NativeString::npos) {
            cmd += arg;
        } else {
            cmd += L'"';

            for (auto it = arg.begin(); ; ++it) {
                size_t backslashes = 0;

                while (it != arg.end() && *it == L'\\') {
                    ++it;
                    ++backslashes;
                }

                if (it == arg.end()) {
                    cmd.append(backslashes * 2, L'\\');
                    break;
                } else if (*it == L'"') {
                    cmd.append(backslashes * 2 + 1, L'\\');
                    cmd.push_back(*it);
                } else {
                    cmd.append(backslashes, L'\\');
                    cmd.push_back(*it);
                }
            }

            cmd += L'"';
        }
    }

    return cmd;
}

static BOOL safely_close(HANDLE &h)
{
    if (h) {
        return CloseHandle(std::exchange(h, nullptr));
    }
    return TRUE;
}

// CreateProcess is not thread safe when it comes to inheriting handles
// https://blogs.msdn.microsoft.com/oldnewthing/20111216-00/?p=8873
static BOOL create_process_with_handles(LPWSTR cmd, LPSTARTUPINFO si,
                                        LPPROCESS_INFORMATION pi,
                                        const std::vector<HANDLE> handles)
{
    if (handles.size() >= MAXDWORD / sizeof(HANDLE)
            || si->cb != sizeof(*si)) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SIZE_T size = 0;

    if (InitializeProcThreadAttributeList(nullptr, 1, 0, &size)
            || GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        return FALSE;
    }

    auto attr_list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(
            HeapAlloc(GetProcessHeap(), 0, size));
    if (!attr_list) {
        return FALSE;
    }

    auto free_attr_list = finally([&] {
        HeapFree(GetProcessHeap(), 0, attr_list);
    });

    if (!InitializeProcThreadAttributeList(attr_list, 1, 0, &size)) {
        return FALSE;
    }

    auto deinit_attr_list = finally([&] {
        DeleteProcThreadAttributeList(attr_list);
    });

    if (!UpdateProcThreadAttribute(attr_list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                   const_cast<void **>(handles.data()),
                                   handles.size() * sizeof(HANDLE),
                                   nullptr, nullptr)) {
        return FALSE;
    }

    STARTUPINFOEX siex = {};
    siex.StartupInfo = *si;
    siex.StartupInfo.cb = sizeof(siex);
    siex.lpAttributeList = attr_list;

    return CreateProcess(nullptr, cmd, nullptr, nullptr, TRUE,
                         CREATE_NEW_PROCESS_GROUP | EXTENDED_STARTUPINFO_PRESENT,
                         nullptr, nullptr, &siex.StartupInfo, pi);
}

oc::result<int> run_process(const std::vector<NativeString> &args,
                            std::string_view in, std::string &out)
{
    if (args.empty()) {
        return std::errc::invalid_argument;
    }

    NativeString cmd = create_command(args);

    HANDLE h_stdin_r;
    HANDLE h_stdin_w;

    if (!CreatePipe(&h_stdin_r, &h_stdin_w, nullptr, 0)) {
        return ec_from_win32();
    }

    auto close_stdin_pipe = finally([&] {
        safely_close(h_stdin_r);
        safely_close(h_stdin_w);
    });

    // Make read end of stdin pipe inheritable
    if (!SetHandleInformation(h_stdin_r, HANDLE_FLAG_INHERIT,
                              HANDLE_FLAG_INHERIT)) {
        return ec_from_win32();
    }

    std::thread stdin_thread([&] {
        auto remain = in;

        while (!remain.empty()) {
            DWORD to_write = std::min<size_t>(remain.size(), MAXDWORD);
            DWORD n;

            if (!WriteFile(h_stdin_w, remain.data(), to_write, &n, nullptr)) {
                return;
            }

            remain.remove_prefix(n);
        }

        safely_close(h_stdin_w);
    });

    auto join_stdin_thread = finally([&] {
        safely_close(h_stdin_w);
        stdin_thread.join();
    });

    HANDLE h_stdout_r;
    HANDLE h_stdout_w;

    if (!CreatePipe(&h_stdout_r, &h_stdout_w, nullptr, 0)) {
        return ec_from_win32();
    }

    auto close_stdout_pipe = finally([&] {
        safely_close(h_stdout_r);
        safely_close(h_stdout_w);
    });

    // Make write end of stdout pipe inheritable
    if (!SetHandleInformation(h_stdout_w, HANDLE_FLAG_INHERIT,
                              HANDLE_FLAG_INHERIT)) {
        return ec_from_win32();
    }

    STARTUPINFOW si = {};
    si.cb  = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = h_stdin_r;
    si.hStdOutput = h_stdout_w;

    PROCESS_INFORMATION pi = {};

    if (!create_process_with_handles(
            // https://blogs.msdn.microsoft.com/oldnewthing/20090601-00/?p=18083
            cmd.data(), &si, &pi, {h_stdin_r, h_stdout_w})) {
        return ec_from_win32();
    }

    auto close_process_handles = finally([&] {
        safely_close(pi.hProcess);
        safely_close(pi.hThread);
    });

    auto kill_process = finally([&] {
        TerminateProcess(pi.hProcess, 127);
        WaitForSingleObject(pi.hProcess, INFINITE);
    });

    if (!safely_close(h_stdout_w) || !safely_close(h_stdin_r)) {
        return ec_from_win32();
    }

    out.clear();
    char buf[1024];
    DWORD n;

    while (ReadFile(h_stdout_r, &buf, sizeof(buf), &n, nullptr)) {
        if (n == 0) {
            break;
        }

        out.append(buf, n);
    }

    if (auto error = GetLastError(); error != ERROR_BROKEN_PIPE) {
        return ec_from_win32(error);
    }

    if (WaitForSingleObject(pi.hProcess, INFINITE) == WAIT_FAILED) {
        return ec_from_win32();
    }

    DWORD exit_code;
    if (!GetExitCodeProcess(pi.hProcess, &exit_code)) {
        return ec_from_win32();
    } else if (exit_code == STILL_ACTIVE) {
        return std::errc::io_error;
    }

    kill_process.dismiss();

    return exit_code;
}

#else

static int safely_close(int &fd)
{
    if (fd >= 0) {
        return close(std::exchange(fd, -1));
    }
    return 0;
}

oc::result<int> run_process(const std::vector<std::string> &args,
                            std::string_view in, std::string &out)
{
    if (args.empty()) {
        return std::errc::invalid_argument;
    }

    int stdin_pipe[2];
    if (pipe2(stdin_pipe, O_CLOEXEC) < 0) {
        return ec_from_errno();
    }

    auto close_stdin_pipe = finally([&] {
        safely_close(stdin_pipe[0]);
        safely_close(stdin_pipe[1]);
    });

    std::thread stdin_thread([&] {
        auto remain = in;

        while (!remain.empty()) {
            auto n = TEMP_FAILURE_RETRY(
                    write(stdin_pipe[1], remain.data(), remain.size()));
            if (n < 0) {
                return;
            }

            remain.remove_prefix(static_cast<size_t>(n));
        }

        safely_close(stdin_pipe[1]);
    });

    auto join_stdin_thread = finally([&] {
        safely_close(stdin_pipe[1]);
        stdin_thread.join();
    });

    int stdout_pipe[2];
    if (pipe2(stdout_pipe, O_CLOEXEC) < 0) {
        return ec_from_errno();
    }

    auto close_stdout_pipe = finally([&] {
        safely_close(stdout_pipe[0]);
        safely_close(stdout_pipe[1]);
    });

    int pid = fork();
    if (pid < 0) {
        return ec_from_errno();
    } else if (pid == 0) {
        if (dup2(stdout_pipe[1], STDOUT_FILENO) < 0
                || dup2(stdin_pipe[0], STDIN_FILENO) < 0) {
            _exit(127);
        }

        std::vector<const char *> argv;
        for (auto const &arg : args) {
            argv.push_back(arg.c_str());
        }
        argv.push_back(nullptr);

        execvp(argv[0], const_cast<char * const *>(argv.data()));
        _exit(127);
    }

    auto kill_process = finally([&] {
        kill(pid, SIGKILL);
        TEMP_FAILURE_RETRY(waitpid(pid, nullptr, 0));
    });

    if (safely_close(stdout_pipe[1]) < 0 || safely_close(stdin_pipe[0]) < 0) {
        return ec_from_errno();
    }

    out.clear();
    char buf[1024];
    ssize_t n;

    while ((n = TEMP_FAILURE_RETRY(
            read(stdout_pipe[0], buf, sizeof(buf)))) > 0) {
        out.append(buf, static_cast<size_t>(n));
    }

    if (n < 0) {
        return ec_from_errno();
    }

    kill_process.dismiss();

    int status;
    TEMP_FAILURE_RETRY(waitpid(pid, &status, 0));

    if (!WIFEXITED(status)) {
        return std::errc::io_error;
    }

    return WEXITSTATUS(status);
}

#endif
