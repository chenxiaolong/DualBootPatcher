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

#include "mbsystrace/tracer.h"

#include <vector>

#include <climits>
#include <cstdio>
#include <cstring>

#include <signal.h>
#include <sys/ptrace.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>

#include "mbcommon/error_code.h"
#include "mbcommon/finally.h"
#include "mbcommon/integer.h"

#include "mbsystrace/procfs_p.h"
#include "mbsystrace/syscalls.h"
#include "mbsystrace/tracee.h"

/*!
 * \namespace mb::systrace
 *
 * \brief libmbsystrace namespace
 */

namespace mb::systrace
{

using namespace detail;

/*!
 * \class Tracer
 *
 * \brief Main class for tracing processes
 *
 * This is the main class for tracing processes and hooking their events. To
 * trace a process, use either fork() to spawn a new process or attach() to
 * attach to an existing process. Once all desired processes are attached, call
 * execute() to begin the event loop.
 *
 * Example:
 *
 * \code{.cpp}
 * Hooks hooks;
 *
 * hooks.syscall_entry = [](auto tracee, auto &info) -> SysCallEntryAction {
 *     printf("[%d] Entering syscall: %s\n", tracee->tid, info.syscall.name());
 *     return action::Default{};
 * };
 *
 * hooks.syscall_exit = [](auto tracee, auto &info) -> SysCallExitAction {
 *     printf("[%d] Exiting syscall: %s\n", tracee->tid, info.syscall.name());
 *     return action::Default{};
 * };
 *
 * Tracer tracer;
 *
 * if (auto r = tracer.fork([&] { printf("Hello, world!\n"); }); !r) {
 *     fprintf(stderr, "Failed to create process: %s\n",
 *             r.error().message().c_str());
 *     ...
 * }
 *
 * // pid_t tid = ...
 *
 * if (auto r = tracer.attach(tid); !r) {
 *     fprintf(stderr, "Failed to attach to TID %d: %s\n",
 *             tid, r.error().message().c_str());
 *     ...
 * }
 *
 * if (auto r = tracer.execute(hooks); !r) {
 *     fprintf(stderr, "Error in tracer: %s\n",
 *             r.error().message().c_str());
 *     ...
 * }
 * \endcode
 */

/*!
 * \brief Construct Tracer instance
 */
Tracer::Tracer()
    : m_should_stop(false)
{
}

/*!
 * \brief Destruct Tracer instance
 *
 * All tracees will be detached. If a tracee was created with fork(), then its
 * thread group will be killed.
 */
Tracer::~Tracer()
{
    // Remove tracees explicitly so that owned TGIDs are properly killed
    while (!m_tracees.empty()) {
        remove_child(m_tracees.begin()->first);
    }

    assert(m_owned_tgids.empty());
}

/*!
 * \brief Fork a new traced process
 *
 * \note The child process will be killed after all if its threads are detached.
 *       Also, on kernel 3.8 and newer, if the tracer process dies, the kernel
 *       will kill the tracee.
 *
 * \param child Function to exit in child process. If the function does not
 *              exit itself, then the child process will exit with status code
 *              255.
 * \param flags Flags to control how the child process is attached.
 *
 * \return If successful, returns the Tracee instance of the new tracee.
 *         Otherwise, returns a specific error code.
 */
oc::result<Tracee *> Tracer::fork(std::function<void()> child, Flags flags)
{
    SeizeFlags seize_flags = SeizeFlag::TryKillOnExit;

    if (flags & Flag::TraceChildren) {
        seize_flags |= SeizeFlag::TraceChildren;
    }

    pid_t pid = ::fork();

    if (pid == 0) {
        // Loop indefinitely until the parent attaches and changes the behavior
        // of the syscall. This is better than raise(SIGSTOP) and having the
        // the parent wait for a SIGSTOP because the parent would have no way of
        // determining which process sent the SIGSTOP.
        while (syscall(SYS_ppoll, nullptr, 0, nullptr, nullptr) != 0);

        child();
        _exit(255);
    } else if (pid < 0) {
        return ec_from_errno();
    }

    auto kill_child = finally([&] {
        kill(pid, SIGKILL);

        int status;
        while (waitpid(pid, &status, 0) < 0 && errno == EINTR);
    });

    m_tracees[pid] = std::make_unique<Tracee>(this, pid, pid);

    auto &tracee = m_tracees[pid];

    auto remove_tracee = finally([&] {
        m_tracees.erase(pid);
    });

    OUTCOME_TRYV(tracee->seize(seize_flags));

    // Force a stop to interrupt pause()
    OUTCOME_TRYV(tracee->interrupt());

    // Modify behavior of pause to allow child to continue
    Hooks hooks;
    hooks.syscall_entry = [](auto *, auto &info) -> SysCallEntryAction {
        if (strcmp(info.syscall.name(), "ppoll") == 0) {
            return action::SuppressSysCall{0};
        }
        return action::Default{};
    };
    hooks.syscall_exit = [this](auto *, auto &info) -> SysCallExitAction {
        if (strcmp(info.syscall.name(), "ppoll") == 0) {
            stop_after_hook();
        }
        return action::Default{};
    };
    OUTCOME_TRYV(execute(hooks, tracee->tid));

    kill_child.dismiss();
    remove_tracee.dismiss();

    // An external process could have killed the process before ppoll()
    if (auto t = find_tracee(pid)) {
        m_owned_tgids.insert(pid);
        return oc::success(t);
    } else {
        return std::errc::no_such_process;
    }
}

/*!
 * \brief Attach to an existing process
 *
 * \note In order to trigger a ptrace stop, this function will interrupt the
 *       process, which may result in an InterruptStopEvent on the next call to
 *       next_event() that waits for \p tid. If another event occurs at the same
 *       time, the other event will be reported instead.
 *
 * \param tid Thread ID to attach to
 * \param flags Flags to control how the process is attached.
 *
 * \return If successful, returns list of Tracee instances for each attached
 *         tracee. If the process is not found, returns
 *         std::errc::no_such_process. Otherwise, returns a specific error code.
 */
oc::result<std::vector<Tracee *>> Tracer::attach(pid_t tid, Flags flags)
{
    SeizeFlags seize_flags;

    if (flags & Flag::TraceChildren) {
        seize_flags |= SeizeFlag::TraceChildren;
    }

    std::vector<std::unique_ptr<Tracee>> new_tracees;
    pid_t tgid;

    {
        auto tracee = std::make_unique<Tracee>(this, -1, tid);
        OUTCOME_TRYV(tracee->seize(seize_flags));
        tgid = tracee->tgid;
        new_tracees.push_back(std::move(tracee));
    }

    // Add tracee's threads
    OUTCOME_TRYV(for_each_tid(tid, [&](pid_t new_tid) -> oc::result<bool> {
        if (new_tid == tid) {
            // Already attached
            return true;
        }

        auto tracee = std::make_unique<Tracee>(this, tgid, new_tid);
        if (auto r = tracee->seize(seize_flags); !r) {
            if (r.error() == std::errc::no_such_process) {
                return false;
            }
            return r.as_failure();
        }

        new_tracees.push_back(std::move(tracee));

        return true;
    }, true));

    // Interrupt so that the event handler will trigger a PTRACE_SYSCALL
    for (auto &tracee : new_tracees) {
        OUTCOME_TRYV(tracee->interrupt());
    }

    std::vector<Tracee *> result;

    for (auto &tracee : new_tracees) {
        result.push_back(tracee.get());
        m_tracees[tracee->tid] = std::move(tracee);
    }

    return oc::success(std::move(result));
}

/*!
 * \brief Add a new tracee.
 *
 * Called when a process forks a new process or thread.
 *
 * \param tgid Thread group ID
 * \param tid Thread ID
 *
 * \return Returns pointer to the new Tracee
 */
oc::result<Tracee *> Tracer::add_child(pid_t tgid, pid_t tid)
{
    m_tracees[tid] = std::make_unique<Tracee>(this, tgid, tid);
    m_tracees[tid]->set_state(TraceeState::Executing);

    return oc::success(m_tracees[tid].get());
}

/*!
 * \brief Detach and remove tracee
 *
 * If the thread group is owned (ie. created by fork()) and the tracee being
 * detached is the last tracee in the thread group, then the thread group will
 * be killed.
 */
bool Tracer::remove_child(pid_t tid)
{
    pid_t tgid;

    if (auto it = m_tracees.find(tid); it != m_tracees.end()) {
        tgid = it->second->tgid;
        m_tracees.erase(it);
    } else {
        return false;
    }

    // Make best effort to kill owned tgid if needed
    if (auto it = m_owned_tgids.find(tgid); it != m_owned_tgids.end()) {
        auto matching_tgid = std::find_if(
            m_tracees.begin(), m_tracees.end(),
            [&](auto const &item) {
                return item.second->tgid == tgid;
            }
        );

        // Kill only if there are no more tracees in the thread group
        if (matching_tgid == m_tracees.end()) {
            if (kill(tgid, SIGKILL) == 0) {
                while (true) {
                    auto e = next_event(tgid);

                    if (e && !std::holds_alternative<RetryEvent>(e.value())) {
                        break;
                    }
                }
            }

            m_owned_tgids.erase(it);
        }
    }

    return true;
}

}
