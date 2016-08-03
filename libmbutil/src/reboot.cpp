/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/reboot.h"

#include <cstring>

#include <sys/reboot.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/command.h"
#include "mbutil/properties.h"

#include "external/android_reboot.h"

namespace mb
{
namespace util
{

static void log_output(const char *line, bool error, void *userdata)
{
    (void) error;
    (void) userdata;

    size_t size = strlen(line);

    std::string copy;
    if (size > 0 && line[size - 1] == '\n') {
        copy.assign(line, line + size - 1);
    } else {
        copy.assign(line, line + size);
    }

    LOGD("Reboot command output: %s", copy.c_str());
}

bool reboot_via_framework(bool show_confirm_dialog)
{
    const char *argv[] = {
        "am", "start",
        //"-W",
        "--ez", "android.intent.extra.KEY_CONFIRM",
            show_confirm_dialog ? "true" : "false",
        "-a", "android.intent.action.REBOOT",
        nullptr
    };

    int status = run_command(argv[0], argv, nullptr, nullptr, &log_output,
                             nullptr);

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool reboot_via_init(const char *reboot_arg)
{
    // The length of the prefix + reboot_arg + NULL terminator cannot exceed
    // PROP_VALUE_MAX
    char buf[PROP_VALUE_MAX - 7 - 1];

    if (!reboot_arg) {
        reboot_arg = "";
    }

    int ret = snprintf(buf, sizeof(buf), "reboot,%s", reboot_arg);
    if (ret < 0) {
        return false;
    } else if (ret >= (int) sizeof(buf)) {
        LOGE("Reboot argument %d bytes too long",
             ret + 1 - PROP_VALUE_MAX);
        return false;
    }

    if (property_set(ANDROID_RB_PROPERTY, buf) < 0) {
        LOGE("Failed to set '%s' property", ANDROID_RB_PROPERTY);
        return false;
    }

    return true;
}

bool reboot_via_syscall(const char *reboot_arg)
{
    // Reboot to system if arg is null
    int reason = reboot_arg ? ANDROID_RB_RESTART2 : ANDROID_RB_RESTART;

    if (android_reboot(reason, reboot_arg) < 0) {
        LOGE("Failed to reboot via syscall: %s", strerror(errno));
        return false;
    }

    return true;
}

bool shutdown_via_init()
{
    if (property_set(ANDROID_RB_PROPERTY, "shutdown,") < 0) {
        LOGE("Failed to set '%s' property", ANDROID_RB_PROPERTY);
        return false;
    }

    return true;
}

bool shutdown_via_syscall()
{
    if (android_reboot(ANDROID_RB_POWEROFF, nullptr) < 0) {
        LOGE("Failed to shut down via syscall: %s", strerror(errno));
        return false;
    }

    return true;
}

}
}
