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

#include "mbsystrace/tracee.h"

#include <cstdio>
#include <cstring>

#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/ptrace.h>

#include "mbcommon/error_code.h"

#include "mbsystrace/syscalls.h"
#include "mbsystrace/tracer.h"

namespace mb::systrace
{

/*!
 * \brief Continue execution until tracee enters or exits a syscall
 *
 * \return
 *   * Any error code that detail::next_event() returns
 *   * `std::errc::no_such_process` if the process exits or is killed
 */
oc::result<void> Tracee::proceed_until_syscall()
{
    bool executed = false;
    Hooks hooks;
    hooks.syscall_entry = [&](auto *, auto &) {
        executed = true;
        m_tracer->stop_after_hook();
        return action::NoAction{};
    };
    hooks.syscall_exit = [&](auto *, auto &) {
        executed = true;
        m_tracer->stop_after_hook();
        return action::NoAction{};
    };
    hooks.tracee_exit = [&](pid_t, int) {
        m_tracer->stop_after_hook();
        return action::RequeueEvent{};
    };
    hooks.tracee_death = [&](pid_t, int) {
        m_tracer->stop_after_hook();
        return action::RequeueEvent{};
    };

    OUTCOME_TRYV(m_tracer->execute(hooks, tid));

    if (!executed) {
        return std::errc::no_such_process;
    }

    return oc::success();
}

/*!
 * \brief Synchronously inject a system call into the process
 *
 * This function is a lightweight wrapper around the asynchronous syscall
 * injection functions to inject a syscall synchronously. No syscall-entry/exit
 * events will be produced for the injected syscall.
 *
 * \note If the syscall-entry/exit hook modifies the registers before calling
 *       this function, those changes will be lost. This is an intentional
 *       design choice to avoid needing to read/write the registers more times
 *       than necessary.
 *
 * \warning Because this function is synchronous, it consumes events for the
 *          tracee, including exit or death events. If this function returns
 *          `std::errc::process_not_found`, this Tracee will be deallocated.
 *
 * \pre state() must be TraceeState::PreSysCallStop or
 *      TraceeState::PostSysCallStop
 *
 * \param num System call number
 * \param args System call arguments
 *
 * \return The result of the system call if it is successful. Otherwise, returns
 *         an appropriate error code. If a failure occurs, the tracee's
 *         registers are left in an undefined state, which may cause the process
 *         to crash if it continues executing.
 */
oc::result<SysCallRet>
Tracee::inject_syscall(SysCallNum num, const SysCallArgs &args)
{
    switch (m_state) {
        case TraceeState::PreSysCallStop: {
            // In syscall-entry state
            OUTCOME_TRYV(inject_syscall_async(num, args));
            OUTCOME_TRYV(continue_exec_raw(0));
            OUTCOME_TRYV(proceed_until_syscall());
            // In syscall-exit state
            OUTCOME_TRY(regs, read_regs(tid));
            m_regs = regs;
            OUTCOME_TRY(ret, inject_syscall_entry_end(false));
            OUTCOME_TRYV(continue_exec_raw(0));
            OUTCOME_TRYV(proceed_until_syscall());
            // In syscall-entry state
            return ret;
        }
        case TraceeState::PostSysCallStop: {
            // In syscall-exit state
            OUTCOME_TRYV(inject_syscall_async(num, args));
            OUTCOME_TRYV(continue_exec_raw(0));
            OUTCOME_TRYV(proceed_until_syscall());
            // In syscall-entry state
            OUTCOME_TRYV(continue_exec_raw(0));
            OUTCOME_TRYV(proceed_until_syscall());
            // In syscall-exit state
            OUTCOME_TRY(regs, read_regs(tid));
            m_regs = regs;
            OUTCOME_TRY(ret, inject_syscall_exit_end());
            return ret;
        }
        default:
            return std::errc::invalid_argument;
    }
}

/*!
 * \brief Asynchronously inject a system call into the process
 *
 * This function asynchronously invokes a system call in the target process. It
 * does so by backing up the registers, changing the target system call number
 * and arguments, and if executed in the syscall-exit hook, steps back the
 * instruction pointer register.
 *
 * Depending on whether this function is called in the syscall-entry hook or the
 * syscall-exit hook, the behavior will differ slighly. The following tables
 * describe the sequence of events. The `Event` column describes the event that
 * occurs. The `Syscall` column indicates whether the event object passed to the
 * hooks will contain details of the original syscall or the injected syscall.
 * The `Status` column indicates the syscall status.
 *
 * The syscall status field allows the hooks to determine whether they are
 * operating on an injected syscall or not. When a system call is injected in
 * the syscall-entry hook, there is an additional `Repeated` status. Since the
 * syscall-entry hook must be invoked twice for the original syscall, the
 * `Repeated` status allows the hook to differentiate between the two and avoid
 * an infinite loop of syscall injections.
 *
 * The following is the sequence of events if a syscall is asynchronously
 * injected during the syscall-entry hook.
 *
 * | Event                      | Syscall  | Status   |
 * |----------------------------|----------|----------|
 * | Syscall-entry hook invoked | Original | Normal   |
 * | User injects async syscall | Injected | Injected |
 * | Syscall-exit hook invoked  | Injected | Injected |
 * | Syscall-entry hook invoked | Original | Repeated |
 * | Syscall-exit hook invoked  | Original | Normal   |
 *
 * The following is the sequence of events if a syscall is asynchronously
 * injected during the syscall-exit hook.
 *
 * | Event                      | Syscall  | Status   |
 * |----------------------------|----------|----------|
 * | Syscall-entry hook invoked | Original | Normal   |
 * | Syscall-exit hook invoked  | Original | Normal   |
 * | User injects async syscall | Injected | Injected |
 * | Syscall-entry hook invoked | Injected | Injected |
 * | Syscall-exit hook invoked  | Injected | Injected |
 *
 * This function can be called in either the syscall-entry hook or the
 * syscall-exit hook. It cannot be called in any other hook because it depends
 * on the `ptrace`d process being in the syscall-entry/exit state.
 *
 * \note If the syscall-entry/exit hook modifies the registers before calling
 *       this function, those changes will be lost. This is an intentional
 *       design choice to avoid needing to read/write the registers more times
 *       than necessary.
 *
 * \note Injecting `execve` is not allowed due to how injection is implemented.
 *       The method of manipulating the IP/PC register is insufficient when the
 *       entire memory of the process is replaced.
 *
 * \pre state() must be TraceeState::PreSysCallStop or
 *      TraceeState::PostSysCallStop
 *
 * \param num System call number
 * \param args System call arguments
 *
 * \return Nothing if system call is successfully injected. Otherwise, returns
 *         an appropriate error code. If a failure occurs, the tracee's
 *         registers are left in an undefined state, which may cause the process
 *         to crash if it continues executing.
 */
oc::result<void>
Tracee::inject_syscall_async(SysCallNum num, const SysCallArgs &args)
{
    if (m_sc_status == SysCallStatus::Injected) {
        return std::errc::invalid_argument;
    } else if (auto sc = SysCall(num, m_regs.abi());
            sc && strcmp(sc.name(), "execve") == 0) {
        return std::errc::invalid_argument;
    }

    switch (m_state) {
        case TraceeState::PreSysCallStop:
            return inject_syscall_entry_start(num, args);
        case TraceeState::PostSysCallStop:
            return inject_syscall_exit_start(num, args);
        default:
            return std::errc::invalid_argument;
    }
}

/*!
 * \brief Complete the asynchronous injection of a system call
 *
 * This function is called after the execution of the syscall-exit hook for an
 * asynchronously injected syscall. It will restore the backed up registers and
 * if the injection originally occurred in the syscall-entry hook, then the
 * instruction pointer register will be stepped back.
 *
 * \note This function is called by continue_exec() as needed. The tracer should
 *       not call this function.
 *
 * \return The result of the system call if it is successful. Otherwise, returns
 *         an appropriate error code. If a failure occurs, the tracee's
 *         registers are left in an undefined state, which may cause the process
 *         to crash if it continues executing.
 */
oc::result<SysCallRet> Tracee::inject_syscall_async_end()
{
    switch (m_inj_exec_mode) {
        case ExecMode::User:
            return inject_syscall_entry_end(true);
        case ExecMode::Kernel:
            return inject_syscall_exit_end();
        default:
            return std::errc::invalid_argument;
    }
}

/*!
 * \brief Asynchronously inject a system call into the process from the
 *        syscall-entry hook
 *
 * \sa inject_syscall_async()
 *
 * \param num System call number
 * \param args System call arguments
 *
 * \return Nothing if system call is successfully injected. Otherwise, returns
 *         an appropriate error code. If a failure occurs, the tracee's
 *         registers are left in an undefined state, which may cause the process
 *         to crash if it continues executing.
 */
oc::result<void>
Tracee::inject_syscall_entry_start(SysCallNum num, const SysCallArgs &args)
{
    m_sc_status = SysCallStatus::Injected;
    m_inj_exec_mode = m_exec_mode;
    m_inj_regs = m_regs;

    return modify_syscall_args(num, args);
}

/*!
 * \brief Complete the asynchronous injection of a system call injected from
 *        the syscall-entry hook
 *
 * \sa inject_syscall_async_end()
 *
 * \param will_repeat Whether the syscall-entry hook for the original syscall is
 *                    intended to be invoked again
 *
 * \return The result of the system call if it is successful. Otherwise, returns
 *         an appropriate error code. If a failure occurs, the tracee's
 *         registers are left in an undefined state, which may cause the process
 *         to crash if it continues executing.
 */
oc::result<SysCallRet>
Tracee::inject_syscall_entry_end(bool will_repeat)
{
    // Restore register backup
    m_inj_regs.set_ip(m_inj_regs.ip() - SYSCALL_OPSIZE);
    m_inj_regs.set_pending_syscall(m_inj_regs.ptrace_syscall());

    OUTCOME_TRYV(write_regs(tid, m_inj_regs));

    // Repeated status because the syscall-entry hook is invoked for the
    // original syscall again
    m_sc_status = will_repeat ? SysCallStatus::Repeated : SysCallStatus::Normal;

    return m_regs.ret();
}

/*!
 * \brief Asynchronously inject a system call into the process from the
 *        syscall-exit hook
 *
 * \sa inject_syscall_async()
 *
 * \param num System call number
 * \param args System call arguments
 *
 * \return Nothing if system call is successfully injected. Otherwise, returns
 *         an appropriate error code. If a failure occurs, the tracee's
 *         registers are left in an undefined state, which may cause the process
 *         to crash if it continues executing.
 */
oc::result<void>
Tracee::inject_syscall_exit_start(SysCallNum num, const SysCallArgs &args)
{
    m_sc_status = SysCallStatus::Injected;
    m_inj_exec_mode = m_exec_mode;
    m_inj_regs = m_regs;

    // Step back IP to redo syscall
    m_regs.set_ip(m_regs.ip() - SYSCALL_OPSIZE);
    m_regs.set_pending_syscall(num);

    return modify_syscall_args(num, args);
}

/*!
 * \brief Complete the asynchronous injection of a system call injected from
 *        the syscall-exit hook
 *
 * \sa inject_syscall_async_end()
 *
 * \return The result of the system call if it is successful. Otherwise, returns
 *         an appropriate error code. If a failure occurs, the tracee's
 *         registers are left in an undefined state, which may cause the process
 *         to crash if it continues executing.
 */
oc::result<SysCallRet> Tracee::inject_syscall_exit_end()
{
    // Restore register backup
    OUTCOME_TRYV(write_regs(tid, m_inj_regs));

    m_sc_status = SysCallStatus::Normal;

    return m_regs.ret();
}

/*!
 * \brief Modify syscall number and arguments
 *
 * \pre state() must be TraceeState::PreSysCallStop
 *
 * \return Nothing if the syscall number and arguments are successfully changed.
 *         Otherwise, returns an appropriate error code.
 */
oc::result<void>
Tracee::modify_syscall_args(SysCallNum num, const SysCallArgs &args)
{
    m_regs.set_ptrace_syscall(num);

    m_regs.set_arg0(args[0]);
    m_regs.set_arg1(args[1]);
    m_regs.set_arg2(args[2]);
    m_regs.set_arg3(args[3]);
    m_regs.set_arg4(args[4]);
    m_regs.set_arg5(args[5]);

    return write_regs(tid, m_regs);
}

/*!
 * \brief Modify syscall number and return value
 *
 * \pre state() must be TraceeState::PostSysCallStop
 *
 * \return Nothing if the syscall number and return value are successfully
 *         changed. Otherwise, returns an appropriate error code.
 */
oc::result<void>
Tracee::modify_syscall_ret(SysCallNum num, SysCallRet ret)
{
    m_regs.set_ptrace_syscall(num);

    m_regs.set_ret(ret);

    return write_regs(tid, m_regs);
}

/*!
 * \brief Supress syscall and set return value
 *
 * \pre state() must be TraceeState::PreSysCallStop
 *
 * \return Nothing if the syscall is successfully set to be suppressed.
 *         Otherwise, returns an appropriate error code.
 */
oc::result<void> Tracee::suppress_syscall(SysCallRet ret)
{
    if (m_state != TraceeState::PreSysCallStop) {
        return std::errc::invalid_argument;
    }

    auto getpid_sc = SysCall("getpid", m_regs.abi());
    if (!getpid_sc) {
        return std::errc::function_not_supported;
    }

    auto nr = m_regs.ptrace_syscall();

    OUTCOME_TRYV(modify_syscall_args(getpid_sc.num(), {}));

    m_suppress_orig_num = nr;
    m_suppress_ret = ret;

    return oc::success();
}

/*!
 * \brief Complete the supression of a syscall
 *
 * This should be called after entering the syscall-exit stop, but before the
 * syscall-exit hook is invoked. It sets the return value and restores the
 * original syscall number.
 *
 * \return Nothing if the syscall number and return value registers are
 *         successfully updated. Otherwise, returns an appropriate error code.
 */
oc::result<void> Tracee::suppress_syscall_post()
{
    // Restore syscall number and set return code if syscall was suppressed
    if (m_suppress_orig_num) {
        SysCallNum num = *std::exchange(m_suppress_orig_num, {});
        SysCallRet ret = std::exchange(m_suppress_ret, 0);

        OUTCOME_TRYV(modify_syscall_ret(num, ret));
    }

    return oc::success();
}

}
