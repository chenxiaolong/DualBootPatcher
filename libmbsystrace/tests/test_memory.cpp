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

#include <fcntl.h>

#include "mbcommon/finally.h"

#include "mbsystrace/syscalls.h"
#include "mbsystrace/tracee.h"
#include "mbsystrace/tracer.h"

using namespace mb;
using namespace mb::systrace;

TEST(MemoryTest, ReadBytes)
{
    static constexpr char expected[] = "foobar";

    std::string buf;
    Hooks hooks;

    hooks.syscall_entry = [&](auto tracee, auto &info)
            -> SysCallEntryAction {
        if (strcmp(info.syscall.name(), "write") == 0) {
            buf.resize(info.args[2]);

            (void) tracee->read_mem(info.args[1], buf.data(), buf.size());

            return action::SuppressSysCall{0};
        } else {
            return action::ContinueExec{};
        }
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        MB_IGNORE_VALUE(write(-1, expected, strlen(expected)));
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_EQ(buf, expected);
}

TEST(MemoryTest, WriteBytes)
{
    int fds[2];
    ASSERT_EQ(pipe2(fds, O_CLOEXEC), 0);

    auto close_fds = finally([&] {
        close(fds[0]);
        close(fds[1]);
    });

    std::string buf;
    Hooks hooks;

    hooks.syscall_entry = [&](auto tracee, auto &info) {
        if (strcmp(info.syscall.name(), "write") == 0) {
            buf.resize(info.args[2]);

            (void) tracee->read_mem(info.args[1], buf.data(), buf.size());

            std::reverse(buf.begin(), buf.end());

            (void) tracee->write_mem(info.args[1], buf.data(), buf.size());

            buf.clear();
            buf.resize(info.args[2]);
        }

        return action::ContinueExec{};
    };
    hooks.syscall_exit = [&](auto, auto &info) {
        if (strcmp(info.syscall.name(), "write") == 0) {
            MB_IGNORE_VALUE(read(fds[0], buf.data(), buf.size()));
        }

        return action::ContinueExec{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        // Use mutable char array since the tracer needs to write to the
        // string's memory
        char str[] = "foobar";
        MB_IGNORE_VALUE(write(fds[1], str, strlen(str)));
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_EQ(buf, "raboof");
}

TEST(MemoryTest, ReadSmallNullTerminatedString)
{
    static constexpr char expected[] = "foobar";

    std::string buf;
    Hooks hooks;

    hooks.syscall_entry = [&](auto tracee, auto &info)
            -> SysCallEntryAction {
        if (strcmp(info.syscall.name(), "chdir") == 0) {
            buf = tracee->read_string(info.args[0]).value();

            return action::SuppressSysCall{0};
        } else {
            return action::ContinueExec{};
        }
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        MB_IGNORE_VALUE(chdir(expected));
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_EQ(buf, expected);
}

TEST(MemoryTest, ReadLargeNullTerminatedString)
{
    // Ensure string spans multiple pages
    std::string expected(54321, 'x');

    std::string buf;
    Hooks hooks;

    hooks.syscall_entry = [&](auto tracee, auto &info)
            -> SysCallEntryAction {
        if (strcmp(info.syscall.name(), "chdir") == 0) {
            buf = tracee->read_string(info.args[0]).value();

            return action::SuppressSysCall{0};
        } else {
            return action::ContinueExec{};
        }
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        MB_IGNORE_VALUE(chdir(expected.c_str()));
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_EQ(buf, expected);
}
