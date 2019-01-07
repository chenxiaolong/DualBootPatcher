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

// Based on logic from strace

#include "mbsystrace/arch.h"

#include <sys/ptrace.h>

#include "mbcommon/error_code.h"
#include "mbcommon/integer.h"

#include "mbsystrace/registers_p.h"

#define ARM_ARG0            uregs[0] // ARM_r0
#define ARM_ARG1            uregs[1] // ARM_r1
#define ARM_ARG2            uregs[2] // ARM_r2
#define ARM_ARG3            uregs[3] // ARM_r3
#define ARM_ARG4            uregs[4] // ARM_r4
#define ARM_ARG5            uregs[5] // ARM_r5
#define ARM_RET             uregs[0] // ARM_r0
#define ARM_SYSCALL         uregs[7] // ARM_r7
#define ARM_PENDING_SYSCALL uregs[7] // ARM_r7
#define ARM_IP              uregs[15] // ARM_pc

namespace mb::systrace
{

ArchRegs::ArchRegs() : ArchRegs(NATIVE_ARCH_ABI)
{
}

ArchRegs::ArchRegs(ArchAbi abi) : m_abi(abi), m_regs(), m_new_syscall()
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
    return m_regs.ARM_ARG0;
}

void ArchRegs::set_arg0(KernelULong value)
{
    m_regs.ARM_ARG0 = value;
}

KernelULong ArchRegs::arg1() const
{
    return m_regs.ARM_ARG1;
}

void ArchRegs::set_arg1(KernelULong value)
{
    m_regs.ARM_ARG1 = value;
}

KernelULong ArchRegs::arg2() const
{
    return m_regs.ARM_ARG2;
}

void ArchRegs::set_arg2(KernelULong value)
{
    m_regs.ARM_ARG2 = value;
}

KernelULong ArchRegs::arg3() const
{
    return m_regs.ARM_ARG3;
}

void ArchRegs::set_arg3(KernelULong value)
{
    m_regs.ARM_ARG3 = value;
}

KernelULong ArchRegs::arg4() const
{
    return m_regs.ARM_ARG4;
}

void ArchRegs::set_arg4(KernelULong value)
{
    m_regs.ARM_ARG4 = value;
}

KernelULong ArchRegs::arg5() const
{
    return m_regs.ARM_ARG5;
}

void ArchRegs::set_arg5(KernelULong value)
{
    m_regs.ARM_ARG5 = value;
}

KernelSLong ArchRegs::ret() const
{
    return make_signed_v(m_regs.ARM_RET);
}

void ArchRegs::set_ret(KernelSLong value)
{
    m_regs.ARM_RET = make_unsigned_v(value);
}

KernelULong ArchRegs::ptrace_syscall() const
{
    if (m_new_syscall) {
        return *m_new_syscall;
    }

    return m_regs.ARM_SYSCALL;
}

void ArchRegs::set_ptrace_syscall(KernelULong value)
{
    m_new_syscall = value;
}

KernelULong ArchRegs::pending_syscall() const
{
    return m_regs.ARM_PENDING_SYSCALL;
}

void ArchRegs::set_pending_syscall(KernelULong value)
{
    m_regs.ARM_PENDING_SYSCALL = value;
}

KernelULong ArchRegs::ip() const
{
    return m_regs.ARM_IP;
}

void ArchRegs::set_ip(KernelULong addr)
{
    m_regs.ARM_IP = addr;
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
    if (ar.m_new_syscall) {
        uint32_t n = static_cast<uint16_t>(*ar.m_new_syscall);

        if (ptrace(PTRACE_SET_SYSCALL, tid, nullptr, n) != 0) {
            return ec_from_errno();
        }
    }

    iovec iov;
    iov.iov_base = const_cast<user_regs *>(&ar.m_regs);
    iov.iov_len = sizeof(ar.m_regs);

    return detail::write_raw_regs(tid, iov);
}

}
