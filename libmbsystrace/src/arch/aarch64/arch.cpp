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

#include <linux/elf.h>
#include <sys/ptrace.h>
#include <sys/uio.h>

#include "mbcommon/error_code.h"
#include "mbcommon/integer.h"

#include "mbsystrace/registers_p.h"

#define AARCH64_ARG0            aarch64.regs[0]
#define AARCH64_ARG1            aarch64.regs[1]
#define AARCH64_ARG2            aarch64.regs[2]
#define AARCH64_ARG3            aarch64.regs[3]
#define AARCH64_ARG4            aarch64.regs[4]
#define AARCH64_ARG5            aarch64.regs[5]
#define AARCH64_RET             aarch64.regs[0]
#define AARCH64_SYSCALL         aarch64.regs[8]
#define AARCH64_PENDING_SYSCALL aarch64.regs[8]
#define AARCH64_IP              aarch64.pc

#define ARM_ARG0                arm.uregs[0] // ARM_r0
#define ARM_ARG1                arm.uregs[1] // ARM_r1
#define ARM_ARG2                arm.uregs[2] // ARM_r2
#define ARM_ARG3                arm.uregs[3] // ARM_r3
#define ARM_ARG4                arm.uregs[4] // ARM_r4
#define ARM_ARG5                arm.uregs[5] // ARM_r5
#define ARM_RET                 arm.uregs[0] // ARM_r0
#define ARM_SYSCALL             arm.uregs[7] // ARM_r7
#define ARM_PENDING_SYSCALL     arm.uregs[7] // ARM_r7
#define ARM_IP                  arm.uregs[15] // ARM_pc

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
    switch (m_abi) {
    case ArchAbi::Aarch64:
        return m_regs.AARCH64_ARG0;
    case ArchAbi::Eabi:
        return m_regs.ARM_ARG0;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_arg0(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        m_regs.AARCH64_ARG0 = value;
        break;
    case ArchAbi::Eabi:
        m_regs.ARM_ARG0 = static_cast<uint32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::arg1() const
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        return m_regs.AARCH64_ARG1;
    case ArchAbi::Eabi:
        return m_regs.ARM_ARG1;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_arg1(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        m_regs.AARCH64_ARG1 = value;
        break;
    case ArchAbi::Eabi:
        m_regs.ARM_ARG1 = static_cast<uint32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::arg2() const
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        return m_regs.AARCH64_ARG2;
    case ArchAbi::Eabi:
        return m_regs.ARM_ARG2;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_arg2(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        m_regs.AARCH64_ARG2 = value;
        break;
    case ArchAbi::Eabi:
        m_regs.ARM_ARG2 = static_cast<uint32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::arg3() const
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        return m_regs.AARCH64_ARG3;
    case ArchAbi::Eabi:
        return m_regs.ARM_ARG3;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_arg3(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        m_regs.AARCH64_ARG3 = value;
        break;
    case ArchAbi::Eabi:
        m_regs.ARM_ARG3 = static_cast<uint32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::arg4() const
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        return m_regs.AARCH64_ARG4;
    case ArchAbi::Eabi:
        return m_regs.ARM_ARG4;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_arg4(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        m_regs.AARCH64_ARG4 = value;
        break;
    case ArchAbi::Eabi:
        m_regs.ARM_ARG4 = static_cast<uint32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::arg5() const
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        return m_regs.AARCH64_ARG5;
    case ArchAbi::Eabi:
        return m_regs.ARM_ARG5;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_arg5(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        m_regs.AARCH64_ARG5 = value;
        break;
    case ArchAbi::Eabi:
        m_regs.ARM_ARG5 = static_cast<uint32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelSLong ArchRegs::ret() const
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        return make_signed_v(m_regs.AARCH64_RET);
    case ArchAbi::Eabi:
        return make_signed_v(m_regs.ARM_RET);
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_ret(KernelSLong value)
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        m_regs.AARCH64_RET = make_unsigned_v(value);
        break;
    case ArchAbi::Eabi:
        m_regs.ARM_RET = static_cast<uint32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::ptrace_syscall() const
{
    if (m_new_syscall) {
        return *m_new_syscall;
    }

    switch (m_abi) {
    case ArchAbi::Aarch64:
        return m_regs.AARCH64_SYSCALL;
    case ArchAbi::Eabi:
        return m_regs.ARM_SYSCALL;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_ptrace_syscall(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        m_new_syscall = value;
        break;
    case ArchAbi::Eabi:
        m_new_syscall = static_cast<uint32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::pending_syscall() const
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        return m_regs.AARCH64_PENDING_SYSCALL;
    case ArchAbi::Eabi:
        return m_regs.ARM_PENDING_SYSCALL;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_pending_syscall(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        m_regs.AARCH64_PENDING_SYSCALL = value;
        break;
    case ArchAbi::Eabi:
        m_regs.ARM_PENDING_SYSCALL = static_cast<uint32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::ip() const
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        return m_regs.AARCH64_IP;
    case ArchAbi::Eabi:
        return m_regs.ARM_IP;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_ip(KernelULong addr)
{
    switch (m_abi) {
    case ArchAbi::Aarch64:
        m_regs.AARCH64_IP = addr;
        break;
    case ArchAbi::Eabi:
        m_regs.ARM_IP = static_cast<uint32_t>(addr);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

static ArchAbi get_abi(const detail::RegsUnion &regs, size_t size)
{
    if (size == sizeof(regs.arm)) {
        return ArchAbi::Eabi;
    } else {
        return ArchAbi::Aarch64;
    }
}

oc::result<ArchRegs> read_regs(pid_t tid)
{
    ArchRegs ar;

    iovec iov;
    iov.iov_base = &ar.m_regs;
    iov.iov_len = sizeof(ar.m_regs);

    OUTCOME_TRYV(detail::read_raw_regs(tid, iov));

    ar.m_abi = get_abi(ar.m_regs, iov.iov_len);

    return std::move(ar);
}

oc::result<void> write_regs(pid_t tid, const ArchRegs &ar)
{
    if (ar.m_new_syscall) {
        uint32_t n = static_cast<uint16_t>(*ar.m_new_syscall);
        const iovec iov = { &n, sizeof(n) };

        if (ptrace(PTRACE_SETREGSET, tid, NT_ARM_SYSTEM_CALL, &iov) != 0) {
            return ec_from_errno();
        }
    }

    iovec iov;
    iov.iov_base = const_cast<detail::RegsUnion *>(&ar.m_regs);
    iov.iov_len = ar.m_abi == ArchAbi::Eabi
            ? sizeof(ar.m_regs.arm) : sizeof(ar.m_regs.aarch64);

    return detail::write_raw_regs(tid, iov);
}

}
