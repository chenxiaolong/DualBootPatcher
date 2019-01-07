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
#include <string_view>

#include <unistd.h>

#include "mbcommon/outcome.h"


namespace mb::systrace::detail
{

oc::result<pid_t> get_pid_status_field(pid_t pid, std::string_view name);
oc::result<pid_t> get_tgid(pid_t pid);

oc::result<bool> for_each_tid(pid_t pid,
                              std::function<oc::result<bool>(pid_t)> func,
                              bool retry_until_no_more);

}
