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

#include <optional>

#include "mbcommon/common.h"
#include "mbcommon/flags.h"
#include "mbcommon/outcome.h"

#include "mbsystrace/arch.h"
#include "mbsystrace/hooks.h"
#include "mbsystrace/types.h"


/*!
 * \file mbsystrace/tracee.h
 *
 * \brief Types relevant to tracees
 */

namespace mb::systrace
{

//! Flags to use when performing a ptrace seize on a process
enum class SeizeFlag
{
    //! Trace `fork`/`vfork`/`clone`
    TraceChildren = 1 << 0,
    //! Set `PTRACE_O_EXITKILL` if supported
    TryKillOnExit = 1 << 1,
};
MB_DECLARE_FLAGS(SeizeFlags, SeizeFlag)
MB_DECLARE_OPERATORS_FOR_FLAGS(SeizeFlags)

//! List of possible Tracee states
enum class TraceeState
{
    //! Detached
    Detached,
    //! Exited or killed
    Exited,
    //! Executing
    Executing,
    //! Syscall-entry stop
    PreSysCallStop,
    //! Syscall-exit stop
    PostSysCallStop,
    //! Signal delivery stop
    SignalStop,
    //! Group stop
    GroupStop,
    //! Interrupt stop
    InterruptStop,
    //! `fork`/`vfork`/`clone` stop
    NewProcessStop,
    //! `execve` stop
    ExecveStop,
    //! Exit stop
    ExitStop,
};

//! Execution context of tracee
enum class ExecMode
{
    //! Running user space code
    User,
    //! Running kernel space code (i.e. syscall)
    Kernel,
};

class Tracer;

class MB_EXPORT Tracee final
{
public:
    Tracee(Tracer *tracer, pid_t tgid, pid_t tid);
    ~Tracee();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(Tracee)
    MB_DISABLE_MOVE_CONSTRUCT_AND_ASSIGN(Tracee)

    oc::result<void> signal_thread(int signal);
    oc::result<void> signal_process(int signal);

    oc::result<void> continue_exec(int signal);
    oc::result<void> continue_stopped();

    oc::result<void> interrupt();

    oc::result<void> seize(SeizeFlags flags);
    oc::result<void> detach();

    oc::result<void> resolve_tgid();

    TraceeState state() const;
    void set_state(TraceeState state);

    ExecMode exec_mode() const;
    void set_exec_mode(ExecMode mode);

    ArchRegs regs() const;
    void set_regs(ArchRegs regs);

    void syscall_entry_pre_hook();
    oc::result<void> syscall_exit_pre_hook();

    SysCallStatus syscall_status() const;

    // Child memory access

    oc::result<size_t> read_mem(uintptr_t addr, void *buf, size_t size);
    oc::result<size_t> write_mem(uintptr_t addr, const void *buf, size_t size);
    oc::result<std::string> read_string(uintptr_t addr);

    // Child memory management

    oc::result<uintptr_t> mmap(uintptr_t addr, size_t length, int prot,
                               int flags, int fd, off64_t offset);
    oc::result<void> munmap(uintptr_t addr, size_t length);

    // Syscall injection

    oc::result<SysCallRet> inject_syscall(SysCallNum num,
                                          const SysCallArgs &args);
    oc::result<void> inject_syscall_async(SysCallNum num,
                                          const SysCallArgs &args);

    // Syscall modification

    oc::result<void> modify_syscall_args(SysCallNum num,
                                         const SysCallArgs &args);
    oc::result<void> modify_syscall_ret(SysCallNum num, SysCallRet ret);

    oc::result<void> suppress_syscall(SysCallRet ret);

    //! Thread group ID (i.e. process ID)
    pid_t tgid;

    //! Thread ID
    pid_t tid;

private:
    //! Pointer to tracer
    Tracer *m_tracer;

    //! Tracee state
    TraceeState m_state;

    //! Whether the tracee is executing in user space or kernel space
    ExecMode m_exec_mode;

    //! Registers prior to execution of syscall-entry/exit hooks
    ArchRegs m_regs;

    //! Syscall injection status
    SysCallStatus m_sc_status;

    //! Whether the tracee was executing in user space or kernel space prior to
    //! injection
    ExecMode m_inj_exec_mode;

    //! Backup of registers for use during injection
    ArchRegs m_inj_regs;

    //! Original syscall number before suppression
    std::optional<SysCallNum> m_suppress_orig_num;

    //! Fake return value for suppressed syscall
    SysCallRet m_suppress_ret;

#if defined(__arm__) || defined(__aarch64__)
    //! Whether the last syscall-entry stop was for execve
    bool m_started_execve;
#endif

    oc::result<void> proceed_until_syscall();

    oc::result<void> continue_exec_raw(int signal);

    // Asynchronous syscall injection

    oc::result<SysCallRet> inject_syscall_async_end();

    oc::result<void> inject_syscall_entry_start(SysCallNum num,
                                                const SysCallArgs &args);
    oc::result<SysCallRet> inject_syscall_entry_end(bool will_repeat);

    oc::result<void> inject_syscall_exit_start(SysCallNum num,
                                               const SysCallArgs &args);
    oc::result<SysCallRet> inject_syscall_exit_end();

    // Syscall modification

    oc::result<void> suppress_syscall_post();
};

}
