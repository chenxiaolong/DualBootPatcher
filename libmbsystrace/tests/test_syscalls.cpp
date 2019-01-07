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

#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include <cerrno>

#include <sys/syscall.h>

#include "mbsystrace/syscalls.h"
#include "mbsystrace/tracee.h"
#include "mbsystrace/tracer.h"

using namespace mb::systrace;

TEST(SysCallsTest, CheckSysCallsListSanity)
{
    struct
    {
        const char *name;
        SysCallNum num;
        ArchAbi abi;
    } test_cases[] = {
#if defined(__x86_64__)
        {"getpid", 39, ArchAbi::X86_64},
        {"getpid", 0x40000027, ArchAbi::X32},
#endif
#if defined(__i386__) || defined(__x86_64__)
        {"getpid", 20, ArchAbi::X86_32},
#endif
#if defined(__aarch64__)
        {"getpid", 172, ArchAbi::Aarch64},
#endif
#if defined(__arm__) || defined(__aarch64__)
        {"getpid", 20, ArchAbi::Eabi},
        // Special ARM syscall
        {"get_tls", 0xf0006, ArchAbi::Eabi},
        // Special ARM syscall that was removed in kernel 4.4
        {"cmpxchg", 0xffff0, ArchAbi::Eabi},
#endif
    };

    for (auto const &tc : test_cases) {
        auto abi = static_cast<std::underlying_type_t<ArchAbi>>(tc.abi);

        SysCall sc(tc.name, tc.abi);
        ASSERT_TRUE(sc) << "[" << tc.name << ", " << abi << "] "
                << "Syscall is not valid";
        ASSERT_EQ(sc.num(), tc.num) << "[" << tc.name << ", " << abi << "] "
                << "Expected " << tc.num << ", but got " << sc.num();
    }
}

TEST(SysCallsTest, CheckEntryAndExitMatch)
{
    // Pair of (is entry?, syscall name)
    std::vector<std::pair<bool, std::string>> syscalls;

    Hooks hooks;

    hooks.syscall_entry = [&](auto, auto &info) {
        syscalls.emplace_back(true, info.syscall.name());
        return action::Default{};
    };
    hooks.syscall_exit = [&](auto, auto &info) {
        syscalls.emplace_back(false, info.syscall.name());
        return action::Default{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        for (int i = 0; i < 10; ++i) {
            syscall(SYS_getpid);
            syscall(SYS_gettid);
        }
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    std::optional<std::string> last_syscall;
    bool found_getpid;

    // Ensure alternating
    for (auto const &syscall : syscalls) {
        if (syscall.second == "getpid") {
            found_getpid = true;
        }

        if (last_syscall) {
            ASSERT_FALSE(syscall.first)
                    << "Expected syscall-exit, but got syscall-entry for "
                    << syscall.second;
            ASSERT_EQ(last_syscall, syscall.second)
                    << "Expected syscall-exit for " << *last_syscall
                    << ", but got " << syscall.second;
            last_syscall = std::nullopt;
        } else {
            ASSERT_TRUE(syscall.first)
                    << "Expected syscall-entry, but got syscall-exit for "
                    << syscall.second;
            last_syscall = syscall.second;
        }
    }

    ASSERT_TRUE(found_getpid) << "Called getpid, but received no callback";
}

TEST(SysCallsTest, DenySysCall)
{
    int exit_code = -1;

    Hooks hooks;

    hooks.syscall_entry = [](auto, auto &info) -> SysCallEntryAction {
        if (strcmp(info.syscall.name(), "close") == 0) {
            return action::SuppressSysCall{-EPERM};
        }

        return action::ContinueExec{};
    };
    hooks.tracee_exit = [&](auto, auto ec) -> TraceeExitAction {
        exit_code = ec;
        return action::NoAction{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        if (close(STDOUT_FILENO) < 0 && errno == EPERM) {
            exit(EXIT_SUCCESS);
        } else {
            exit(EXIT_FAILURE);
        }
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_EQ(exit_code, EXIT_SUCCESS);
}

TEST(SysCallsTest, ModifySysCallArgs)
{
    int exit_code = -1;

    Hooks hooks;

    hooks.syscall_entry = [](auto tracee, auto &info) {
        if (strcmp(info.syscall.name(), "read") == 0) {
            SysCall sc("exit_group", info.syscall.abi());
            assert(sc);
            (void) tracee->modify_syscall_args(sc.num(), info.args);
        }

        return action::ContinueExec{};
    };
    hooks.tracee_exit = [&](auto, auto ec) -> TraceeExitAction {
        exit_code = ec;
        return action::NoAction{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        read(7, nullptr, 0);
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_EQ(exit_code, 7);
}

TEST(SysCallsTest, ModifySysCallRet)
{
    int exit_code = -1;

    Hooks hooks;

    hooks.syscall_exit = [](auto tracee, auto &info) {
        if (strcmp(info.syscall.name(), "getpid") == 0) {
            SysCall sc("exit_group", info.syscall.abi());
            assert(sc);
            (void) tracee->modify_syscall_ret(sc.num(), -1);
        }

        return action::ContinueExec{};
    };
    hooks.tracee_exit = [&](auto, auto ec) -> TraceeExitAction {
        exit_code = ec;
        return action::NoAction{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        if (syscall(SYS_getpid) < 0) {
            _exit(7);
        }
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_EQ(exit_code, 7);
}
