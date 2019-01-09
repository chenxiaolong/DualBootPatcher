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

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <unistd.h>

#include "mbcommon/common.h"
#include "mbcommon/flags.h"
#include "mbcommon/outcome.h"

#include "mbsystrace/event_p.h"
#include "mbsystrace/hooks.h"

namespace mb::systrace
{

//! Flags to influence the behavior or attaching to a tracee
enum class Flag
{
    //! Automatically trace new child processes of a tracee
    TraceChildren = 1 << 0,
};
MB_DECLARE_FLAGS(Flags, Flag)
MB_DECLARE_OPERATORS_FOR_FLAGS(Flags)

class Tracee;

class MB_EXPORT Tracer final
{
public:
    Tracer();
    ~Tracer();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(Tracer)
    MB_DISABLE_MOVE_CONSTRUCT_AND_ASSIGN(Tracer)

    oc::result<void> execute(const Hooks &hooks);
    oc::result<void> execute(const Hooks &hooks, int pid_spec);
    void stop_after_hook() noexcept;

    oc::result<Tracee *> fork(std::function<void()> child, Flags flags = {});
    oc::result<std::vector<Tracee *>> attach(pid_t tid, Flags flags = {});

private:
    std::unordered_map<pid_t, std::unique_ptr<Tracee>> m_tracees;
    std::unordered_set<pid_t> m_owned_tgids;
    std::optional<detail::ProcessEvent> m_requeued_event;
    bool m_should_stop;

    oc::result<Tracee *> add_child(pid_t tgid, pid_t tid);
    bool remove_child(pid_t tid);

    oc::result<bool>
    dispatch_event(const Hooks &hooks, const detail::ProcessEvent &event);

    void
    execute_new_tracee_hook(const Hooks &hooks, Tracee *tracee);
    void
    execute_tracee_exit_hook(const Hooks &hooks, pid_t tid, int status,
                             int exit_code);
    void
    execute_tracee_death_hook(const Hooks &hooks, pid_t tid, int status,
                              int exit_signal);
    void
    execute_tracee_disappear_hook(const Hooks &hooks, pid_t tid);
    oc::result<void>
    execute_tracee_signal_hook(const Hooks &hooks, Tracee *tracee, int signal);
    oc::result<void>
    execute_group_stop_hook(const Hooks &hooks, Tracee *tracee, int signal);
    oc::result<void>
    execute_interrupt_stop_hook(const Hooks &hooks, Tracee *tracee);
    oc::result<void>
    execute_syscall_entry_hook(const Hooks &hooks, Tracee *tracee,
                               const SysCallEntryInfo &info);
    oc::result<void>
    execute_syscall_exit_hook(const Hooks &hooks, Tracee *tracee,
                              const SysCallExitInfo &info);
    void
    execute_unknown_child_hook(const Hooks &hooks, pid_t tid, int status);

    Tracee * find_tracee(pid_t tid);
};

}
