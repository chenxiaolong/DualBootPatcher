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

#include "mbsystrace/arch.h"

#include "mbcommon/integer.h"

#include "mbsystrace/registers_p.h"

#define I386_ARG0               ebx
#define I386_ARG1               ecx
#define I386_ARG2               edx
#define I386_ARG3               esi
#define I386_ARG4               edi
#define I386_ARG5               ebp
#define I386_RET                eax
#define I386_SYSCALL            orig_eax
#define I386_PENDING_SYSCALL    eax
#define I386_IP                 eip

namespace mb::systrace
{

ArchRegs::ArchRegs() : ArchRegs(NATIVE_ARCH_ABI)
{
}

ArchRegs::ArchRegs(ArchAbi abi) : m_abi(abi), m_regs()
{
}

ArchAbi ArchRegs::abi() const
{
    return m_abi;
}

void ArchRegs::set_abi(ArchAbi abi)
{
    m_abi = abi;
}

KernelULong ArchRegs::arg0() const
{
    return make_unsigned_v(m_regs.I386_ARG0);
}

void ArchRegs::set_arg0(KernelULong value)
{
    m_regs.I386_ARG0 = make_signed_v(value);
}

KernelULong ArchRegs::arg1() const
{
    return make_unsigned_v(m_regs.I386_ARG1);
}

void ArchRegs::set_arg1(KernelULong value)
{
    m_regs.I386_ARG1 = make_signed_v(value);
}

KernelULong ArchRegs::arg2() const
{
    return make_unsigned_v(m_regs.I386_ARG2);
}

void ArchRegs::set_arg2(KernelULong value)
{
    m_regs.I386_ARG2 = make_signed_v(value);
}

KernelULong ArchRegs::arg3() const
{
    return make_unsigned_v(m_regs.I386_ARG3);
}

void ArchRegs::set_arg3(KernelULong value)
{
    m_regs.I386_ARG3 = make_signed_v(value);
}

KernelULong ArchRegs::arg4() const
{
    return make_unsigned_v(m_regs.I386_ARG4);
}

void ArchRegs::set_arg4(KernelULong value)
{
    m_regs.I386_ARG4 = make_signed_v(value);
}

KernelULong ArchRegs::arg5() const
{
    return make_unsigned_v(m_regs.I386_ARG5);
}

void ArchRegs::set_arg5(KernelULong value)
{
    m_regs.I386_ARG5 = make_signed_v(value);
}

KernelSLong ArchRegs::ret() const
{
    return m_regs.I386_RET;
}

void ArchRegs::set_ret(KernelSLong value)
{
    m_regs.I386_RET = value;
}

KernelULong ArchRegs::ptrace_syscall() const
{
    return make_unsigned_v(m_regs.I386_SYSCALL);
}

void ArchRegs::set_ptrace_syscall(KernelULong value)
{
    m_regs.I386_SYSCALL = make_signed_v(value);
}

KernelULong ArchRegs::pending_syscall() const
{
    return make_unsigned_v(m_regs.I386_PENDING_SYSCALL);
}

void ArchRegs::set_pending_syscall(KernelULong value)
{
    m_regs.I386_PENDING_SYSCALL = make_signed_v(value);
}

KernelULong ArchRegs::ip() const
{
    return make_unsigned_v(m_regs.I386_IP);
}

void ArchRegs::set_ip(KernelULong addr)
{
    m_regs.I386_IP = make_signed_v(addr);
}

oc::result<ArchRegs> read_regs(pid_t tid)
{
    ArchRegs ar;

    iovec iov;
    iov.iov_base = &ar.m_regs;
    iov.iov_len = sizeof(ar.m_regs);

    OUTCOME_TRYV(detail::read_raw_regs(tid, iov));

    ar.m_abi = NATIVE_ARCH_ABI;

    return std::move(ar);
}

oc::result<void> write_regs(pid_t tid, const ArchRegs &ar)
{
    iovec iov;
    iov.iov_base = const_cast<user_regs_struct *>(&ar.m_regs);
    iov.iov_len = sizeof(ar.m_regs);

    return detail::write_raw_regs(tid, iov);
}

}
