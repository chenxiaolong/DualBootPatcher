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

#include <gmock/gmock.h>

#include <optional>

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>

#include "mbcommon/finally.h"

#include "mbsystrace/tracee.h"
#include "mbsystrace/tracer.h"

using namespace mb;
using namespace mb::systrace;


// Tests for certain hooks are grouped with other tests
// - NewTraceeHook       - test_new_process.cpp
// - TraceeDisappearHook - test_execve.cpp
// - TraceeSignalHook    - test_signals.cpp
// - GroupStopHook       - test_signals.cpp
// - InterruptStopHook   - test_signals.cpp
// - SysCallEntryHook    - test_syscalls.cpp
// - SysCallExitHook     - test_syscalls.cpp

TEST(HooksTest, TraceeExitHook)
{
    std::optional<int> exit_code;

    Hooks hooks;

    hooks.tracee_exit = [&](auto, auto ec) {
        exit_code = ec;
        return action::Default{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        _exit(10);
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_EQ(exit_code, 10);
}

TEST(HooksTest, TraceeDeathHook)
{
    std::optional<int> death_signal;

    Hooks hooks;

    hooks.tracee_death = [&](auto, auto signal) {
        death_signal = signal;
        return action::Default{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        raise(SIGTERM);
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_EQ(death_signal, SIGTERM);
}

TEST(HooksTest, UnknownChildHook)
{
    pid_t child_pid = fork();
    if (child_pid == 0) {
        _exit(0);
    }

    ASSERT_GE(child_pid, 0);

    std::optional<pid_t> exited_pid;
    std::optional<pid_t> unknown_pid;

    Tracer tracer;

    // Tracee must remain alive long enough for child_pid to exit
    auto tracee = tracer.fork([] { while (true) pause(); });
    ASSERT_TRUE(tracee);

    auto tracee_tid = tracee.value()->tid;

    Hooks hooks;

    hooks.syscall_entry = [&](auto t, auto &info) {
        if (unknown_pid) {
            SysCall sc("exit_group", info.syscall.abi());
            assert(sc);
            (void) t->modify_syscall_args(sc.num(), {});
        }
        return action::Default{};
    };

    hooks.tracee_exit = [&](auto pid, auto) {
        exited_pid = pid;
        return action::Default{};
    };

    hooks.unknown_child = [&](auto pid, auto) {
        unknown_pid = pid;

        // tracee must be a valid pointer because this hook would not have been
        // called if the only tracee exited or died
        (void) tracee.value()->interrupt();
    };

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_EQ(exited_pid, tracee_tid);
    ASSERT_EQ(unknown_pid, child_pid);
}
