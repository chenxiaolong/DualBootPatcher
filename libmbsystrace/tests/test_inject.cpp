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

#include <fcntl.h>
#include <sys/mman.h>

#include "mbcommon/finally.h"

#include "mbsystrace/syscalls.h"
#include "mbsystrace/tracee.h"
#include "mbsystrace/tracer.h"

using namespace mb;
using namespace mb::systrace;

TEST(InjectTest, AsynchronousInjection)
{
    std::vector<SysCallStatus> entry_statuses;
    std::vector<SysCallStatus> exit_statuses;

    Hooks hooks;

    hooks.syscall_entry = [&](auto tracee, auto &info) {
        if (strcmp(info.syscall.name(), "write") == 0) {
            entry_statuses.push_back(info.status);

            if (info.status == SysCallStatus::Normal) {
                // Duplicate the syscall
                (void) tracee->inject_syscall_async(
                        SysCall("write", info.syscall.abi()).num(), info.args);
            }
        }

        return action::ContinueExec{};
    };
    hooks.syscall_exit = [&](auto, auto &info) {
        if (strcmp(info.syscall.name(), "write") == 0) {
            exit_statuses.push_back(info.status);
        }

        return action::ContinueExec{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        MB_IGNORE_VALUE(write(-1, "foobar", 6));
    }));
    ASSERT_TRUE(tracer.execute(hooks));

    EXPECT_THAT(entry_statuses, testing::ElementsAre(
            SysCallStatus::Normal, SysCallStatus::Repeated));
    EXPECT_THAT(exit_statuses, testing::ElementsAre(
            SysCallStatus::Injected, SysCallStatus::Normal));
}

TEST(InjectTest, SynchronousInjection)
{
    int fds[2];
    ASSERT_EQ(pipe2(fds, O_CLOEXEC), 0);

    auto close_fds = finally([&] {
        close(fds[0]);
        close(fds[1]);
    });

    auto write_fd = static_cast<SysCallArg>(-1);

    Hooks hooks;

    hooks.syscall_entry = [&](auto tracee, auto &info) -> SysCallEntryAction {
        if (strcmp(info.syscall.name(), "write") == 0) {
            static constexpr char text[] = "[begin]";

            write_fd = info.args[0];

            auto addr = tracee->mmap(0, 4096, PROT_WRITE | PROT_READ,
                                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0).value();

            (void) tracee->write_mem(addr, text, strlen(text));

            (void) tracee->inject_syscall(
                    SysCall("write", info.syscall.abi()).num(),
                    {write_fd, addr, strlen(text), 0, 0, 0});

            (void) tracee->munmap(addr, 4096);
        }

        return action::ContinueExec{};
    };
    hooks.syscall_exit = [&](auto tracee, auto &info) -> SysCallExitAction {
        if (strcmp(info.syscall.name(), "write") == 0) {
            static constexpr char text[] = "[end]";

            auto addr = tracee->mmap(0, 4096, PROT_WRITE | PROT_READ,
                                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0).value();

            (void) tracee->write_mem(addr, text, strlen(text));

            (void) tracee->inject_syscall(
                    SysCall("write", info.syscall.abi()).num(),
                    {write_fd, addr, strlen(text), 0, 0, 0});

            (void) tracee->munmap(addr, 4096);
        }

        return action::ContinueExec{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        dup3(fds[1], STDOUT_FILENO, O_CLOEXEC);
        MB_IGNORE_VALUE(write(fds[1], "foobar", 6));
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    close(fds[1]);

    std::string result;
    char buf[4096];

    while (true) {
        auto n = read(fds[0], buf, sizeof(buf));
        ASSERT_GE(n, 0);

        if (n == 0) {
            break;
        }

        result += std::string_view(buf, static_cast<size_t>(n));
    }

    ASSERT_EQ(result, "[begin]foobar[end]");
}
