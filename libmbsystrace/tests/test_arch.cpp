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

#include <gtest/gtest.h>

#include <sys/syscall.h>

#include "mbcommon/string.h"

#include "mbsystrace/arch.h"
#include "mbsystrace/tracee.h"
#include "mbsystrace/tracer.h"

using namespace mb;
using namespace mb::systrace;

TEST(ArchTest, ArchRegsRoundTripAllAbis)
{
    enum Truncated : uint8_t
    {
        Args    = 1 << 0,
        Ret     = 1 << 1,
        Syscall = 1 << 2,
        Ip      = 1 << 3,
    };

    struct {
        ArchAbi abi;
        uint8_t trunc;
    } abis[] = {
#if defined(__x86_64__)
        { ArchAbi::X86_64,  0                         },
        { ArchAbi::X86_32,  Args | Ret | Syscall | Ip },
        { ArchAbi::X32,     Args                      },
#elif defined(__i386__)
        { ArchAbi::X86_32,  0                         },
#elif defined(__aarch64__)
        { ArchAbi::Aarch64, 0                         },
        { ArchAbi::Eabi,    Args | Ret | Syscall | Ip },
#elif defined(__arm__)
        { ArchAbi::Eabi,    0                         },
#endif
    };

    for (auto abi : abis) {
        ArchRegs ar(abi.abi);

        auto u_num = std::numeric_limits<KernelULong>::max();
        auto s_num = std::numeric_limits<KernelSLong>::max();

        // Assume truncated is 32 bits
        auto ut_num = std::numeric_limits<uint32_t>::max();
        auto st_num = -1;

        std::string details = format("[abi=%d, trunc=0x%x]",
                                     static_cast<int>(abi.abi), abi.trunc);

        if (abi.trunc & Args) {
            ar.set_arg0(u_num);
            EXPECT_EQ(ar.arg0(), ut_num) << details;
            ar.set_arg0(ut_num);
            EXPECT_EQ(ar.arg0(), ut_num) << details;
            ar.set_arg1(u_num);
            EXPECT_EQ(ar.arg1(), ut_num) << details;
            ar.set_arg1(ut_num);
            EXPECT_EQ(ar.arg1(), ut_num) << details;
            ar.set_arg2(u_num);
            EXPECT_EQ(ar.arg2(), ut_num) << details;
            ar.set_arg2(ut_num);
            EXPECT_EQ(ar.arg2(), ut_num) << details;
            ar.set_arg3(u_num);
            EXPECT_EQ(ar.arg3(), ut_num) << details;
            ar.set_arg3(ut_num);
            EXPECT_EQ(ar.arg3(), ut_num) << details;
            ar.set_arg4(u_num);
            EXPECT_EQ(ar.arg4(), ut_num) << details;
            ar.set_arg4(ut_num);
            EXPECT_EQ(ar.arg4(), ut_num) << details;
            ar.set_arg5(u_num);
            EXPECT_EQ(ar.arg5(), ut_num) << details;
            ar.set_arg5(ut_num);
            EXPECT_EQ(ar.arg5(), ut_num) << details;
        } else {
            ar.set_arg0(u_num);
            EXPECT_EQ(ar.arg0(), u_num) << details;
            ar.set_arg1(u_num);
            EXPECT_EQ(ar.arg1(), u_num) << details;
            ar.set_arg2(u_num);
            EXPECT_EQ(ar.arg2(), u_num) << details;
            ar.set_arg3(u_num);
            EXPECT_EQ(ar.arg3(), u_num) << details;
            ar.set_arg4(u_num);
            EXPECT_EQ(ar.arg4(), u_num) << details;
            ar.set_arg5(u_num);
            EXPECT_EQ(ar.arg5(), u_num) << details;
        }

        if (abi.trunc & Ret) {
            ar.set_ret(s_num);
            EXPECT_EQ(ar.ret(), st_num) << details;
            ar.set_ret(st_num);
            EXPECT_EQ(ar.ret(), st_num) << details;
        } else {
            ar.set_ret(s_num);
            EXPECT_EQ(ar.ret(), s_num) << details;
        }

        if (abi.trunc & Syscall) {
            ar.set_ptrace_syscall(u_num);
            EXPECT_EQ(ar.ptrace_syscall(), ut_num) << details;
            ar.set_ptrace_syscall(ut_num);
            EXPECT_EQ(ar.ptrace_syscall(), ut_num) << details;
            ar.set_pending_syscall(u_num);
            EXPECT_EQ(ar.pending_syscall(), ut_num) << details;
            ar.set_pending_syscall(ut_num);
            EXPECT_EQ(ar.pending_syscall(), ut_num) << details;
        } else {
            ar.set_ptrace_syscall(u_num);
            EXPECT_EQ(ar.ptrace_syscall(), u_num) << details;
            ar.set_pending_syscall(u_num);
            EXPECT_EQ(ar.pending_syscall(), u_num) << details;
        }

        if (abi.trunc & Ip) {
            ar.set_ip(u_num);
            EXPECT_EQ(ar.ip(), ut_num) << details;
            ar.set_ip(ut_num);
            EXPECT_EQ(ar.ip(), ut_num) << details;
        } else {
            ar.set_ip(u_num);
            EXPECT_EQ(ar.ip(), u_num) << details;
        }
    }
}

