/*
 * Copyright (C) 2015-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/reboot.h"

#include <chrono>
#include <thread>

#include <cstring>

#include <fcntl.h>
#include <linux/reboot.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/command.h"
#include "mbutil/file.h"
#include "mbutil/mount.h"
#include "mbutil/properties.h"

#define LOG_TAG "mbutil/reboot"

namespace mb::util
{

constexpr char ANDROID_RB_PROPERTY[] = "sys.powerctl";

static void log_output(std::string_view line, bool error)
{
    (void) error;

    if (!line.empty() && line.back() == '\n') {
        line.remove_suffix(1);
    }

    LOGD("Reboot command output: %.*s",
         static_cast<int>(line.size()), line.data());
}

static bool run_command_and_log(const std::vector<std::string> &args)
{
    int status = run_command(args[0], args, {}, {}, &log_output);

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static bool remount_ro_done()
{
    auto entries = util::get_mount_entries();
    if (!entries) {
        return true;
    }

    for (auto const &entry : entries.value()) {
        if (starts_with(entry.source, "/dev/block")
                && starts_with(entry.vfs_options, "rw,")) {
            return false;
        }
    }

    return true;
}

static bool remount_ro()
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    if (!file_write_data("/proc/sysrq-trigger", "u", 1)) {
        return false;
    }

    // Allow 1 minute to remount read-only
    auto stop_tp = steady_clock::now() + 1min;

    while (!remount_ro_done() && steady_clock::now() < stop_tp) {
        std::this_thread::sleep_for(100ms);
    }

    return true;
}

bool reboot_via_framework(bool show_confirm_dialog)
{
    return run_command_and_log({
        "am", "start",
        //"-W",
        "--ez", "android.intent.extra.KEY_CONFIRM",
            show_confirm_dialog ? "true" : "false",
        "-a", "android.intent.action.REBOOT",
    });
}

bool reboot_via_init(const std::string &reboot_arg)
{
    std::string prop_value{"reboot,"};
    prop_value += reboot_arg;

    if (!property_set(ANDROID_RB_PROPERTY, prop_value)) {
        LOGE("Failed to set property '%s'='%s'",
             ANDROID_RB_PROPERTY, prop_value.c_str());
        return false;
    }

    return true;
}

bool reboot_via_syscall(const std::string &reboot_arg)
{
    sync();
    remount_ro();

    auto ret = syscall(__NR_reboot, LINUX_REBOOT_MAGIC1,
                       LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART2,
                       reboot_arg.c_str());
    if (ret < 0) {
        LOGE("Failed to reboot via syscall: %s", strerror(errno));
        return false;
    }

    return true;
}

bool shutdown_via_framework(bool show_confirm_dialog)
{
    for (auto const &action : {
        "com.android.internal.intent.action.REQUEST_SHUTDOWN",
        "android.intent.action.ACTION_REQUEST_SHUTDOWN",
    }) {
        if (run_command_and_log({
            "am", "start",
            //"-W",
            "--ez", "android.intent.extra.KEY_CONFIRM",
                show_confirm_dialog ? "true" : "false",
            "--ez", "android.intent.extra.USER_REQUESTED_SHUTDOWN", "true",
            "-a", action,
        })) {
            return true;
        }
    }

    return false;
}

bool shutdown_via_init()
{
    constexpr char prop_value[] = "shutdown,";

    if (!property_set(ANDROID_RB_PROPERTY, prop_value)) {
        LOGE("Failed to set property '%s'='%s'",
             ANDROID_RB_PROPERTY, prop_value);
        return false;
    }

    return true;
}

bool shutdown_via_syscall()
{
    sync();
    remount_ro();

    if (reboot(RB_POWER_OFF) < 0) {
        LOGE("Failed to shut down via syscall: %s", strerror(errno));
        return false;
    }

    return true;
}

}
