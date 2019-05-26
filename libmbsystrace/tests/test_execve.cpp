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

#include <condition_variable>
#include <mutex>
#include <thread>
#include <unordered_set>
#include <vector>

#include <sys/mman.h>
#include <sys/syscall.h>

#include "mbcommon/common.h"

#include "mbsystrace/syscalls.h"
#include "mbsystrace/tracee.h"
#include "mbsystrace/tracer.h"

using namespace mb::systrace;
using namespace testing;


class Barrier
{
public:
    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(Barrier)

    explicit Barrier(size_t count)
        : m_total(count)
        , m_count(0)
        , m_generation()
    {
        assert(count > 0);
    }

    void wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        const bool generation = m_generation;

        if (++m_count == m_total) {
            m_generation = !m_generation;
            m_count = 0;
            lock.unlock();
            m_cv.notify_all();
        } else {
            m_cv.wait(lock, [&] { return generation != m_generation; });
        }
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    size_t m_total;
    size_t m_count;
    bool m_generation;
};

/*!
 * Spawn \p num_threads threads (including main thread) and then exec from the
 * \p exec_thread'th thread (where 0 is the main thread).
 *
 * It is guaranteed that all threads will be up and running prior to the execve
 * call.
 */
static void exec_from(size_t num_threads, size_t exec_thread)
{
    assert(num_threads >= 1);

    std::vector<std::thread> threads;
    Barrier barrier1(num_threads);
    Barrier barrier2(num_threads);

    auto thread_fn = [&](size_t thread_num) {
        // Double block here to give tracer a chance to detach a thread
        barrier1.wait();
        syscall(SYS_getpid);
        barrier2.wait();

        if (thread_num == exec_thread) {
            execl("/proc/self/exe", "/proc/self/exe", "true", nullptr);
            abort();
        } else {
            while (true) {
                syscall(SYS_ppoll, nullptr, 0, nullptr, nullptr);
            }
        }
    };

    for (size_t i = 1; i < num_threads; ++i) {
        threads.emplace_back([&thread_fn, i] {
            thread_fn(i);
        });
    }

    thread_fn(0);
}

