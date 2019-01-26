/*
 * Copyright (C) 2015-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include <sys/mount.h>
#include <sys/reboot.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/command.h"
#include "mbutil/file.h"
#include "mbutil/mount.h"
#include "mbutil/path.h"
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

static bool remount_ro_and_unmount()
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    if (auto r = file_write_data("/proc/sysrq-trigger", "u", 1); !r) {
        LOGW("Failed to remount everything read-only via sysrq: %s",
             r.error().message().c_str());
    }

    // Allow 6 seconds to remount read-only (same as Android 9.0 init)
    auto stop_tp = steady_clock::now() + 6s;

    do {
        if (auto entries = util::get_mount_entries()) {
            // Iterate backwards and try to unmount. If there are no "rw"
            // volumes left, then stop.
            std::vector<std::string> rw;

            for (auto it = entries.value().rbegin();
                    it != entries.value().rend(); ++it) {
                if (!starts_with(it->source, "/dev/block/")
                        && !starts_with(it->target, "/data/")) {
                    continue;
                }

                if (umount2(it->target.c_str(), MNT_FORCE) == 0) {
                    continue;
                }

                LOGW("%s: Failed to unmount: %s",
                     it->target.c_str(), strerror(errno));

                if (::mount("", it->target.c_str(), "",
                            MS_REMOUNT | MS_RDONLY, "") == 0) {
                    continue;
                }

                LOGW("%s: Failed to remount read only: %s",
                     it->target.c_str(), strerror(errno));

                if (starts_with(it->vfs_options, "rw,")) {
                    rw.emplace_back(it->target);
                }
            }

            if (rw.empty()) {
                break;
            } else {
                LOGW("Remaining rw mountpoints: [%s]", join(rw, ", ").c_str());
            }
        } else {
            LOGW("Failed to get mount entries: %s",
                 entries.error().message().c_str());
        }

        std::this_thread::sleep_for(100ms);
    } while (steady_clock::now() < stop_tp);

    return true;
}

static void pre_shutdown_tasks()
{
    using namespace std::chrono_literals;

    sync();
    remount_ro_and_unmount();
    sync();

    std::this_thread::sleep_for(100ms);

    LOGV("Ready for shutdown");
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
    pre_shutdown_tasks();

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
    pre_shutdown_tasks();

    if (reboot(RB_POWER_OFF) < 0) {
        LOGE("Failed to shut down via syscall: %s", strerror(errno));
        return false;
    }

    return true;
}

}
