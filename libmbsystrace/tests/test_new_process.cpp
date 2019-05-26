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
#include <string_view>
#include <thread>
#include <unordered_set>
#include <utility>

#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "mbsystrace/tracee.h"
#include "mbsystrace/tracer.h"

using namespace mb::systrace;


struct NewProcessTest : public ::testing::Test
{
protected:
    void run_test(void (*fn)(), bool is_thread,
                  const std::unordered_set<std::string_view> &expect_one_of)
    {
        Tracer tracer;

        auto parent = tracer.fork(fn, Flag::TraceChildren);
        ASSERT_TRUE(parent);

        auto parent_tgid = parent.value()->tgid;
        std::optional<std::pair<pid_t, pid_t>> child;
        bool some_process_died = false;
        bool found_expected_syscall = false;

        Hooks hooks;

        hooks.new_tracee = [&](auto *tracee) {
            child = std::pair{tracee->tgid, tracee->tid};
            return action::Default{};
        };

        hooks.tracee_death = [&](pid_t, int) {
            some_process_died = true;
            return action::Default{};
        };

        hooks.syscall_entry = [&](auto, auto &info) {
            if (expect_one_of.find(info.syscall.name())
                    != expect_one_of.end()) {
                found_expected_syscall = true;
            }
            return action::Default{};
        };

        ASSERT_TRUE(tracer.execute(hooks));

        ASSERT_FALSE(some_process_died);
        ASSERT_TRUE(found_expected_syscall);
        ASSERT_TRUE(child);

        if (is_thread) {
            ASSERT_EQ(parent_tgid, child->first);
            ASSERT_NE(child->first, child->second);
        } else {
            ASSERT_NE(parent_tgid, child->first);
            ASSERT_EQ(child->first, child->second);
        }
    }

    template<typename F>
    static void waitpid_wrapper(F new_pid_fn)
    {
        pid_t pid = new_pid_fn();
        if (pid < 0) {
            die_with_msg("create process");
        } else if (pid == 0) {
            _exit(0);
        } else {
            int status;

            if (TEMP_FAILURE_RETRY(waitpid(pid, &status, 0)) < 0) {
                die_with_msg("waitpid");
            }
        }
    }

    [[noreturn]] static void die_with_msg(const char *msg)
    {
        perror(msg);
        abort();
    }
};

TEST_F(NewProcessTest, VerifyTgidAfterClone)
{
    run_test([] {
        // We can't use syscall(SYS_clone, ...) because it has no special
        // handling for the shared stack and using CLONE_THREAD without CLONE_VM
        // is not allowed.
        std::thread([] {}).join();
    }, true, {"clone"});
}

TEST_F(NewProcessTest, VerifyTgidAfterCloneWithForkSemantics)
{
    run_test([] {
        waitpid_wrapper([] {
            return static_cast<pid_t>(syscall(SYS_clone, SIGCHLD, 0));
        });
    }, false, {"clone"});
}

TEST_F(NewProcessTest, VerifyTgidAfterCloneWithVforkSemantics)
{
    run_test([] {
        // We can't use CLONE_VM (which would match vfork() behavior) because
        // syscall() has no special handling for the shared stack
        waitpid_wrapper([] {
            return static_cast<pid_t>(syscall(SYS_clone, CLONE_VFORK | SIGCHLD, 0));
        });
    }, false, {"clone"});
}

// x86 and x86_64 only
#ifdef SYS_fork
TEST_F(NewProcessTest, VerifyTgidAfterFork)
{
    run_test([] {
        waitpid_wrapper([] {
            return static_cast<pid_t>(syscall(SYS_fork));
        });
    }, false, {"fork"});
}
#endif

TEST_F(NewProcessTest, VerifyTgidAfterVfork)
{
    run_test([] {
        // We can't use syscall(SYS_vfork, ...) because it has no special
        // handling for the shared stack
        waitpid_wrapper(vfork);
    }, false, {"clone", "vfork"});
}
