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

#pragma once

#include <variant>

#include <sys/signal.h>

#include "mbcommon/outcome.h"

#include "mbsystrace/arch.h"
#include "mbsystrace/types.h"

namespace mb::systrace::detail
{

/*!
 * \file event_p.h
 *
 * \brief High-level abstraction for various ptrace events
 *
 * Unless otherwise specified, the following terms should be interpreted as
 * specified:
 *
 * * [`tid`, "thread"]: A thread within a thread group
 * * [`tgid`, "thread group"]: A thread group (a.k.a. what is generally referred
 *   to as a process)
 * * [`pid`, "process"]: A kernel process (i.e. a thread OR thread group).
 *   Threads are just processes created via the clone() syscall and are not
 *   distinct from normal processes from the kernel's point of view.
 *
 * Certain events can be emitted for untraced child processes, tracees, or both.
 * This will be clearly stated in the description of the event type.
 */

/*!
 * \brief Event emitted when a child process or tracee is not found
 *
 * This event occurs when `waitpid()` returns `ECHILD`. Depending on the
 * \p pid_spec that was provided to next_event(), this could mean one of the
 * following.
 *
 * * If `pid < -1`, then there are no child processes or tracees whose process
 *   group ID equals `|pid|`.
 * * If `pid == -1`, then there are no child processes or tracees.
 * * If `pid == 0`, then there are no child processes or tracees whose process
 *   group ID matches that of the tracer.
 * * If `pid > 0`, then `pid` does not exist or is not a child or tracee of the
 *   tracer.
 *
 * This event can also occur if the tracer ignores `SIGCHLD` signals by setting
 * the signal action for `SIGCHLD` to `SIG_IGN`.
 *
 * \sa The `waitpid()` manpage for more details on `ECHILD` return values
 */
struct NoChildrenEvent
{
};

/*!
 * \brief Event emitted when next_event() should be called again
 *
 * This event is emitted when `waitpid()` returns `EINTR` or when a process is
 * killed between `waitpid()` and a `ptrace()` call needed for handling a stop
 * event. In the latter case, the process death will be reported in a subsequent
 * event.
 *
 * Upon receiving this event, the tracer should reattempt a call to
 * next_event().
 *
 * \sa
 *   * The `waitpid()` manpage for details on the `EINTR` return value
 *   * The `ptrace()` manpage for details on the `ESRCH` return value
 */
struct RetryEvent
{
};

/*!
 * \brief Event emitted when a child process or tracee exits normally
 *
 * Thie event is emitted when `waitpid()` returns a status such that
 * `WIFEXITED(status)` is true, indicating that a process exited normally via
 * the `_exit()` or `exit_group()` syscalls.
 *
 * Upon receiving this event, the tracer should remove all references to this
 * process as no further events for this process will be emitted.
 */
struct ProcessExitEvent
{
    //! Child process or tracee
    pid_t pid;
    //! Raw waitpid status
    int status;
    //! Exit code
    int exit_code;
};

/*!
 * \brief Event emitted when a child process or tracee is killed by a signal
 *
 * This event is emitted when `waitpid()` returns a status such that
 * `WIFSIGNALED(status)` is true, indicating that a process was killed by a
 * signal.
 *
 * Upon receiving this event, the tracer should remove all references to this
 * process as no further events for this process will be emitted.
 */
struct ProcessDeathEvent
{
    //! Child process or tracee
    pid_t pid;
    //! Raw waitpid status
    int status;
    //! Signal that killed the process
    int exit_signal;
};

/*!
 * \brief Event emitted when a tracee is about to enter or exit a syscall
 *
 * This event is emitted when `waitpid()` returns a status such that
 * `WIFSTOPPED(status)` is true and `status >> 8` is `SIGTRAP | 0x80`,
 * indicating that a tracee is about to enter or exit a syscall. The tracee will
 * be in a stopped state until it is explicitly continued via
 * Tracee::continue_exec().
 *
 * When the tracee is in the syscall-stop state, the tracer may modify the
 * syscall number, arguments, or return value to change how the syscall executes
 * or what it reports back to the original caller. However, on some
 * architectures, the same registers are used for the arguments and return
 * value. Thus, during the syscall entry stop, only the syscall number and
 * arguments should be modified. Similarly, in the syscall exit stop, only the
 * syscall number and return value should be modified.
 *
 * Note that the ptrace API does not distinguish between syscall-entry and
 * syscall-exit events. The tracer must keep track of this state for each
 * tracee.
 *
 * In normal cases, a syscall-entry event will be followed by a syscall-exit
 * event. However, there are exceptions to this. For example, if the tracee dies
 * before completing the syscall, a syscall-exit event will not be emitted. If
 * seccomp is used, it is possible for there to be a syscall-exit event, but not
 * a syscall-entry event.
 *
 * Upon receiving this event, the tracer must continue the execution of the
 * tracee via Tracee::continue_exec(). Otherwise, it will remain in the
 * syscall-stop state until it is detached or killed.
 *
 * \sa The `ptrace()` manpage for details on when syscall-entry/exit events
 *     occur
 */
struct SysCallStopEvent
{
    //! Tracee
    pid_t tid;
    //! Register values at the time of the syscall-stop
    ArchRegs regs;
};

/*!
 * \brief Event emitted when a signal is about to be delivered to a tracee
 *
 * This event is emitted when the following conditions are true:
 *
 * * `waitpid()` returns a status such that `WIFSTOPPED(status)` is true
 * * The signal is not one of `SIGSTOP`, `SIGTSTP`, `SIGTTIN`, `SIGTTOU` **OR**
 *   the `PTRACE_GETSIGINFO` ptrace call does not fail with `EINVAL`
 *
 * This indicates that a signal is about to be delivered to the tracee.
 *
 * Upon receiving this event, the tracer must either forward the signal to the
 * tracee or suppress the signal via Tracee::continue_exec(). Otherwise, it will
 * remain in the signal-delivery-stop state until it is detached or killed.
 */
struct SignalDeliveryStopEvent
{
    //! Tracee
    pid_t tid;
    //! Signal to be delivered
    int signal;
};

/*!
 * \brief Event emitted when a tracee is about to enter a group stop
 *
 * This event is emitted when the following conditions are true:
 *
 * * `waitpid()` returns a status such that `WIFSTOPPED(status)` is true
 * * The signal is one of `SIGSTOP`, `SIGTSTP`, `SIGTTIN`, `SIGTTOU`
 * * `status >> 8` is `PTRACE_EVENT_STOP`
 *
 * This indicates that a tracee is about to enter group-stop.
 *
 * A group-stop occurs when a (potentially multithreaded) tracee process
 * receives a stopping signal. Prior to receiving a group-stop event, one of the
 * tracees for a thread in the process will receive a signal-delivery-stop.
 * After the stopping signal is injected, all tracees in the process will
 * individually receive a group-stop event.
 *
 * Upon receiving this event, the tracer must perform one of the following
 * actions.
 *
 * * The tracer may continue the execution of the tracee via
 *   Tracee::continue_exec(). This causes the stop signal to be effectively
 *   ignored.
 * * The tracer may put the tracee into a "listen" state via
 *   Tracee::continue_stopped(). This causes the tracee to be put into a state
 *   where it is not executing, but new events will still be delivered to the
 *   tracer via `waitpid()`.
 *
 * If the tracer does not perform one of these actions before the next call to
 * `waitpid()` (and thus, next_event()), the tracee will be stuck in a state
 * where future `SIGCONT` signals will not be reported to the tracer. The tracee
 * would remain stopped until it is detached or killed.
 *
 * \sa The `ptrace()` manpage for details on when group-stop events occur
 */
struct GroupStopEvent
{
    //! Tracee
    pid_t tid;
    //! Signal to be delivered
    int signal;
};

/*!
 * \brief Event emitted when `PTRACE_INTERRUPT` is used, tracee receives
 *        `SIGCONT`, or newly attached child of tracee begins executing
 *
 * This event is emitted when `waitpid()` returns a status such that
 * `WIFSTOPPED(status)` is true and `status >> 8` is
 * `SIGTRAP | (PTRACE_EVENT_STOP << 8)`.
 *
 * This event occurs in the following three cases:
 *
 * 1. When the event is caused by `PTRACE_INTERRUPT`, the tracee was executing
 *    user space code prior to the stop. If `PTRACE_INTERRUPT` was called and
 *    the tracee was executing a syscall, a syscall-exit stop would occur
 *    instead and the syscall would be restarted when the tracee is continued.
 *    If another ptrace stop occurs at the same time as a `PTRACE_INTERRUPT`
 *    call, the non-`PTRACE_INTERRUPT` event will occur.
 * 2. This event can occur by a tracee's receipt of the `SIGCONT` signal. This
 *    behavior is not documented in the ptrace manpage. It was introduced in
 *    version 3.1 of the kernel, specifically by commit
 *    [`fb1d910c178ba0c5bc32d3e5a9e82e05b7aad3cd`][1].
 * 3. This event will occur if a tracee creates a child process via `clone()`,
 *    `fork()`, or `vfork()`, the tracer is configured to trace these three
 *    syscalls via `PTRACE_O_TRACE{CLONE,FORK,VFORK}`, and the newly attached
 *    child process begins executing. This scenario is not guaranteed to happen
 *    if the child process exits quickly.
 *
 * It is impossible to distinguish among these three cases due to limitations of
 * the ptrace API.
 *
 * Upon receiving this event, the tracer must continue the execution of the
 * tracee via Tracee::continue_exec(). Otherwise, it will remain in the
 * interrupt-stop state until it is detached or killed.
 *
 * \sa
 *   * The `ptrace()` manpage for details on `PTRACE_INTERRUPT` events
 *   * Kernel commit [`fb1d910c178ba0c5bc32d3e5a9e82e05b7aad3cd`][1] for details
 *     on `SIGCONT`-triggered events
 *
 * [1]: https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=fb1d910c178ba0c5bc32d3e5a9e82e05b7aad3cd
 */
struct InterruptStopEvent
{
    //! Tracee
    pid_t tid;
    //! Signal to be delivered
    int signal;
};

/*!
 * \brief Event emitted when a new process is created by a tracee
 *
 * This event is emitted when `waitpid()` returns a status such that
 * `WIFSTOPPED(status)` is true and `status >> 8` is one of the following:
 *
 * * `SIGTRAP | (PTRACE_EVENT_CLONE << 8)`
 * * `SIGTRAP | (PTRACE_EVENT_FORK << 8)`
 * * `SIGTRAP | (PTRACE_EVENT_VFORK << 8)`
 *
 * This indicates that the tracee has created a new process via the `clone()`,
 * `fork()`, or `vfork()` syscalls.
 *
 * Upon receiving this event, the tracer must keep track of the new process or
 * detach it since the process will start off in an attached state. If not
 * detached, an additional GroupStopEvent will be emitted for the new tracee.
 */
struct NewProcessStopEvent
{
    //! Tracee that called `clone()`, `fork()`, or `vfork()`
    pid_t tid;
    //! New process that is created
    pid_t new_tid;
};

/*!
 * \brief Event emitted when a tracee is about to call `execve()`
 *
 * This event is emitted when `waitpid()` returns a status such that
 * `WIFSTOPPED(status)` is true and `status >> 8` is
 * `SIGTRAP | (PTRACE_EVENT_EXEC << 8)`, indicating that the tracee is about to
 * execute `execve()`.
 *
 * Handling this event is pretty straight forward for a single-threaded process,
 * but the sequence of events can be confusing for multi-threaded processes.
 * When a thread in a multi-threaded process calls `execve()`, the kernel will
 * destroy all other threads and change the thread ID of the calling thread to
 * the thread group ID (i.e. process ID).
 *
 * The following sequence of events will occur.
 *
 * * An ExitStopEvent is emitted for all threads besides the `execve`-calling
 *   thread
 * * A ProcessExitEvent is emitted for all threads besides the `execve`-calling
 *   thread and the thread group leader (the thread in which `tid` == `pid`)
 * * The `execve`-calling thread changes its thread ID to the process ID during
 *   the `execve()` call
 * * An ExecveStopEvent is emitted for the thread group leader
 *
 * At the time that ExecveStopEvent is emitted, only the execve-calling thread
 * and thread group leader are alive. Note that there is no event that indicates
 * the death of the original thread group leader.
 *
 * Upon receiving this event, the tracer must discard any references to threads
 * in the thread group besides the thread group leader (threads in which
 * `tid` != `tgid`). If the tracer was previously tracing the `execve`-calling
 * thread, but not the thread group leader, it is now responsible for tracing
 * the thread group leader.
 *
 * After removing references to threads that aren't the thread group leader,
 * the tracer must continue the execution of the tracee via
 * Tracee::continue_exec(). Otherwise, it will remain in the execve-stop state
 * until it is detached or killed.
 *
 * \sa The `ptrace()` manpage for details on the `execve()` behavior
 */
struct ExecveStopEvent
{
    /*!
     * \brief New TID of process
     *
     * This will be the TGID of the process that called `execve()`.
     */
    pid_t tid;
    /*!
     * \brief Original process (thread) ID before `execve()`
     *
     * \note The value is inaccurate unless the system is running kernel 3.0+.
     */
    pid_t orig_tid;
};

/*!
 * \brief Event emitted when a tracee is about to exit
 *
 * This event is emitted when `waitpid()` returns a status such that
 * `WIFSTOPPED(status)` is true and `status >> 8` is
 * `SIGTRAP | (PTRACE_EVENT_EXIT << 8)`, indicating that a tracee is about to
 * exit normally. The tracee will be in a stopped state until it is explicitly
 * continued via Tracee::continue_exec().
 *
 * When the tracee is in the exit-stop state, the tracer may read the registers
 * of the tracee, but there is no way to prevent the exit from happening.
 *
 * Upon receiving this event, the tracer must continue the execution of the
 * tracee via Tracee::continue_exec() to complete the exiting of the tracee.
 * A ProcessExitEvent or ProcessDeathEvent will be emitted for the tracee once
 * the exit completes or the process is killed.
 */
struct ExitStopEvent
{
    //! Tracee
    pid_t tid;
    //! Status (same as `waitpid()` status)
    int status;
};

/*!
 * \brief Tagged union of all possible events
 */
using ProcessEvent = std::variant<
    NoChildrenEvent,
    RetryEvent,
    ProcessExitEvent,
    ProcessDeathEvent,
    SysCallStopEvent,
    SignalDeliveryStopEvent,
    GroupStopEvent,
    InterruptStopEvent,
    NewProcessStopEvent,
    ExecveStopEvent,
    ExitStopEvent
>;

oc::result<ProcessEvent> next_event(pid_t pid_spec) noexcept;

}
