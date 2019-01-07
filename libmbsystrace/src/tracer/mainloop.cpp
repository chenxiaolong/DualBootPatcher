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

#include <cstring>

#include "mbcommon/finally.h"

#include "mbsystrace/event_p.h"
#include "mbsystrace/syscalls.h"
#include "mbsystrace/tracee.h"

#define DEBUG_EVENTS 0

#if DEBUG_EVENTS
#define DEBUG(...) printf(__VA_ARGS__)
#else
#define DEBUG(...)
#endif

namespace mb::systrace
{

using namespace detail;

namespace
{

template<class T>
struct AlwaysFalse : std::false_type
{
};

}

#if DEBUG_EVENTS
static const char * status_string(SysCallStatus status)
{
    switch (status) {
        case SysCallStatus::Normal:
            return "<normal>";
        case SysCallStatus::Injected:
            return "<injected>";
        case SysCallStatus::Repeated:
            return "<repeated>";
        default:
            return "<unknown>";
    }
}

static const char * syscall_string(SysCallNum num, ArchAbi abi)
{
    if (auto sc = SysCall(num, abi)) {
        return sc.name();
    } else {
        return "<unknown>";
    }
}

static void print_event(const ProcessEvent &event)
{
    std::visit(
        [](auto &&e) {
            using T = std::decay_t<decltype(e)>;

            if constexpr (std::is_same_v<T, NoChildrenEvent>) {
                DEBUG("NoChildrenEvent { }\n");
            } else if constexpr (std::is_same_v<T, RetryEvent>) {
                DEBUG("RetryEvent { }\n");
            } else if constexpr (std::is_same_v<T, ProcessExitEvent>) {
                DEBUG("ProcessExitEvent { pid=%d, code=%d }\n",
                       e.pid, e.exit_code);
            } else if constexpr (std::is_same_v<T, ProcessDeathEvent>) {
                DEBUG("ProcessDeathEvent { pid=%d, signal=%d }\n",
                      e.pid, e.exit_signal);
            } else if constexpr (std::is_same_v<T, SysCallStopEvent>) {
                DEBUG("SysCallStopEvent { tid=%d, abi=%d, num=%lu/%s"
                      ", arg0=0x%lx, arg1=0x%lx, arg2=0x%lx, arg3=0x%lx"
                      ", arg4=0x%lx, arg5=0x%lx, ret=%ld }\n",
                      e.tid, static_cast<int>(e.regs.abi()),
                      e.regs.ptrace_syscall(),
                      syscall_string(e.regs.ptrace_syscall(), e.regs.abi()),
                      e.regs.arg0(), e.regs.arg1(), e.regs.arg2(),
                      e.regs.arg3(), e.regs.arg4(), e.regs.arg5(),
                      e.regs.ret());
            } else if constexpr (std::is_same_v<T, SignalDeliveryStopEvent>) {
                DEBUG("SignalDeliveryStopEvent { tid=%d, signal=%d }\n",
                      e.tid, e.signal);
            } else if constexpr (std::is_same_v<T, GroupStopEvent>) {
                DEBUG("GroupStopEvent { tid=%d, signal=%d }\n",
                      e.tid, e.signal);
            } else if constexpr (std::is_same_v<T, InterruptStopEvent>) {
                DEBUG("InterruptStopEvent { tid=%d, signal=%d }\n",
                      e.tid, e.signal);
            } else if constexpr (std::is_same_v<T, NewProcessStopEvent>) {
                DEBUG("NewProcessStopEvent { tid=%d, new_tid=%d }\n",
                      e.tid, e.new_tid);
            } else if constexpr (std::is_same_v<T, ExecveStopEvent>) {
                DEBUG("ExecveStopEvent { tid=%d, orig_tid=%d }\n",
                      e.tid, e.orig_tid);
            } else if constexpr (std::is_same_v<T, ExitStopEvent>) {
                DEBUG("ExitStopEvent { tid=%d, status=0x%x }\n",
                      e.tid, e.status);
            } else {
                static_assert(AlwaysFalse<T>::value, "non-exhaustive visitor");
            }
        },
        event
    );
}
#endif

/*!
 * \brief Call the appropriate hooks based on an event
 *
 * \param hooks Callback hooks for tracee events
 * \param event Incoming event for a tracee
 *
 * \return
 *   * True if the event loop should continue
 *   * False if the event loop should stop (successfully)
 *   * An appropriate error code if an error occurs
 */
oc::result<bool> Tracer::dispatch_event(const Hooks &hooks,
                                        const ProcessEvent &event)
{
    auto ret = std::visit(
        [this, &hooks](auto &&e) -> oc::result<bool> {
            using T = std::decay_t<decltype(e)>;

            if constexpr (std::is_same_v<T, NoChildrenEvent>) {
                return false;
            } else if constexpr (std::is_same_v<T, RetryEvent>) {
                return true;
            } else if constexpr (std::is_same_v<T, ProcessExitEvent>) {
                if (auto tracee = find_tracee(e.pid)) {
                    execute_tracee_exit_hook(hooks, e.pid, e.status,
                                             e.exit_code);
                } else {
                    execute_unknown_child_hook(hooks, e.pid, e.status);
                }
                return true;
            } else if constexpr (std::is_same_v<T, ProcessDeathEvent>) {
                if (auto tracee = find_tracee(e.pid)) {
                    execute_tracee_death_hook(hooks, e.pid, e.status,
                                              e.exit_signal);
                } else {
                    execute_unknown_child_hook(hooks, e.pid, e.status);
                }
                return true;
            } else if constexpr (std::is_same_v<T, ExecveStopEvent>) {
                auto tracee = find_tracee(e.orig_tid);
                if (!tracee) {
                    MB_UNREACHABLE("Tracee %d not found", e.orig_tid);
                }

                assert(e.tid == tracee->tgid);

                // Threads aside from the execve-ing thread and the thread group
                // leader will report their deaths. If we're tracing the thread
                // group leader, remove its Tracee object and update the TID of
                // the execve-ing thread to the TGID.

                if (e.tid != e.orig_tid) {
                    bool need_new_tracee_hook = false;

                    if (auto it = m_tracees.find(e.tid); it != m_tracees.end()) {
                        // The thread group leader will just disappear, so don't
                        // try to detach or kill it
                        it->second->set_state(TraceeState::Exited);
                    } else {
                        // We're not tracing the thread group leader, but we're
                        // about to, so make sure the new tracee hook is invoked
                        need_new_tracee_hook = true;
                    }

                    // orig_tid will magically become tid after execve()
                    m_tracees[e.tid] = std::move(m_tracees[e.orig_tid]);
                    m_tracees.erase(e.orig_tid);

                    tracee->tid = e.tid;

                    execute_tracee_disappear_hook(hooks, e.orig_tid);

                    if (need_new_tracee_hook) {
                        execute_new_tracee_hook(hooks, tracee);
                    }
                }

                tracee->set_state(TraceeState::ExecveStop);
                OUTCOME_TRYV(tracee->continue_exec(0));
                return true;
            } else {
                auto tracee = find_tracee(e.tid);
                if (!tracee) {
                    // If the tracee is not found, then a tracee must have
                    // created a new child process.

                    OUTCOME_TRY(new_tracee, add_child(-1, e.tid));

                    // Read tgid from procfs since we don't know if the new
                    // tracee is a thread or not
                    OUTCOME_TRYV(new_tracee->resolve_tgid());

                    execute_new_tracee_hook(hooks, new_tracee);

                    tracee = new_tracee;
                }

                if constexpr (std::is_same_v<T, SysCallStopEvent>) {
                    tracee->set_regs(e.regs);

                    switch (tracee->exec_mode()) {
                        case ExecMode::User: {
                            tracee->set_state(TraceeState::PreSysCallStop);
                            tracee->syscall_entry_pre_hook();

                            SysCallEntryInfo info{
                                SysCall(e.regs.ptrace_syscall(), e.regs.abi()),
                                {{
                                    e.regs.arg0(),
                                    e.regs.arg1(),
                                    e.regs.arg2(),
                                    e.regs.arg3(),
                                    e.regs.arg4(),
                                    e.regs.arg5(),
                                }},
                                tracee->syscall_status(),
                            };
                            assert(info.syscall);

                            OUTCOME_TRYV(execute_syscall_entry_hook(
                                    hooks, tracee, info));
                            break;
                        }
                        case ExecMode::Kernel: {
                            tracee->set_state(TraceeState::PostSysCallStop);
                            OUTCOME_TRYV(tracee->syscall_exit_pre_hook());

                            auto regs = tracee->regs();

                            SysCallExitInfo info{
                                SysCall(regs.ptrace_syscall(), regs.abi()),
                                regs.ret(),
                                tracee->syscall_status(),
                            };
                            assert(info.syscall);

                            OUTCOME_TRYV(execute_syscall_exit_hook(
                                    hooks, tracee, info));
                            break;
                        }
                        default:
                            MB_UNREACHABLE("Invalid exec mode");
                    }

                    return true;
                } else if constexpr (std::is_same_v<T, SignalDeliveryStopEvent>) {
                    tracee->set_state(TraceeState::SignalStop);
                    OUTCOME_TRYV(execute_tracee_signal_hook(
                            hooks, tracee, e.signal));
                    return true;
                } else if constexpr (std::is_same_v<T, GroupStopEvent>) {
                    tracee->set_state(TraceeState::GroupStop);
                    OUTCOME_TRYV(execute_group_stop_hook(
                            hooks, tracee, e.signal));
                    return true;
                } else if constexpr (std::is_same_v<T, InterruptStopEvent>) {
                    tracee->set_state(TraceeState::InterruptStop);
                    OUTCOME_TRYV(execute_interrupt_stop_hook(hooks, tracee));
                    return true;
                } else if constexpr (std::is_same_v<T, NewProcessStopEvent>) {
                    // This event is intentionally ignored. When a tracee uses
                    // SYS_fork, SYS_vfork, or SYS_clone with vfork semantics,
                    // the kernel will sometimes deliver an InterruptStopEvent
                    // for the child before NewProcessStopEvent for the parent.
                    // Instead, we'll call the new tracee hook when it hits its
                    // first ptrace stop.
                    OUTCOME_TRYV(tracee->continue_exec(0));
                    return true;
                } else if constexpr (std::is_same_v<T, ExitStopEvent>) {
                    tracee->set_state(TraceeState::ExitStop);
                    OUTCOME_TRYV(tracee->continue_exec(0));
                    return true;
                } else {
                    static_assert(AlwaysFalse<T>::value, "non-exhaustive visitor");
                }
            }
        },
        event
    );

    // A tracee may be killed at any point, so retry if we get an ESRCH. We
    // should see a ProcessDeathEvent later.
    if (!ret) {
        if (ret.error() == std::errc::no_such_process) {
            return true;
        } else {
            return ret.as_failure();
        }
    }

    return ret.value();
}

void
Tracer::execute_new_tracee_hook(const Hooks &hooks, Tracee *tracee)
{
    DEBUG("[Hook] NewTracee { tgid=%d, tid=%d }\n", tracee->tgid, tracee->tid);

    if (hooks.new_tracee) {
        hooks.new_tracee(tracee);
    }
}

void
Tracer::execute_tracee_exit_hook(const Hooks &hooks, pid_t tid, int status,
                                 int exit_code)
{
    DEBUG("[Hook] TraceeExit { tid=%d, status=0x%x, exit_code=%d }\n",
          tid, status, exit_code);

    auto ret = hooks.tracee_exit
            ? hooks.tracee_exit(tid, exit_code)
            : action::Default{};

    find_tracee(tid)->set_state(TraceeState::Exited);

    std::visit(
        [&](auto &&a) {
            using T = std::decay_t<decltype(a)>;

            if constexpr (std::is_same_v<T, action::NoAction>
                    || std::is_same_v<T, action::Default>) {
                remove_child(tid);
            } else if constexpr (std::is_same_v<T, action::RequeueEvent>) {
                m_requeued_event = ProcessExitEvent{tid, status, exit_code};
            } else {
                static_assert(AlwaysFalse<T>::value, "non-exhaustive visitor");
            }
        },
        ret
    );
}

void
Tracer::execute_tracee_death_hook(const Hooks &hooks, pid_t tid, int status,
                                  int exit_signal)
{
    DEBUG("[Hook] TraceeDeath { tid=%d, status=0x%x, exit_signal=%d }\n",
          tid, status, exit_signal);

    auto ret = hooks.tracee_death
            ? hooks.tracee_death(tid, exit_signal)
            : action::Default{};

    find_tracee(tid)->set_state(TraceeState::Exited);

    std::visit(
        [&](auto &&a) {
            using T = std::decay_t<decltype(a)>;

            if constexpr (std::is_same_v<T, action::NoAction>
                    || std::is_same_v<T, action::Default>) {
                remove_child(tid);
            } else if constexpr (std::is_same_v<T, action::RequeueEvent>) {
                m_requeued_event = ProcessDeathEvent{tid, status, exit_signal};
            } else {
                static_assert(AlwaysFalse<T>::value, "non-exhaustive visitor");
            }
        },
        ret
    );
}

void
Tracer::execute_tracee_disappear_hook(const Hooks &hooks, pid_t tid)
{
    DEBUG("[Hook] TraceeDisappear { tid=%d }\n", tid);

    if (hooks.tracee_disappear) {
        hooks.tracee_disappear(tid);
    }
}

oc::result<void>
Tracer::execute_tracee_signal_hook(const Hooks &hooks, Tracee *tracee,
                                   int signal)
{
    DEBUG("[Hook] TraceeSignal { tid=%d, signal=%d }\n", tracee->tid, signal);

    auto ret = hooks.tracee_signal
            ? hooks.tracee_signal(tracee, signal)
            : action::Default{};

    return std::visit(
        [&](auto &&a) -> oc::result<void> {
            using T = std::decay_t<decltype(a)>;

            if constexpr (std::is_same_v<T, action::ForwardSignal>
                    || std::is_same_v<T, action::Default>) {
                OUTCOME_TRYV(tracee->continue_exec(signal));
            } else if constexpr (std::is_same_v<T, action::SuppressSignal>) {
                OUTCOME_TRYV(tracee->continue_exec(0));
            } else if constexpr (std::is_same_v<T, action::DetachTracee>) {
                if (!remove_child(tracee->tid)) {
                    MB_UNREACHABLE("tid %d was not registered", tracee->tid);
                }
            } else if constexpr (std::is_same_v<T, action::KillTracee>) {
                OUTCOME_TRYV(tracee->signal_thread(SIGKILL));
            } else {
                static_assert(AlwaysFalse<T>::value, "non-exhaustive visitor");
            }
            return oc::success();
        },
        ret
    );
}

oc::result<void>
Tracer::execute_group_stop_hook(const Hooks &hooks, Tracee *tracee, int signal)
{
    DEBUG("[Hook] GroupStop { tid=%d, signal=%d }\n", tracee->tid, signal);

    auto ret = hooks.group_stop
            ? hooks.group_stop(tracee, signal)
            : action::Default{};

    return std::visit(
        [&](auto &&a) -> oc::result<void> {
            using T = std::decay_t<decltype(a)>;

            if constexpr (std::is_same_v<T, action::ContinueStopped>
                    || std::is_same_v<T, action::Default>) {
                OUTCOME_TRYV(tracee->continue_stopped());
            } else if constexpr (std::is_same_v<T, action::DetachTracee>) {
                if (!remove_child(tracee->tid)) {
                    MB_UNREACHABLE("tid %d was not registered", tracee->tid);
                }
            } else if constexpr (std::is_same_v<T, action::KillTracee>) {
                OUTCOME_TRYV(tracee->signal_thread(SIGKILL));
            } else {
                static_assert(AlwaysFalse<T>::value, "non-exhaustive visitor");
            }
            return oc::success();
        },
        ret
    );
}

oc::result<void>
Tracer::execute_interrupt_stop_hook(const Hooks &hooks, Tracee *tracee)
{
    DEBUG("[Hook] InterruptStop { tid=%d }\n", tracee->tid);

    auto ret = hooks.interrupt_stop
            ? hooks.interrupt_stop(tracee)
            : action::Default{};

    return std::visit(
        [&](auto &&a) -> oc::result<void> {
            using T = std::decay_t<decltype(a)>;

            if constexpr (std::is_same_v<T, action::ContinueExec>
                    || std::is_same_v<T, action::Default>) {
                OUTCOME_TRYV(tracee->continue_exec(0));
            } else if constexpr (std::is_same_v<T, action::DetachTracee>) {
                if (!remove_child(tracee->tid)) {
                    MB_UNREACHABLE("tid %d was not registered", tracee->tid);
                }
            } else if constexpr (std::is_same_v<T, action::KillTracee>) {
                OUTCOME_TRYV(tracee->signal_thread(SIGKILL));
            } else {
                static_assert(AlwaysFalse<T>::value, "non-exhaustive visitor");
            }
            return oc::success();
        },
        ret
    );
}

oc::result<void>
Tracer::execute_syscall_entry_hook(const Hooks &hooks, Tracee *tracee,
                                   const SysCallEntryInfo &info)
{
    DEBUG("[Hook] SysCallEntry { tid=%d, abi=%d, num=%lu/%s"
          ", arg0=0x%lx, arg1=0x%lx, arg2=0x%lx, arg3=0x%lx, arg4=0x%lx"
          ", arg5=0x%lx, status=%s }\n",
          tracee->tid, static_cast<int>(info.syscall.abi()), info.syscall.num(),
          syscall_string(info.syscall.num(), info.syscall.abi()), info.args[0],
          info.args[1], info.args[2], info.args[3], info.args[4], info.args[5],
          status_string(info.status));

    auto ret = hooks.syscall_entry
            ? hooks.syscall_entry(tracee, info)
            : action::Default{};

    return std::visit(
        [&](auto &&a) -> oc::result<void> {
            using T = std::decay_t<decltype(a)>;

            if constexpr (std::is_same_v<T, action::ContinueExec>
                    || std::is_same_v<T, action::Default>) {
                OUTCOME_TRYV(tracee->continue_exec(0));
            } else if constexpr (std::is_same_v<T, action::SuppressSysCall>) {
                OUTCOME_TRYV(tracee->suppress_syscall(a.ret));
                OUTCOME_TRYV(tracee->continue_exec(0));
            } else if constexpr (std::is_same_v<T, action::DetachTracee>) {
                if (!remove_child(tracee->tid)) {
                    MB_UNREACHABLE("tid %d was not registered", tracee->tid);
                }
            } else if constexpr (std::is_same_v<T, action::KillTracee>) {
                OUTCOME_TRYV(tracee->signal_thread(SIGKILL));
            } else if constexpr (std::is_same_v<T, action::NoAction>) {
                // Ignore
            } else {
                static_assert(AlwaysFalse<T>::value, "non-exhaustive visitor");
            }
            return oc::success();
        },
        ret
    );
}

oc::result<void>
Tracer::execute_syscall_exit_hook(const Hooks &hooks, Tracee *tracee,
                                  const SysCallExitInfo &info)
{
    DEBUG("[Hook] SysCallExit { tid=%d, abi=%d, num=%lu/%s"
          ", ret=%ld, status=%s }\n",
          tracee->tid, static_cast<int>(info.syscall.abi()), info.syscall.num(),
          syscall_string(info.syscall.num(), info.syscall.abi()), info.ret,
          status_string(info.status));

    auto hook_ret = hooks.syscall_exit
            ? hooks.syscall_exit(tracee, info)
            : action::Default{};

    return std::visit(
        [&](auto &&a) -> oc::result<void> {
            using T = std::decay_t<decltype(a)>;

            if constexpr (std::is_same_v<T, action::ContinueExec>
                    || std::is_same_v<T, action::Default>) {
                OUTCOME_TRYV(tracee->continue_exec(0));
            } else if constexpr (std::is_same_v<T, action::DetachTracee>) {
                if (!remove_child(tracee->tid)) {
                    MB_UNREACHABLE("tid %d was not registered", tracee->tid);
                }
            } else if constexpr (std::is_same_v<T, action::KillTracee>) {
                OUTCOME_TRYV(tracee->signal_thread(SIGKILL));
            } else if constexpr (std::is_same_v<T, action::NoAction>) {
                // Ignore
            } else {
                static_assert(AlwaysFalse<T>::value, "non-exhaustive visitor");
            }
            return oc::success();
        },
        hook_ret
    );
}

void
Tracer::execute_unknown_child_hook(const Hooks &hooks, pid_t tid, int status)
{
    DEBUG("[Hook] UnknownChild { tid=%d, status=0x%x }\n", tid, status);

    if (hooks.unknown_child) {
        hooks.unknown_child(tid, status);
    }
}

/*!
 * \brief Find Tracee instance by its TID
 *
 * \param tid TID to find
 *
 * \return Tracee instance if found. Otherwise, nullptr.
 */
Tracee * Tracer::find_tracee(pid_t tid)
{
    if (auto it = m_tracees.find(tid); it != m_tracees.end()) {
        return it->second.get();
    } else {
        return nullptr;
    }
}

/*!
 * \brief Start event loop
 *
 * Calling this function is equivalent to calling `execute(hooks, -1)`.
 *
 * \sa execute(const Hooks &, int)
 *
 * \param hooks Callback hooks for tracee events
 */
oc::result<void> Tracer::execute(const Hooks &hooks)
{
    return execute(hooks, -1);
}

/*!
 * \brief Start event loop
 *
 * Calling this function will start the event loop and block until all tracees
 * have been detached or an error occurs. If an error occurs, the only valid
 * action that can be performed on this Tracer object is destroying it.
 *
 * The event loop will wait for events (for tracees specified by \p pid_spec)
 * and then invoke the callbacks specified in \p hooks. The callbacks can
 * influence how a tracee behaves, for example, by modifying syscall parameters
 * and return values or suppressing syscalls.
 *
 * There is intentionally no way for callbacks to report errors. The callbacks
 * must make a decision about what will happen if an error occurs. For example,
 * if an error occurs, a callback could call stop_after_hook() to stop the event
 * loop and/or return action::KillTracee to kill the problematic tracee.
 *
 * \param hooks Callback hooks for tracee events
 * \param pid_spec
 *   Specification for the tracees in which to receive events. This has the same
 *   semantics as the first parameter of `waitpid()`. The most common value is
 *   `-1`, which allows receiving events for all tracees. The possible values
 *   are:
 *     * If `< -1`, receive events for any tracee whose process group ID is
 *       `|pid|`
 *     * If `== -1`, receive events for any tracee
 *     * If `== 0`, receive events for any tracee whose process group ID matches
 *       that of the calling process
 *     * If `> 0`, receive events for the tracee whose process ID is equal to
 *       \p pid_spec
 *
 * \return Returns nothing if no errors occur while tracing the tracees.
 *         Otherwise, a specific error code is returned.
 */
oc::result<void> Tracer::execute(const Hooks &hooks, int pid_spec)
{
    while (!m_should_stop && (pid_spec == -1
            ? !m_tracees.empty()
            : m_tracees.find(pid_spec) != m_tracees.end())) {
        ProcessEvent e;

        if (m_requeued_event) {
            e = *m_requeued_event;
            m_requeued_event.reset();

#if DEBUG_EVENTS
            DEBUG("[Replayed] ");
#endif
        } else {
            OUTCOME_TRY(event, next_event(pid_spec));
            e = event;
        }

#if DEBUG_EVENTS
        print_event(e);
#endif

        OUTCOME_TRY(should_continue, dispatch_event(hooks, e));

        if (!should_continue) {
            break;
        }
    }

    m_should_stop = false;

    return oc::success();
}

/*!
 * \brief Stop event loop on the next iteration
 *
 * \note This may sometimes appear to be slightly delayed because multiple hooks
 *       can be called in one iteration of the event loop. For example, when a
 *       tracee creates a new child process and it begins executing,
 *       Hooks::new_tracee and another hook will be called on the same iteration
 *       of the event loop.
 */
void Tracer::stop_after_hook() noexcept
{
    m_should_stop = true;
}

}
