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

#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "mbcommon/error_code.h"
#include "mbcommon/finally.h"

#include "mbsystrace/event_p.h"
#include "mbsystrace/procfs_p.h"

namespace mb::systrace
{

using namespace detail;

#if defined(__aarch64__)
const SysCallNum g_execve_by_abi[] = {
    SysCall("execve", ArchAbi::Aarch64).num(),
    SysCall("execve", ArchAbi::Eabi).num(),
};
#elif defined(__arm__)
const SysCallNum g_execve_by_abi[] = {
    SysCall("execve", ArchAbi::Eabi).num(),
};
#endif

/*!
 * \class Tracee
 *
 * \brief Class for manipulating a ptrace tracee
 */

/*!
 * \brief Construct new tracee
 *
 * \param tracer Reference to tracer. Used when synchronous operations that
 *               manipulate global tracer state are required, such as
 *               synchronous injection.
 * \param tgid Thread group ID. Can be set to `-1` to query procfs in seize().
 * \param tid Thread ID
 */
Tracee::Tracee(Tracer *tracer, pid_t tgid, pid_t tid)
    : tgid(tgid)
    , tid(tid)
    , m_tracer(tracer)
    , m_state(TraceeState::Detached)
    , m_exec_mode(ExecMode::User)
    , m_sc_status(SysCallStatus::Normal)
    , m_inj_exec_mode()
    , m_suppress_orig_num()
    , m_suppress_ret()
#if defined(__arm__) || defined(__aarch64__)
    , m_started_execve(false)
#endif
{
}

/*!
 * \brief Destroy tracee
 *
 * This will detach the tracee if it is not already detached or exited.
 * the tracee will be detached.
 */
Tracee::~Tracee()
{
    if (m_state != TraceeState::Exited && m_state != TraceeState::Detached) {
        (void) detach();
    }
}

/*!
 * \brief Send signal to thread
 *
 * If the thread dies due to the signal, it will be removed from the Tracer
 * once the ProcessDeathEvent is received.
 *
 * \param signal Signal to send
 *
 * \return Returns nothing on success or an error code on failure.
 */
oc::result<void> Tracee::signal_thread(int signal)
{
    if (tgid < 0) {
        return std::errc::invalid_argument;
    }

    if (syscall(SYS_tgkill, tgid, tid, signal) != 0) {
        return ec_from_errno();
    }

    return oc::success();
}

/*!
 * \brief Send signal to process
 *
 * The signal will be sent to one of the threads within the process that the
 * kernel picks.
 *
 * If the process dies due to the signal, it will be removed from the Tracer
 * once the ProcessDeathEvent is received.
 *
 * \param signal Signal to send
 *
 * \return Returns nothing on success or an error code on failure.
 */
oc::result<void> Tracee::signal_process(int signal)
{
    if (tgid < 0) {
        return std::errc::invalid_argument;
    }

    if (::kill(tgid, signal) != 0) {
        return ec_from_errno();
    }

    return oc::success();
}

/*!
 * \brief Continue the execution of the tracee until the next syscall or inject
 *        a signal
 *
 * This will complete the asynchronous injection of a system call if one was
 * injected.
 *
 * \pre state() must not be TraceeState::Detached or TraceeState::Exited
 *
 * \post
 *   * exec_mode() will be toggled appropriately if tracee was in a syscall stop
 *   * state() will be updated to TraceeState::Executing
 *
 * \param signal If zero, continue the execution of the tracee until the next
 *               syscall is reached (or another stop event occurs). If positive,
 *               inject the specified syscall.
 *
 * \return Returns nothing on success or an error code on failure.
 */
oc::result<void> Tracee::continue_exec(int signal)
{
    if (m_state == TraceeState::Detached || m_state == TraceeState::Exited) {
        return std::errc::invalid_argument;
    }

    // Reset the syscall status after the second invocation of the syscall-entry
    // hook if a syscall was injected in the first invocation
    if (m_sc_status == SysCallStatus::Repeated) {
        m_sc_status = SysCallStatus::Normal;
    }
    // Restore registers if in syscall-exit and injected
    else if (m_state == TraceeState::PostSysCallStop
            && m_sc_status == SysCallStatus::Injected) {
        OUTCOME_TRYV(inject_syscall_async_end());
    }

    return continue_exec_raw(signal);
}

oc::result<void> Tracee::continue_exec_raw(int signal)
{
    std::optional<ExecMode> new_exec_mode;

    switch (m_state) {
        case TraceeState::PreSysCallStop:
            new_exec_mode = ExecMode::Kernel;
            break;
        case TraceeState::PostSysCallStop:
            new_exec_mode = ExecMode::User;
            break;
        case TraceeState::Detached:
        case TraceeState::Exited:
            return std::errc::invalid_argument;
        default:
            break;
    }

    if (ptrace(PTRACE_SYSCALL, tid, nullptr, signal) != 0) {
        return ec_from_errno();
    }

    if (new_exec_mode) {
        m_exec_mode = *new_exec_mode;
    }

    m_state = TraceeState::Executing;

    return oc::success();
}

/*!
 * \brief Restart tracee without continuing execution
 *
 * This allows receiving further ptrace events for the tracee (eg. if it's in
 * group stop), but does not allow the tracee to execute any code.
 *
 * \pre state() must not be TraceeState::Detached or TraceeState::Exited
 *
 * \post state() will be updated to TraceeState::Executing
 *
 * \return Returns nothing on success or an error code on failure.
 */
oc::result<void> Tracee::continue_stopped()
{
    if (m_state == TraceeState::Detached || m_state == TraceeState::Exited) {
        return std::errc::invalid_argument;
    }

    if (ptrace(PTRACE_LISTEN, tid, nullptr, nullptr) != 0) {
        return ec_from_errno();
    }

    m_state = TraceeState::Executing;

    return oc::success();
}

/*!
 * \brief Trigger a ptrace stop for the tracee
 *
 * This will result in the tracee being stopped. If another ptrace stop event
 * would have been emitted for the tracee, that event will still be emitted and
 * this call will be a no-op. Otherwise, an InterruptStopEvent will be emitted.
 *
 * \pre state() must not be TraceeState::Detached or TraceeState::Exited
 *
 * \return Returns nothing on success or an error code on failure.
 */
oc::result<void> Tracee::interrupt()
{
    if (m_state == TraceeState::Detached || m_state == TraceeState::Exited) {
        return std::errc::invalid_argument;
    }

    if (ptrace(PTRACE_INTERRUPT, tid, nullptr, nullptr) != 0) {
        return ec_from_errno();
    }

    return oc::success();
}

/*!
 * \brief Attach to the process via ptrace seize operation
 *
 * If SeizeFlag::TraceChildren is specified in \p flags, when the tracee creates
 * a new child process (or thread) via `fork`, `vfork`, or `clone`, then the new
 * process will also be traced.
 *
 * If SeizeFlag::TryKillOnExit is specified, the tracee will be killed by the
 * kernel when the tracer process exits.
 *
 * \note SeizeFlag::TryKillOnExit requires kernel 3.8 to work. If running on a
 *       system with an older kernel, the flag is a no-op.
 *
 * \param flags Flags controlling the seize operation
 *
 * \pre state() must be TraceeState::Detached
 *
 * \post state() will be updated to TraceeState::Executing. If the seize
 *       operation fails, the state will not be changed.
 *
 * \return Returns nothing on success or an error code on failure.
 */
oc::result<void> Tracee::seize(SeizeFlags flags)
{
    if (m_state != TraceeState::Detached) {
        return std::errc::invalid_argument;
    }

    long options = PTRACE_O_TRACESYSGOOD
            | PTRACE_O_TRACEEXEC
            | PTRACE_O_TRACEEXIT;

    if (flags & SeizeFlag::TraceChildren) {
        options |= PTRACE_O_TRACECLONE
                | PTRACE_O_TRACEFORK
                | PTRACE_O_TRACEVFORK;
    }

    long ret = -1;
    errno = EINVAL;

    if (flags & SeizeFlag::TryKillOnExit) {
        ret = ptrace(PTRACE_SEIZE, tid, nullptr, options | PTRACE_O_EXITKILL);
    }

    // PTRACE_O_EXITKILL was added in kernel 3.8
    if (ret != 0 && errno == EINVAL) {
        ret = ptrace(PTRACE_SEIZE, tid, nullptr, options);
    }

    if (ret != 0) {
        return ec_from_errno();
    }

    auto detach = finally([&] {
        ptrace(PTRACE_DETACH, tid, nullptr, nullptr);
    });

    OUTCOME_TRYV(resolve_tgid());

    m_state = TraceeState::Executing;

    detach.dismiss();

    return oc::success();
}

/*!
 * \brief Determine tgid from procfs if unknown
 *
 * \return Returns nothing on success or an error code on failure
 */
oc::result<void> Tracee::resolve_tgid()
{
    if (tgid < 0) {
        OUTCOME_TRY(real_tgid, get_tgid(tid));
        tgid = real_tgid;
    }

    return oc::success();
}

/*!
 * \brief Detach the tracee
 *
 * If the tracee is in a ptrace stop, then this function detaches the process
 * directly. Otherwise, interrupt() will be invoked to induce a ptrace stop
 * prior to the detaching.
 *
 * \pre state() must not be TraceeState::Detached or TraceeState::Exited
 *
 * \post If successful, state() will be updated to TraceeState::Detached if the
 *       tracee is detached or TraceeState::Exited if the tracee exited or died
 *       before it could be detached (only if detaching outside of a ptrace
 *       stop). Otherwise, the state is left unchanged.
 *
 * \return Returns nothing on success or an error code on failure. If
 *         `std::errc::no_such_process` is returned, the tracer should remove
 *         all references to the tracee as it is no longer alive.
 */
oc::result<void> Tracee::detach()
{
    if (m_state == TraceeState::Detached || m_state == TraceeState::Exited) {
        return std::errc::invalid_argument;
    }

    if (ptrace(PTRACE_DETACH, tid, nullptr, nullptr) == 0) {
        m_state = TraceeState::Detached;
        return oc::success();
    } else if (errno != ESRCH) {
        return ec_from_errno();
    }

    // At this point, the process is either dead or is not in a ptrace stop

    if (auto r = signal_thread(0); !r) {
        if (r.error() == std::errc::no_such_process) {
            m_state = TraceeState::Exited;
            return oc::success();
        } else {
            return r.as_failure();
        }
    }

    if (auto r = interrupt(); !r) {
        if (r.error() == std::errc::no_such_process) {
            m_state = TraceeState::Exited;
            return oc::success();
        } else {
            return r.as_failure();
        }
    }

    // Cannot use Tracer hooks since this function might execute during the
    // destruction of both the Tracer and the Tracee

    auto set_state_to = TraceeState::Detached;

    while (true) {
        OUTCOME_TRY(event, next_event(tid));

        OUTCOME_TRY(loop_again, std::visit(
            [&](auto &&e) -> oc::result<bool> {
                using T = std::decay_t<decltype(e)>;

                int sig = 0;

                if constexpr (std::is_same_v<T, NoChildrenEvent>) {
                    MB_UNREACHABLE("%d is not a tracee", tid);
                } else if constexpr (std::is_same_v<T, ProcessExitEvent>
                        || std::is_same_v<T, ProcessDeathEvent>) {
                    set_state_to = TraceeState::Exited;
                    return false;
                } else if constexpr (std::is_same_v<T, RetryEvent>) {
                    return true;
                } else if constexpr (std::is_same_v<T, SignalDeliveryStopEvent>) {
                    sig = e.signal;
                }

                // Assume that this will succeed (like strace)
                ptrace(PTRACE_DETACH, tid, nullptr, sig);
                return false;
            },
            event
        ));

        if (!loop_again) {
            break;
        }
    }

    m_state = set_state_to;
    return oc::success();
}

/*!
 * \brief Tracee state
 *
 * \sa TraceeState
 *
 * \return Tracee state
 */
TraceeState Tracee::state() const
{
    return m_state;
}

/*!
 * \brief Set tracee state
 *
 * \sa TraceeState
 *
 * \note This should normally not be changed since the owning Tracer will ensure
 *       that the current state is accurate based on the ptrace events that it
 *       has received.
 *
 * \param state Tracee state
 */
void Tracee::set_state(TraceeState state)
{
    m_state = state;
}

/*!
 * \brief Execution context of tracee
 *
 * \sa ExecMode
 *
 * \return Execution context
 */
ExecMode Tracee::exec_mode() const
{
    return m_exec_mode;
}

/*!
 * \brief Set execution context of tracee
 *
 * \sa ExecMode
 *
 * \note This should normally not be changed. The owning Tracer will update this
 *       value upon receiving syscall-stop events and restarting the tracee.
 *
 * \param mode Execution context
 */
void Tracee::set_exec_mode(ExecMode mode)
{
    m_exec_mode = mode;
}

/*!
 * \brief State of registers during syscall-stop
 *
 * \pre state() must be TraceeState::PreSysCallStop or
 *      TraceeState::PostSysCallStop
 *
 * \return Current state of the registers
 */
ArchRegs Tracee::regs() const
{
    return m_regs;
}

/*!
 * \brief Set state of registers
 *
 * \note This function does **not** change the tracees registers. It only
 *       updates the register cache used by other functions that actually modify
 *       the registers. This should not be called by anything other than the
 *       owning Tracer.
 *
 * \pre state() must be TraceeState::PreSysCallStop or
 *      TraceeState::PostSysCallStop
 *
 * \param regs State of the registers
 */
void Tracee::set_regs(ArchRegs regs)
{
    m_regs = regs;
}

/*!
 * \brief Run pre-syscall-entry-hook fixups
 */
void Tracee::syscall_entry_pre_hook()
{
#if defined(__arm__) || defined(__aarch64__)
    auto i = static_cast<std::underlying_type_t<ArchAbi>>(m_regs.abi());

    if (m_regs.ptrace_syscall() == g_execve_by_abi[i]) {
        m_started_execve = true;
    }
#endif
}

/*!
 * \brief Run pre-syscall-exit-hook fixups
 */
oc::result<void> Tracee::syscall_exit_pre_hook()
{
#if defined(__arm__) || defined(__aarch64__)
    // This is a hack for arm and aarch64 where when execve completes, the
    // syscall number is reset to zero, preventing exec from being hooked in the
    // syscall-exit stop.

    if (m_started_execve) {
        auto i = static_cast<std::underlying_type_t<ArchAbi>>(m_regs.abi());

        m_regs.set_ptrace_syscall(g_execve_by_abi[i]);
        m_started_execve = false;
    }
#endif

    return suppress_syscall_post();
}

/*!
 * \brief Syscall status during asynchronous injection
 *
 * \sa SysCallStatus
 * \sa inject_syscall_async()
 *
 * \pre state() must be TraceeState::PreSysCallStop or
 *      TraceeState::PostSysCallStop
 *
 * \return Syscall injection status
 */
SysCallStatus Tracee::syscall_status() const
{
    return m_sc_status;
}

}