TEST(ExecveTest, AsyncInjectExecve)
{
    bool executed = false;
    std::string msg;

    Hooks hooks;

    hooks.syscall_entry = [&](auto tracee, auto &info) -> SysCallEntryAction {
        if (strcmp(info.syscall.name(), "write") == 0) {
            msg.resize(info.args[2]);
            (void) tracee->read_mem(info.args[1], msg.data(), msg.size());

            return action::SuppressSysCall{
                    static_cast<SysCallRet>(info.args[2])};
        } else if (!std::exchange(executed, true)) {
            constexpr char argv0[] = "/proc/self/exe";
            constexpr char argv1[] = "echo";
            constexpr char argv2[] = "foobar";

            auto mmap_ret = tracee->mmap(0, 4096, PROT_WRITE | PROT_READ,
                                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

            auto ptr = mmap_ret.value();
            decltype(ptr) zero = 0;

            // Write argv0
            auto argv0_addr = ptr;
            (void) tracee->write_mem(ptr, argv0, sizeof(argv0));
            ptr += sizeof(argv0);

            // Write argv1
            auto argv1_addr = ptr;
            (void) tracee->write_mem(ptr, argv1, sizeof(argv1));
            ptr += sizeof(argv1);

            // Write argv2
            auto argv2_addr = ptr;
            (void) tracee->write_mem(ptr, argv2, sizeof(argv2));
            ptr += sizeof(argv2);

            // Write argv array
            auto argv_addr = ptr;
            (void) tracee->write_mem(ptr, &argv0_addr, sizeof(argv0_addr));
            ptr += sizeof(argv0_addr);
            (void) tracee->write_mem(ptr, &argv1_addr, sizeof(argv1_addr));
            ptr += sizeof(argv1_addr);
            (void) tracee->write_mem(ptr, &argv2_addr, sizeof(argv2_addr));
            ptr += sizeof(argv2_addr);
            (void) tracee->write_mem(ptr, &zero, sizeof(zero));
            ptr += sizeof(zero);

            // Write envp array
            auto envp_addr = ptr;
            (void) tracee->write_mem(ptr, &zero, sizeof(zero));
            ptr += sizeof(zero);

            SysCallArgs args = {};
            args[0] = argv0_addr;
            args[1] = argv_addr;
            args[2] = envp_addr;

            auto execve_sc = SysCall("execve", info.syscall.abi());
            assert(execve_sc);
            (void) tracee->modify_syscall_args(execve_sc.num(), args);
        }

        return action::Default{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        pause();
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_EQ(msg, "foobar\n");
}

TEST(ExecveTest, ExecveWithOneThread)
{
    std::unordered_set<pid_t> threads;
    std::unordered_set<pid_t> exited;
    std::unordered_set<pid_t> disappeared;
    bool post_execve = false;

    Hooks hooks;

    hooks.new_tracee = [&](auto *tracee) {
        threads.insert(tracee->tid);
    };

    hooks.tracee_exit = [&](pid_t tid, int) {
        if (!post_execve) {
            exited.insert(tid);
        }
        return action::Default{};
    };

    hooks.tracee_disappear = [&](pid_t tid) {
        disappeared.insert(tid);
    };

    hooks.syscall_exit = [&](auto, auto &info) {
        if (strcmp(info.syscall.name(), "execve") == 0) {
            post_execve = true;
        }
        return action::Default{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([] {
        exec_from(1, 0);
    }, Flag::TraceChildren));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_THAT(threads, IsEmpty());
    // Threads aside from the execve'ing thread and the thread group leader
    // should exit
    ASSERT_THAT(exited, IsEmpty());
    // The execve'ing thread should disappear only if it's not the thread group
    // leader
    ASSERT_THAT(disappeared, IsEmpty());
}

TEST(ExecveTest, ExecveFromMainThread)
{
    std::unordered_set<pid_t> threads;
    std::unordered_set<pid_t> exited;
    std::unordered_set<pid_t> disappeared;
    bool post_execve = false;

    Hooks hooks;

    hooks.new_tracee = [&](auto *tracee) {
        threads.insert(tracee->tid);
    };

    hooks.tracee_exit = [&](pid_t tid, int) {
        if (!post_execve) {
            exited.insert(tid);
        }
        return action::Default{};
    };

    hooks.tracee_disappear = [&](pid_t tid) {
        disappeared.insert(tid);
    };

    hooks.syscall_exit = [&](auto, auto &info) {
        if (strcmp(info.syscall.name(), "execve") == 0) {
            post_execve = true;
        }
        return action::Default{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([] {
        exec_from(3, 0);
    }, Flag::TraceChildren));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_THAT(threads, SizeIs(2));
    // Threads aside from the execve'ing thread and the thread group leader
    // should exit
    ASSERT_THAT(exited, ContainerEq(threads));
    // The execve'ing thread should disappear only if it's not the thread group
    // leader
    ASSERT_THAT(disappeared, IsEmpty());
}

TEST(ExecveTest, ExecveFromThread)
{
    std::vector<pid_t> threads;
    std::unordered_set<pid_t> exited;
    std::unordered_set<pid_t> disappeared;
    bool post_execve = false;

    Hooks hooks;

    hooks.tracee_exit = [&](pid_t tid, int) {
        if (!post_execve) {
            exited.insert(tid);
        }
        return action::Default{};
    };

    hooks.tracee_disappear = [&](pid_t tid) {
        disappeared.insert(tid);
    };

    hooks.syscall_exit = [&](auto, auto &info) {
        if (strcmp(info.syscall.name(), "clone") == 0) {
            // This is better than the new_tracee hook because the order is
            // guaranteed
            if (info.ret > 0) {
                threads.push_back(static_cast<pid_t>(info.ret));
            }
        } else if (strcmp(info.syscall.name(), "execve") == 0) {
            post_execve = true;
        }
        return action::Default{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([] {
        exec_from(3, 2);
    }, Flag::TraceChildren));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_THAT(threads, SizeIs(2));
    // Threads aside from the execve'ing thread and the thread group leader
    // should exit
    ASSERT_THAT(exited, ElementsAre(threads[0]));
    // The execve'ing thread should disappear
    ASSERT_THAT(disappeared, ElementsAre(threads[1]));
}

TEST(ExecveTest, ExecveFromThreadWithUntracedLeader)
{
    std::vector<pid_t> threads;
    std::unordered_set<pid_t> exited;
    std::unordered_set<pid_t> disappeared;
    std::unordered_set<pid_t> reappeared;
    bool post_execve_entry = false;
    bool post_execve_exit = false;

    Tracer tracer;

    auto parent = tracer.fork([] {
        exec_from(3, 2);
    }, Flag::TraceChildren);
    ASSERT_TRUE(parent);

    auto parent_tid = parent.value()->tid;

    Hooks hooks;

    hooks.new_tracee = [&](auto tracee) {
        if (post_execve_entry) {
            reappeared.insert(tracee->tid);
        }
    };

    hooks.tracee_exit = [&](pid_t tid, int) {
        if (!post_execve_exit) {
            exited.insert(tid);
        }
        return action::Default{};
    };

    hooks.tracee_disappear = [&](pid_t tid) {
        disappeared.insert(tid);
    };

    hooks.syscall_entry = [&](auto tracee, auto &info) -> SysCallEntryAction {
        if (tracee->tid == parent_tid
                && strcmp(info.syscall.name(), "getpid") == 0) {
            // Detach at exec_from()'s hook point
            return action::DetachTracee{};
        } else if (strcmp(info.syscall.name(), "execve") == 0) {
            post_execve_entry = true;
        }
        return action::Default{};
    };

    hooks.syscall_exit = [&](auto, auto &info) {
        if (strcmp(info.syscall.name(), "clone") == 0) {
            // This is better than the new_tracee hook because the order is
            // guaranteed
            if (info.ret > 0) {
                threads.push_back(static_cast<pid_t>(info.ret));
            }
        } else if (strcmp(info.syscall.name(), "execve") == 0) {
            post_execve_exit = true;
        }
        return action::Default{};
    };

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_THAT(threads, SizeIs(2));
    // Threads aside from the execve'ing thread and the thread group leader
    // should exit
    ASSERT_THAT(exited, UnorderedElementsAre(threads[0]));
    // The execve'ing thread should disappear
    ASSERT_THAT(disappeared, ElementsAre(threads[1]));
    // The thread group leader should reappear during execve()
    ASSERT_THAT(reappeared, ElementsAre(parent_tid));
}

TEST(ExecveTest, ExecveFromUntracedMainThread)
{
    std::unordered_set<pid_t> threads;
    std::unordered_set<pid_t> exited;
    std::unordered_set<pid_t> disappeared;

    Hooks hooks;

    hooks.new_tracee = [&](auto *tracee) {
        threads.insert(tracee->tid);
    };

    hooks.tracee_exit = [&](pid_t tid, int) {
        exited.insert(tid);
        return action::Default{};
    };

    hooks.tracee_disappear = [&](pid_t tid) {
        disappeared.insert(tid);
    };

    hooks.syscall_entry = [&](auto, auto &info) -> SysCallEntryAction {
        if (strcmp(info.syscall.name(), "execve") == 0) {
            return action::DetachTracee{};
        }
        return action::Default{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([] {
        exec_from(3, 0);
    }, Flag::TraceChildren));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_THAT(threads, SizeIs(2));
    // If the execve'ing thread in a thread group is not being traced, then the
    // kernel simply makes it appear as if every thread exited. Normally, an
    // exit event would not be reported for the thread group leader.
    ASSERT_THAT(exited, ContainerEq(threads));
    // Normally, only the execve'ing thread (if it's not the thread group
    // leader) would disappear.
    ASSERT_THAT(disappeared, IsEmpty());
}

TEST(ExecveTest, ExecveFromUntracedThread)
{
    std::vector<pid_t> threads;
    std::unordered_set<pid_t> exited;
    std::unordered_set<pid_t> disappeared;

    Hooks hooks;

    hooks.tracee_exit = [&](pid_t tid, int) {
        exited.insert(tid);
        return action::Default{};
    };

    hooks.tracee_disappear = [&](pid_t tid) {
        disappeared.insert(tid);
    };

    hooks.syscall_entry = [&](auto, auto &info) -> SysCallEntryAction {
        if (strcmp(info.syscall.name(), "execve") == 0) {
            return action::DetachTracee{};
        }
        return action::Default{};
    };

    hooks.syscall_exit = [&](auto, auto &info) {
        if (strcmp(info.syscall.name(), "clone") == 0) {
            // This is better than the new_tracee hook because the order is
            // guaranteed
            if (info.ret > 0) {
                threads.push_back(static_cast<pid_t>(info.ret));
            }
        }
        return action::Default{};
    };

    Tracer tracer;

    auto parent = tracer.fork([] {
        exec_from(3, 2);
    }, Flag::TraceChildren);
    ASSERT_TRUE(parent);

    auto parent_tid = parent.value()->tid;

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_THAT(threads, SizeIs(2));
    // If the execve'ing thread in a thread group is not being traced, then the
    // kernel simply makes it appear as if every thread exited. Normally, an
    // exit event would not be reported for the thread group leader.
    ASSERT_THAT(exited, UnorderedElementsAre(parent_tid, threads[0]));
    // Normally, only the execve'ing thread (if it's not the thread group
    // leader) would disappear.
    ASSERT_THAT(disappeared, IsEmpty());
}
