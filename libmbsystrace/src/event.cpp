/*
 * Copyright (C) 2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbsystrace/event_p.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <sys/syscall.h>
#include <sys/ptrace.h>
#include <sys/wait.h>

#include "mbcommon/common.h"
#include "mbcommon/error_code.h"

namespace mb::systrace::detail
{

/*!
 * \brief Return retry event if error code is equivalent to ESRCH
 *
 * \param ec Error code
 *
 * \return RetryEvent if \p ec is equivalent to ESRCH. Otherwise, returns the
 *         error code.
 */
static oc::result<ProcessEvent> retry_or_failure(std::error_code ec)
{
    if (ec == std::errc::no_such_process) {
        return RetryEvent{};
    } else {
        return ec;
    }
}

/*!
 * \brief Wait for next event from child processes and tracees
 *
 * This function will wait for the next event using the waitpid/ptrace APIs.
 *
 * The caller must respond appropriately to the events as described in each
 * event's documentation.
 *
 * \note This function will only wait on children of the calling thread. Thus,
 *       multiple Tracer instances can be used as long as they're running on
 *       different threads.
 *
 * \param pid_spec Same meaning as the \p pid parameter to waitpid().
 *   * If `< -1`, wait for any child process or tracee whose process group ID is
 *     `|pid|`
 *   * If `== -1`, wait for any child process or tracee
 *   * If `== 0`, wait for any child process or tracee whose process group ID
 *     matches that of the calling process
 *   * If `> 0`, wait for the child process or tracee whose process ID is equal
 *     to \p pid_spec
 *
 * \return Returns a ProcessEvent if an event is emitted and information about
 *         the event is successfully queried. Otherwise, returns an error code.
 *         Note that `EINTR` and `ECHILD` are not considered errors and an
 *         appropriate event will be returned.
 */
oc::result<ProcessEvent> next_event(pid_t pid_spec) noexcept
{
    int status;
    pid_t pid = waitpid(pid_spec, &status, __WALL | __WNOTHREAD);
    if (pid == -1) {
        if (errno == EINTR) {
            return RetryEvent{};
        } else if (errno == ECHILD) {
            return NoChildrenEvent{};
        } else {
            return ec_from_errno();
        }
    }

    if (WIFEXITED(status)) {
        return ProcessExitEvent{pid, status, WEXITSTATUS(status)};
    } else if (WIFSIGNALED(status)) {
        return ProcessDeathEvent{pid, status, WTERMSIG(status)};
    } else if (WIFSTOPPED(status)) {
        int ptrace_event = status >> 16;
        int signal = WSTOPSIG(status);

        switch (ptrace_event) {
            case PTRACE_EVENT_CLONE:
            case PTRACE_EVENT_FORK:
            case PTRACE_EVENT_VFORK: {
                unsigned long new_pid;
                if (ptrace(PTRACE_GETEVENTMSG, pid, nullptr, &new_pid) != 0) {
                    return retry_or_failure(ec_from_errno());
                }

                return NewProcessStopEvent{pid, static_cast<pid_t>(new_pid)};
            }

            case PTRACE_EVENT_EXEC: {
                unsigned long orig_tid;
                if (ptrace(PTRACE_GETEVENTMSG, pid, nullptr, &orig_tid) != 0) {
                    return retry_or_failure(ec_from_errno());
                }

                return ExecveStopEvent{pid, static_cast<pid_t>(orig_tid)};
            }

            case PTRACE_EVENT_EXIT: {
                unsigned long code;
                if (ptrace(PTRACE_GETEVENTMSG, pid, nullptr, &code) != 0) {
                    return retry_or_failure(ec_from_errno());
                }

                return ExitStopEvent{pid, static_cast<int>(code)};
            }

            case PTRACE_EVENT_STOP: {
                if (signal == SIGSTOP || signal == SIGTSTP
                        || signal == SIGTTIN || signal == SIGTTOU) {
                    return GroupStopEvent{pid, signal};
                } else {
                    // We get spurious SIGTRAP signals when SIGCONT'ing a
                    // process. rr seem to be running into this as well:
                    // https://github.com/mozilla/rr/issues/2095
                    // strace handles PTRACE_EVENT_STOP + non-group-stop signal
                    // by restarting the process with PTRACE_SYSCALL:
                    // https://github.com/strace/strace/blob/b1e1eb7731e50900bb4591a3a71b96ab37e106a8/strace.c#L2360
                    // https://github.com/strace/strace/blob/b1e1eb7731e50900bb4591a3a71b96ab37e106a8/strace.c#L2408-L2409
                    return InterruptStopEvent{pid, signal};
                }
            }

            default: {
                if (signal == (SIGTRAP | 0x80)) {
                    auto regs = read_regs(pid);
                    if (!regs) {
                        return retry_or_failure(regs.error());
                    }

                    return SysCallStopEvent{pid, std::move(regs.value())};
                } else {
                    return SignalDeliveryStopEvent{pid, signal};
                }
            }
        }
    } else {
        MB_UNREACHABLE("Invalid waitpid status: 0x%x", status);
    }
}

}
