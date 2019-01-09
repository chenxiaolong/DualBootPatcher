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

#include <sys/user.h>

#include "mbcommon/common.h"
#include "mbcommon/outcome.h"

#include "mbsystrace/types.h"

namespace mb::systrace
{

namespace detail
{
    struct UserRegsStructI386
    {
        int32_t ebx;
        int32_t ecx;
        int32_t edx;
        int32_t esi;
        int32_t edi;
        int32_t ebp;
        int32_t eax;
        int32_t xds;
        int32_t xes;
        int32_t xfs;
        int32_t xgs;
        int32_t orig_eax;
        int32_t eip;
        int32_t xcs;
        int32_t eflags;
        int32_t esp;
        int32_t xss;
    };

    union RegsUnion
    {
        user_regs_struct x86_64;
        UserRegsStructI386 i386;
    };
}

enum class ArchAbi : uint8_t
{
    X86_64 = 0,
    X86_32,
    X32,
};

constexpr ArchAbi NATIVE_ARCH_ABI = ArchAbi::X86_64;

constexpr size_t SYSCALL_OPSIZE = 2;

class ArchRegs
{
public:
    ArchRegs();
    ArchRegs(ArchAbi abi);

    MB_DEFAULT_COPY_CONSTRUCT_AND_ASSIGN(ArchRegs)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(ArchRegs)

    ArchAbi abi() const;
    void set_abi(ArchAbi abi);

    KernelULong arg0() const;
    void set_arg0(KernelULong value);

    KernelULong arg1() const;
    void set_arg1(KernelULong value);

    KernelULong arg2() const;
    void set_arg2(KernelULong value);

    KernelULong arg3() const;
    void set_arg3(KernelULong value);

    KernelULong arg4() const;
    void set_arg4(KernelULong value);

    KernelULong arg5() const;
    void set_arg5(KernelULong value);

    KernelSLong ret() const;
    void set_ret(KernelSLong value);

    KernelULong ptrace_syscall() const;
    void set_ptrace_syscall(KernelULong value);

    KernelULong pending_syscall() const;
    void set_pending_syscall(KernelULong value);

    KernelULong ip() const;
    void set_ip(KernelULong addr);

private:
    ArchAbi m_abi;
    detail::RegsUnion m_regs;

    friend oc::result<ArchRegs> read_regs(pid_t tid);
    friend oc::result<void> write_regs(pid_t tid, const ArchRegs &ar);
};

oc::result<ArchRegs> read_regs(pid_t tid);
oc::result<void> write_regs(pid_t tid, const ArchRegs &ar);

}
