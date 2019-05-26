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

#include <vector>

#include <sys/signal.h>

#include "mbsystrace/signals.h"
#include "mbsystrace/tracee.h"
#include "mbsystrace/tracer.h"

using namespace mb::systrace;
using namespace testing;

TEST(SignalsTest, CheckSignalsListSanity)
{
    struct
    {
        const char *name;
        int num;
    } test_cases[] = {
        {"SIGKILL", 9},
        {"SIGSEGV", 11},
        {"SIGTERM", 15},
    };

    for (auto const &tc : test_cases) {
        Signal sig(tc.name);
        ASSERT_TRUE(sig) << "[" << tc.name << "] "
                << "Signal is not valid";
        ASSERT_EQ(sig.num(), tc.num) << "[" << tc.name << "] "
                << "Expected " << tc.num << ", but got " << sig.num();
    }
}

TEST(SignalsTest, ForwardSignals)
{
    const std::vector<int> expected_signals{SIGINT};
    std::vector<int> received_signals;
    int last_signal = 0;
    int death_signal = 0;

    Tracer tracer;
    Hooks hooks;

    hooks.syscall_entry = [&](auto tracee, auto &) -> SysCallEntryAction {
        if (last_signal == 0) {
            if (!tracee->signal_process(SIGINT)) {
                tracer.stop_after_hook();
                return action::NoAction{};
            }
            last_signal = SIGINT;
        }
        return action::ContinueExec{};
    };
    hooks.tracee_signal = [&](auto, auto signal) {
        received_signals.push_back(signal);
        return action::ForwardSignal{};
    };
    hooks.tracee_death = [&](auto, auto sig) {
        death_signal = sig;
        return action::NoAction{};
    };

    ASSERT_TRUE(tracer.fork([&] {
        while (true) {
            pause();
        }
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_EQ(received_signals, expected_signals);
    ASSERT_EQ(last_signal, SIGINT);
    ASSERT_EQ(death_signal, SIGINT);
}

TEST(SignalsTest, SuppressCatchableSignals)
{
    std::vector<int> signals{
        // Terminating signals
        SIGHUP,
        SIGINT,
        SIGPIPE,
        // Terminating signals that core dump
        SIGILL,
        SIGTRAP,
        SIGABRT,
        // Stopping signals
        SIGTSTP,
        SIGTTIN,
        SIGTTOU,
        // Ignored signals
        SIGCHLD,
        // Continuing signals
        SIGCONT,
    };

    std::vector<int> received_signals;
    Hooks hooks;

    hooks.tracee_signal = [&](auto, auto signal) {
        received_signals.push_back(signal);
        return action::SuppressSignal{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        for (auto const &s : signals) {
            raise(s);
        }
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_EQ(received_signals, signals);
}

TEST(SignalsTest, GroupStopAndResume)
{
    std::vector<int> received_signals;
    bool got_group_stop = false;
    bool got_interrupt_stop = false;
    Hooks hooks;

    hooks.tracee_signal = [&](auto, auto signal) {
        received_signals.push_back(signal);
        return action::Default{};
    };

    hooks.group_stop = [&](auto *tracee, auto) {
        got_group_stop = true;
        (void) tracee->signal_thread(SIGCONT);
        return action::Default{};
    };

    hooks.interrupt_stop = [&](auto) {
        got_interrupt_stop = true;
        return action::Default{};
    };

    Tracer tracer;

    ASSERT_TRUE(tracer.fork([&] {
        raise(SIGSTOP);
    }));

    ASSERT_TRUE(tracer.execute(hooks));

    ASSERT_THAT(received_signals, ElementsAre(SIGSTOP, SIGCONT));
    ASSERT_TRUE(got_group_stop);
    ASSERT_TRUE(got_interrupt_stop);
}
