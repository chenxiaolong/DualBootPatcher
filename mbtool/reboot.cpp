/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "reboot.h"

#include <unistd.h>

#include "external/android_reboot.h"
#include "util/logging.h"
#include "util/properties.h"

namespace mb
{

bool reboot_via_init(const std::string &reboot_arg)
{
    std::string value("reboot,");
    value.append(reboot_arg);

    if (value.size() >= MB_PROP_VALUE_MAX - 1) {
        LOGE("Reboot argument {:d} bytes too long",
             value.size() + 1 - MB_PROP_VALUE_MAX);
        return false;
    }

    if (!util::set_property(ANDROID_RB_PROPERTY, value)) {
        LOGE("Failed to set property");
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
    if (android_reboot(ANDROID_RB_RESTART2, reboot_arg.c_str()) < 0) {
        LOGE("Failed to reboot: {}", strerror(errno));
        return false;
    }

    // Obviously shouldn't return
    while (1) {
        pause();
    }

    return true;
}


}