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

#include <memory>

#include <climits>
#include <cstdio>

#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>

#include "mbcommon/finally.h"
#include "mbcommon/integer.h"

#include "mbsystrace/procfs_p.h"
#include "mbsystrace/syscalls.h"
#include "mbsystrace/tracee.h"
#include "mbsystrace/tracer.h"

using namespace mb;
using namespace mb::systrace;
using namespace mb::systrace::detail;

using ScopedFILE = std::unique_ptr<FILE, decltype(fclose) *>;

static pid_t get_tracer(pid_t pid)
{
    if (auto r = get_pid_status_field(pid, "TracerPid")) {
        return r.value();
    } else {
        return -1;
    }
}

TEST(AttachDetachTest, AttachToProcess)
{
    pid_t pid = fork();
    if (pid == 0) {
        while (true) {
            pause();
        }
    }

    ASSERT_GE(pid, 0);

    auto kill_pid = finally([&] {
        int status;
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
    });

    Hooks hooks;

    hooks.syscall_entry = [](auto tracee, auto &info)
            -> SysCallEntryAction {
        auto exit_nr = SysCall("exit_group", info.syscall.abi());
        assert(exit_nr);
        (void) tracee->modify_syscall_args(exit_nr.num(), {{42, 0, 0, 0, 0, 0}});

        return action::DetachTracee{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.attach(pid));
    ASSERT_TRUE(tracer.execute(hooks));

    int status;
    ASSERT_GE(waitpid(pid, &status, 0), 0);
    ASSERT_TRUE(WIFEXITED(status));

    kill_pid.dismiss();

    ASSERT_EQ(WEXITSTATUS(status), 42);
}

TEST(AttachDetachTest, AttachToStoppedProcess)
{
    pid_t pid = fork();
    if (pid == 0) {
        while (true) {
            pause();
        }
    }

    ASSERT_GE(pid, 0);
    ASSERT_EQ(kill(pid, SIGSTOP), 0);

    auto kill_pid = finally([&] {
        int status;
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
    });

    Tracer tracer;
    Hooks hooks;

    hooks.group_stop = [&](auto tracee, auto signal) -> GroupStopAction {
        if (signal == SIGSTOP) {
            (void) tracee->signal_process(SIGCONT);
            (void) tracee->signal_process(SIGTERM);
            return action::DetachTracee{};
        }
        return action::Default{};
    };

    ASSERT_TRUE(tracer.attach(pid));
    ASSERT_TRUE(tracer.execute(hooks));

    int status;
    ASSERT_GE(waitpid(pid, &status, 0), 0);
    ASSERT_TRUE(WIFSIGNALED(status));

    kill_pid.dismiss();

    ASSERT_EQ(WTERMSIG(status), SIGTERM);
}

TEST(AttachDetachTest, DetachUnhookedProcess)
{
    pid_t pid = fork();
    if (pid == 0) {
        while (true);
    }

    ASSERT_GE(pid, 0);

    auto kill_pid = finally([&] {
        int status;
        kill(pid, SIGKILL);
        waitpid(pid, &status, 0);
    });

    {
        Tracer tracer;
        ASSERT_TRUE(tracer.attach(pid));
        ASSERT_EQ(get_tracer(pid), getpid());
    }

    ASSERT_EQ(get_tracer(pid), 0);
}

TEST(AttachDetachTest, KillUnhookedProcess)
{
    pid_t tid;

    {
        Tracer tracer;

        auto process = tracer.fork([] {
            while (true);
        });
        ASSERT_TRUE(process);

        tid = process.value()->tid;
    }

    // Process should be killed during destruction. This check is racy, but is
    // extremely unlikely to fail due to TIDs always increasing (and wrapping).
    ASSERT_EQ(get_tgid(tid), oc::failure(std::errc::no_such_file_or_directory));
}