TEST(ArchTest, ReadWriteRegsRoundTrip)
{
    std::error_code ec;
    ArchRegs entry_regs_pre;
    ArchRegs entry_regs_post;
    ArchRegs exit_regs_pre;
    ArchRegs exit_regs_post;

    Hooks hooks;

    hooks.syscall_entry = [&](auto tracee, auto &) -> SysCallEntryAction {
        auto ret = [&]() -> oc::result<void> {
            OUTCOME_TRY(regs, read_regs(tracee->tid));

            entry_regs_pre = regs;
            entry_regs_pre.set_arg0(1);
            entry_regs_pre.set_arg1(1);
            entry_regs_pre.set_arg2(2);
            entry_regs_pre.set_arg3(3);
            entry_regs_pre.set_arg4(4);
            entry_regs_pre.set_arg5(5);
            entry_regs_pre.set_ptrace_syscall(6);
            entry_regs_pre.set_ip(7);

            OUTCOME_TRYV(write_regs(tracee->tid, entry_regs_pre));
            OUTCOME_TRY(new_regs, read_regs(tracee->tid));

            entry_regs_post = new_regs;

            OUTCOME_TRYV(write_regs(tracee->tid, regs));

            return oc::success();
        }();

        if (ret) {
            return action::Default{};
        } else {
            ec = ret.error();
            return action::KillTracee{SIGKILL};
        }
    };

    hooks.syscall_exit = [&](auto tracee, auto &) -> SysCallExitAction {
        auto ret = [&]() -> oc::result<void> {
            OUTCOME_TRY(regs, read_regs(tracee->tid));

            exit_regs_pre = regs;
            exit_regs_pre.set_ret(1);
            exit_regs_pre.set_ptrace_syscall(2);
            exit_regs_pre.set_ip(3);

            OUTCOME_TRYV(write_regs(tracee->tid, exit_regs_pre));
            OUTCOME_TRY(new_regs, read_regs(tracee->tid));

            exit_regs_post = new_regs;

            OUTCOME_TRYV(write_regs(tracee->tid, regs));

            return oc::success();
        }();

        if (!ret) {
            ec = ret.error();
        }

        return action::KillTracee{SIGKILL};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        syscall(SYS_getpid);
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_FALSE(ec) << ec.message();
    EXPECT_EQ(entry_regs_pre.arg0(), entry_regs_post.arg0());
    EXPECT_EQ(entry_regs_pre.arg1(), entry_regs_post.arg1());
    EXPECT_EQ(entry_regs_pre.arg2(), entry_regs_post.arg2());
    EXPECT_EQ(entry_regs_pre.arg3(), entry_regs_post.arg3());
    EXPECT_EQ(entry_regs_pre.arg4(), entry_regs_post.arg4());
    EXPECT_EQ(entry_regs_pre.arg5(), entry_regs_post.arg5());
#if !defined(__arm__) && !defined(__aarch64__)
    EXPECT_EQ(entry_regs_pre.ptrace_syscall(), entry_regs_post.ptrace_syscall());
#endif
    EXPECT_EQ(entry_regs_pre.ip(), entry_regs_post.ip());
    EXPECT_EQ(exit_regs_pre.ret(), exit_regs_post.ret());
#if !defined(__arm__) && !defined(__aarch64__)
    EXPECT_EQ(exit_regs_pre.ptrace_syscall(), exit_regs_post.ptrace_syscall());
#endif
    EXPECT_EQ(exit_regs_pre.ip(), exit_regs_post.ip());
}
