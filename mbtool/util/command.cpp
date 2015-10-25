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

#include "util/command.h"

#include <cerrno>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "autoclose/file.h"
#include "util/logging.h"

namespace mb
{
namespace util
{

static std::string list2string(const std::vector<std::string> &list)
{
    std::string output;
    for (std::size_t i = 0; i < list.size(); ++i) {
        output += list[i];
        if (i != list.size() - 1) {
            output += ", ";
        }
    }
    return output;
}

int run_shell_command(const std::string &command)
{
    // If /sbin/sh exists (eg. in recovery), then fork and run that. Otherwise,
    // just call system().

    LOGD("Running shell command: \"%s\"", command.c_str());

    struct stat sb;
    if (stat("/sbin/sh", &sb) == 0) {
        int status;
        pid_t pid;
        if ((pid = fork()) >= 0) {
            if (pid == 0) {
                execlp("/sbin/sh", "sh", "-c", command.c_str(), nullptr);
                _exit(127);
            } else {
                pid = waitpid(pid, &status, 0);
            }
        }

        return pid == -1 ? -1 : status;
    } else {
        return system(command.c_str());
    }
}

int run_command(const std::vector<std::string> &argv)
{
    return run_command2(argv, std::string(), nullptr, nullptr);
}

int run_command_cb(const std::vector<std::string> &argv,
                   OutputCb cb, void *data)
{
    return run_command2(argv, std::string(), cb, data);
}

int run_command_chroot(const std::string &dir,
                       const std::vector<std::string> &argv)
{
    return run_command2(argv, dir, nullptr, nullptr);
}

int run_command_chroot_cb(const std::string &dir,
                          const std::vector<std::string> &argv,
                          OutputCb cb, void *data)
{
    return run_command2(argv, dir, cb, data);
}

int run_command2(const std::vector<std::string> &argv,
                 const std::string &chroot_dir,
                 OutputCb cb, void *data)
{
    if (argv.empty()) {
        errno = EINVAL;
        return -1;
    }

    LOGD("Running command: [ %s ]", list2string(argv).c_str());

    std::vector<const char *> argv_c;
    for (const std::string &arg : argv) {
        argv_c.push_back(arg.c_str());
    }
    argv_c.push_back(nullptr);

    int status;
    pid_t pid;
    int stdio_fds[2];

    if (cb) {
        pipe(stdio_fds);
    }

    if ((pid = fork()) >= 0) {
        if (pid == 0) {
            if (cb) {
                close(stdio_fds[0]);
            }

            if (!chroot_dir.empty()) {
                if (chdir(chroot_dir.c_str()) < 0) {
                    LOGE("%s: Failed to chdir: %s",
                         chroot_dir.c_str(), strerror(errno));
                    _exit(EXIT_FAILURE);
                }
                if (chroot(chroot_dir.c_str()) < 0) {
                    LOGE("%s: Failed to chroot: %s",
                         chroot_dir.c_str(), strerror(errno));
                    _exit(EXIT_FAILURE);
                }
            }

            if (cb) {
                dup2(stdio_fds[1], STDOUT_FILENO);
                dup2(stdio_fds[1], STDERR_FILENO);
                close(stdio_fds[1]);
            }

            execvp(argv_c[0], const_cast<char * const *>(argv_c.data()));
            _exit(127);
        } else {
            if (cb) {
                close(stdio_fds[1]);

                int reader_status;
                pid_t reader_pid = fork();

                if (reader_pid < 0) {
                    LOGE("Failed to fork reader process");
                } else if (reader_pid == 0) {
                    // Read program output in child process (stdout, stderr)
                    char buf[1024];

                    autoclose::file fp(fdopen(stdio_fds[0], "rb"), fclose);

                    while (fgets(buf, sizeof(buf), fp.get())) {
                        cb(buf, data);
                    }

                    _exit(EXIT_SUCCESS);
                } else {
                    do {
                        if (waitpid(reader_pid, &reader_status, 0) < 0) {
                            LOGE("Failed to waitpid(): %s", strerror(errno));
                            break;
                        }
                    } while (!WIFEXITED(reader_status)
                            && !WIFSIGNALED(reader_status));
                }

                close(stdio_fds[0]);
            }

            do {
                if (waitpid(pid, &status, 0) < 0) {
                    return -1;
                }
            } while (!WIFEXITED(status) && !WIFSIGNALED(status));
        }
    }

    return pid == -1 ? -1 : status;
}

}
}
