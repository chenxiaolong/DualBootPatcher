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

#include <functional>
#include <variant>

#include <signal.h>

#include "mbsystrace/syscalls.h"
#include "mbsystrace/types.h"


/*!
 * \file hooks.h
 *
 * \brief Contains all classes related to Tracer hooks
 */

namespace mb::systrace
{

class Tracee;

/*!
 * \brief Injection status of system call
 */
enum class SysCallStatus
{
    //! Normal system call invoked by the process
    Normal,
    //! System call injected by the tracer
    Injected,
    //! Normal system call that is repeated due to a system call injection in
    //! the syscall-entry hook
    Repeated,
};

/*!
 * \brief Struct holding the syscall number and arguments during the
 *        syscall-entry stop
 */
struct SysCallEntryInfo
{
    //! System call number and ABI
    SysCall syscall;
    //! System call arguments
    SysCallArgs args;
    //! System call injection status
    SysCallStatus status;
};

/*!
 * \brief Struct holding the syscall number and return value during the
 *        syscall-exit stop
 */
struct SysCallExitInfo
{
    //! System call number and ABI
    SysCall syscall;
    //! System call return value
    SysCallRet ret;
    //! System call injection status
    SysCallStatus status;
};

namespace action
{

//! Take default action for hook
struct Default {};
//! Trace child
struct AddTracee {};
//! Detach child
struct DetachTracee {};
//! Kill tracee
struct KillTracee
{
    //! Send this signal to the tracee
    int signal;
};
//! No action
struct NoAction {};
//! Requeue event
struct RequeueEvent {};
//! Forward signal to process
struct ForwardSignal {};
//! Suppress signal
struct SuppressSignal {};
//! Continue execution of tracee
struct ContinueExec {};
//! Continue in stopped state
struct ContinueStopped {};
//! Suppress system call
struct SuppressSysCall
{
    //! Set the syscall return value to this value
    SysCallRet ret;
};

}

//! Possible return values for TraceeExitHook handler
using TraceeExitAction = std::variant<
    action::Default,
    action::NoAction,
    action::RequeueEvent
>;

//! Possible return values for TraceeDeathHook handler
using TraceeDeathAction = std::variant<
    action::Default,
    action::NoAction,
    action::RequeueEvent
>;

//! Possible return values for TraceeSignalHook handler
using TraceeSignalAction = std::variant<
    action::Default,
    action::ForwardSignal,
    action::SuppressSignal,
    action::DetachTracee,
    action::KillTracee
>;

//! Possible return values for GroupStopHook handler
using GroupStopAction = std::variant<
    action::Default,
    action::ContinueStopped,
    action::DetachTracee,
    action::KillTracee
>;

//! Possible return values for InterruptStopHook handler
using InterruptStopAction = std::variant<
    action::Default,
    action::ContinueExec,
    action::DetachTracee,
    action::KillTracee
>;

//! Possible return values for SysCallEntryHook handler
using SysCallEntryAction = std::variant<
    action::Default,
    action::ContinueExec,
    action::SuppressSysCall,
    action::DetachTracee,
    action::KillTracee,
    action::NoAction
>;

//! Possible return values for SysCallExitHook handler
using SysCallExitAction = std::variant<
    action::Default,
    action::ContinueExec,
    action::DetachTracee,
    action::KillTracee,
    action::NoAction
>;

/*!
 * \brief Hook called when a new tracee is added
 *
 * This hook is called when an existing tracee calls `clone()`, `fork()`, or
 * `vfork()` and the new process is going to be traced.
 *
 * This hook will also be called if the leader of a thread group is not attached
 * and another thread calls execve(). The other thread will become the new
 * thread group leader and will be traced by the tracer.
 *
 * \param new_tracee New tracee
 */
using NewTraceeHook = void(Tracee *new_tracee);

/*!
 * \brief Hook called when a tracee exits
 *
 * This hook is called when a tracee exits normally with the `_exit()` or
 * `exit_group()` syscalls. The handler should remove any references to \p tid.
 *
 * When a thread in a thread group calls `execve()`, this hook is called for
 * every traced thread that is not the thread group leader or the
 * `execve()`-calling thread. If the `execve()`-calling thread is not the thread
 * group leader, then references to it should be removed in the
 * TraceeDisappearHook handler.
 *
 * \sa TraceeDisappearHook
 *
 * \param tid TID of tracee that exited normally
 * \param exit_code Exit code of the process
 *
 * \return
 *   * action::Default: Same as action::NoAction
 *   * action::NoAction: Drop reference to tracee without further actions
 *   * action::RequeueEvent: (Implementation detail. Do not use.)
 */
using TraceeExitHook = TraceeExitAction(pid_t tid, int exit_code);

/*!
 * \brief Hook called when a tracee is killed by a signal
 *
 * This hook is called when a tracee is killed by a signal. The handler should
 * remove any references to \p tid.
 *
 * \param tid TID of tracee that was killed
 * \param exit_signal Signal that killed the process
 *
 * \return
 *   * action::Default: Same as action::NoAction
 *   * action::NoAction: Drop reference to tracee without further actions
 *   * action::RequeueEvent: (Implementation detail. Do not use.)
 */
using TraceeDeathHook = TraceeDeathAction(pid_t tid, int exit_signal);

/*!
 * \brief Hook called when a tracee disappears due to an `execve()` call
 *
 * This hook is called when a tracee that is not the thread group leader invokes
 * the `execve()` syscall. It is called after TraceeExitHook has been called for
 * every tracee in the thread group that is not the thread group leader or the
 * `execve()`-calling thread. The handler should remove any references to
 * \p tid.
 *
 * After this hook returns, the only thread left in the thread group is the
 * thread group leader.
 *
 * \sa TraceeExitHook
 *
 * \param tid TID of tracee that disappears due to `execve()`
 */
using TraceeDisappearHook = void(pid_t tid);

/*!
 * \brief Hook called when a signal is delivered to a tracee
 *
 * This hook is called when a normal non-ptrace signal is delivered to a tracee.
 * The handler should forward the signal or suppress it. The tracee will be in a
 * stopped state until it is continued.
 *
 * \param tracee Tracee receiving the signal
 * \param signal Signal to be delivered
 *
 * \return
 *   * action::Default: Same as action::ForwardSignal
 *   * action::ForwardSignal: Forward the signal to the tracee
 *   * action::SuppressSignal: Suppress the delivery of signal to the tracee
 *   * action::DetachTracee: Detach the tracee
 *   * action::KillTracee: Kill the tracee with `SIGKILL`
 */
using TraceeSignalHook = TraceeSignalAction(Tracee *tracee, int signal);

/*!
 * \brief Hook called when a group stop occurs
 *
 * This hook is called for every tracee in a thread group when the thread group
 * is about to enter a group-stop (ie. when the tracee receives the `SIGSTOP`,
 * `SIGTSTP`, `SIGTTIN`, or `SIGTTOU` signals).  TraceeSignalHook will have been
 * called for a single random tracee in the thread group prior to this hook.
 *
 * \param tracee Tracee receiving group-stop signal
 * \param signal Group-stop signal
 *
 * \return
 *   * action::Default: Same as action::ContinueStopped
 *   * action::ContinueStopped: Put the tracee into a state where it is not
 *     executing user-space code, but new events, like `SIGCONT`, will still be
 *     delivered.
 *   * action::ContinueExec: Continue execution, effectively ignoring the group
 *     stop.
 *   * action::DetachTracee: Detach the tracee
 *   * action::KillTracee: Kill the tracee with `SIGKILL`
 */
using GroupStopHook = GroupStopAction(Tracee *tracee, int signal);

/*!
 * \brief Hook called when tracee is explicitly interrupted, `SIGCONT` is
 *        received, or newly attached child of tracee begins executing
 *
 * This hook is called when a tracee is explicitly interrupted with
 * Tracee::interrupt(), a tracee receives the `SIGCONT` signal while it is in
 * group-stop, or a newly attached child of a tracee is about to begin
 * execution. It is impossible to distinguish among the three scenarios due to
 * kernel limitations.
 *
 * \note This hook is not guaranteed to be called after a call to
 *       Tracee::interrupt() if another ptrace-stop is going to happen. For
 *       example, if Tracee::interrupt() is called during the execution of a
 *       syscall, the SysCallExitHook will serve as the "interruption".
 *
 * \param tracee Tracee being interrupted, receiving `SIGCONT` signal, or about
 *               to begin execution
 *
 * \return
 *   * action::Default: Same as action::ContinueExec
 *   * action::ContinueExec: Continue execution
 *   * action::DetachTracee: Detach the tracee
 *   * action::KillTracee: Kill the tracee with `SIGKILL`
 */
using InterruptStopHook = InterruptStopAction(Tracee *tracee);

/*!
 * \brief Hook called before a tracee makes a system call
 *
 * This hook is called when the tracee is about to make a system call. The
 * handler can modify the syscall number and arguments to change the execution
 * of the syscall. The tracee will be in a stopped state until it is continued.
 *
 * \param tracee Tracee executing the syscall
 * \param info Syscall number and arguments
 *
 * \return
 *   * action::Default: Same as action::ContinueExec
 *   * action::ContinueExec: Continue execution
 *   * action::SuppressSysCall: Suppress the execution of the syscall
 *   * action::DetachTracee: Detach the tracee
 *   * action::KillTracee: Kill the tracee with `SIGKILL`
 *   * action::NoAction: (Implementation detail. Do not use.)
 */
using SysCallEntryHook = SysCallEntryAction(Tracee *tracee,
                                            const SysCallEntryInfo &info);

/*!
 * \brief Hook called after a tracee makes a system call
 *
 * This hook is called immediately after the execution of a system call by the
 * tracee. The handler can modify the syscall number and return value. The
 * tracee will be in a stopped state until it is continued.
 *
 * \param tracee Tracee executing the syscall
 * \param info Syscall number and return value
 *
 * \return
 *   * action::Default: Same as action::ContinueExec
 *   * action::ContinueExec: Continue execution
 *   * action::DetachTracee: Detach the tracee
 *   * action::KillTracee: Kill the tracee with `SIGKILL`
 *   * action::NoAction: (Implementation detail. Do not use.)
 */
using SysCallExitHook = SysCallExitAction(Tracee *tracee,
                                          const SysCallExitInfo &info);

/*!
 * \brief Hook called when `waitpid()` returns a status for a non-tracee
 *
 * This handler is called if the tracer thread has untraced children and
 * `waitpid()` returns a status for an untraced child. The handler is not
 * required to perform any action.
 *
 * \param tid Process ID returned by `waitpid()`
 * \param status Status returned by `waitpid()`
 */
using UnknownChildHook = void(pid_t tid, int status);

/*!
 * \brief Simple struct holding all the handlers for hooks
 */
struct Hooks
{
    //! NewTraceeHook handler
    std::function<NewTraceeHook> new_tracee;
    //! TraceeDeathHook handler
    std::function<TraceeExitHook> tracee_exit;
    //! TraceeDeathHook handler
    std::function<TraceeDeathHook> tracee_death;
    //! TraceeDisappearHook handler
    std::function<TraceeDisappearHook> tracee_disappear;
    //! TraceeSignalHook handler
    std::function<TraceeSignalHook> tracee_signal;
    //! GroupStopHook handler
    std::function<GroupStopHook> group_stop;
    //! InterruptStopHook handler
    std::function<InterruptStopHook> interrupt_stop;
    //! SysCallEntryHook handler
    std::function<SysCallEntryHook> syscall_entry;
    //! SysCallExitHook handler
    std::function<SysCallExitHook> syscall_exit;
    //! UnknownChildHook handler
    std::function<UnknownChildHook> unknown_child;
};

}
