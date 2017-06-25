/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "reboot.h"

#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/reboot.h"


namespace mb
{

bool reboot_via_framework(bool confirm)
{
    return util::reboot_via_framework(confirm);
}

bool reboot_via_init(const std::string &reboot_arg)
{
    if (!util::reboot_via_init(reboot_arg.c_str())) {
        return false;
    }

    // Obviously shouldn't return
    while (1) {
        pause();
    }

    return true;
}

bool reboot_directly(const std::string &reboot_arg)
{
    if (!util::reboot_via_syscall(reboot_arg.c_str())) {
        return false;
    }

    // Obviously shouldn't return
    while (1) {
        pause();
    }

    return true;
}

bool shutdown_via_init()
{
    if (!util::shutdown_via_init()) {
        return false;
    }

    // Obviously shouldn't return
    while (1) {
        pause();
    }

    return true;
}

bool shutdown_directly()
{
    if (!util::shutdown_via_syscall()) {
        return false;
    }

    // Obviously shouldn't return
    while (1) {
        pause();
    }

    return true;
}

}
