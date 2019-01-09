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

#include <sys/syscall.h>

#include "mbcommon/integer.h"

#include "mbsystrace/registers_p.h"

#define X86_64_ARG0             x86_64.rdi
#define X86_64_ARG1             x86_64.rsi
#define X86_64_ARG2             x86_64.rdx
#define X86_64_ARG3             x86_64.r10
#define X86_64_ARG4             x86_64.r8
#define X86_64_ARG5             x86_64.r9
#define X86_64_RET              x86_64.rax
#define X86_64_SYSCALL          x86_64.orig_rax
#define X86_64_PENDING_SYSCALL  x86_64.rax
#define X86_64_IP               x86_64.rip

#define I386_ARG0               i386.ebx
#define I386_ARG1               i386.ecx
#define I386_ARG2               i386.edx
#define I386_ARG3               i386.esi
#define I386_ARG4               i386.edi
#define I386_ARG5               i386.ebp
#define I386_RET                i386.eax
#define I386_SYSCALL            i386.orig_eax
#define I386_PENDING_SYSCALL    i386.eax
#define I386_IP                 i386.eip

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
    switch (m_abi) {
    case ArchAbi::X86_64:
        return m_regs.X86_64_ARG0;
    case ArchAbi::X86_32:
        return make_unsigned_v(m_regs.I386_ARG0);
    case ArchAbi::X32:
        return static_cast<uint32_t>(m_regs.X86_64_ARG0);
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_arg0(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::X86_64:
        m_regs.X86_64_ARG0 = value;
        break;
    case ArchAbi::X86_32:
        m_regs.I386_ARG0 = static_cast<int32_t>(value);
        break;
    case ArchAbi::X32:
        m_regs.X86_64_ARG0 = static_cast<uint32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::arg1() const
{
    switch (m_abi) {
    case ArchAbi::X86_64:
        return m_regs.X86_64_ARG1;
    case ArchAbi::X86_32:
        return make_unsigned_v(m_regs.I386_ARG1);
    case ArchAbi::X32:
        return static_cast<uint32_t>(m_regs.X86_64_ARG1);
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_arg1(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::X86_64:
        m_regs.X86_64_ARG1 = value;
        break;
    case ArchAbi::X86_32:
        m_regs.I386_ARG1 = static_cast<int32_t>(value);
        break;
    case ArchAbi::X32:
        m_regs.X86_64_ARG1 = static_cast<uint32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::arg2() const
{
    switch (m_abi) {
    case ArchAbi::X86_64:
        return m_regs.X86_64_ARG2;
    case ArchAbi::X86_32:
        return make_unsigned_v(m_regs.I386_ARG2);
    case ArchAbi::X32:
        return static_cast<uint32_t>(m_regs.X86_64_ARG2);
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_arg2(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::X86_64:
        m_regs.X86_64_ARG2 = value;
        break;
    case ArchAbi::X86_32:
        m_regs.I386_ARG2 = static_cast<int32_t>(value);
        break;
    case ArchAbi::X32:
        m_regs.X86_64_ARG2 = static_cast<uint32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::arg3() const
{
    switch (m_abi) {
    case ArchAbi::X86_64:
        return m_regs.X86_64_ARG3;
    case ArchAbi::X86_32:
        return make_unsigned_v(m_regs.I386_ARG3);
    case ArchAbi::X32:
        return static_cast<uint32_t>(m_regs.X86_64_ARG3);
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_arg3(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::X86_64:
        m_regs.X86_64_ARG3 = value;
        break;
    case ArchAbi::X86_32:
        m_regs.I386_ARG3 = static_cast<int32_t>(value);
        break;
    case ArchAbi::X32:
        m_regs.X86_64_ARG3 = static_cast<uint32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::arg4() const
{
    switch (m_abi) {
    case ArchAbi::X86_64:
        return m_regs.X86_64_ARG4;
    case ArchAbi::X86_32:
        return make_unsigned_v(m_regs.I386_ARG4);
    case ArchAbi::X32:
        return static_cast<uint32_t>(m_regs.X86_64_ARG4);
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_arg4(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::X86_64:
        m_regs.X86_64_ARG4 = value;
        break;
    case ArchAbi::X86_32:
        m_regs.I386_ARG4 = static_cast<int32_t>(value);
        break;
    case ArchAbi::X32:
        m_regs.X86_64_ARG4 = static_cast<uint32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::arg5() const
{
    switch (m_abi) {
    case ArchAbi::X86_64:
        return m_regs.X86_64_ARG5;
    case ArchAbi::X86_32:
        return make_unsigned_v(m_regs.I386_ARG5);
    case ArchAbi::X32:
        return static_cast<uint32_t>(m_regs.X86_64_ARG5);
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_arg5(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::X86_64:
        m_regs.X86_64_ARG5 = value;
        break;
    case ArchAbi::X86_32:
        m_regs.I386_ARG5 = static_cast<int32_t>(value);
        break;
    case ArchAbi::X32:
        m_regs.X86_64_ARG5 = static_cast<uint32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelSLong ArchRegs::ret() const
{
    switch (m_abi) {
    case ArchAbi::X86_64:
    case ArchAbi::X32:
        // X32 uses signed 64-bit return values
        return make_signed_v(m_regs.X86_64_RET);
    case ArchAbi::X86_32:
        return m_regs.I386_RET;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_ret(KernelSLong value)
{
    switch (m_abi) {
    case ArchAbi::X86_64:
    case ArchAbi::X32:
        // X32 uses signed 64-bit return values
        m_regs.X86_64_RET = make_unsigned_v(value);
        break;
    case ArchAbi::X86_32:
        m_regs.I386_RET = static_cast<int32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::ptrace_syscall() const
{
    switch (m_abi) {
    case ArchAbi::X86_64:
    case ArchAbi::X32:
        // X32 syscall number is 64 bit
        return m_regs.X86_64_SYSCALL;
    case ArchAbi::X86_32:
        return static_cast<uint32_t>(m_regs.I386_SYSCALL);
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_ptrace_syscall(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::X86_64:
    case ArchAbi::X32:
        // X32 syscall number is 64 bit
        m_regs.X86_64_SYSCALL = value;
        break;
    case ArchAbi::X86_32:
        m_regs.I386_SYSCALL = static_cast<int32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::pending_syscall() const
{
    switch (m_abi) {
    case ArchAbi::X86_64:
    case ArchAbi::X32:
        // X32 syscall number is 64 bit
        return m_regs.X86_64_PENDING_SYSCALL;
    case ArchAbi::X86_32:
        return static_cast<uint32_t>(m_regs.I386_PENDING_SYSCALL);
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_pending_syscall(KernelULong value)
{
    switch (m_abi) {
    case ArchAbi::X86_64:
    case ArchAbi::X32:
        // X32 syscall number is 64 bit
        m_regs.X86_64_PENDING_SYSCALL = value;
        break;
    case ArchAbi::X86_32:
        m_regs.I386_PENDING_SYSCALL = static_cast<int32_t>(value);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

KernelULong ArchRegs::ip() const
{
    switch (m_abi) {
    case ArchAbi::X86_64:
    case ArchAbi::X32:
        return m_regs.X86_64_IP;
    case ArchAbi::X86_32:
        return static_cast<uint32_t>(m_regs.I386_IP);
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

void ArchRegs::set_ip(KernelULong addr)
{
    switch (m_abi) {
    case ArchAbi::X86_64:
    case ArchAbi::X32:
        m_regs.X86_64_IP = addr;
        break;
    case ArchAbi::X86_32:
        m_regs.I386_IP = static_cast<int32_t>(addr);
        break;
    default:
        MB_UNREACHABLE("Unknown ABI");
    }
}

static ArchAbi get_abi(const detail::RegsUnion &regs, size_t size)
{
    auto abi = NATIVE_ARCH_ABI;

    if (size == sizeof(regs.i386)) {
        abi = ArchAbi::X86_32;
    } else if (regs.X86_64_SYSCALL & __X32_SYSCALL_BIT
            && make_signed_v(regs.X86_64_SYSCALL) != -1) {
        abi = ArchAbi::X32;
    }

    return abi;
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
    iovec iov;
    iov.iov_base = const_cast<detail::RegsUnion *>(&ar.m_regs);
    iov.iov_len = ar.m_abi == ArchAbi::X86_32
            ? sizeof(ar.m_regs.i386) : sizeof(ar.m_regs.x86_64);

    return detail::write_raw_regs(tid, iov);
}

}
