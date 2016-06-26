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

#include <sys/wait.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/command.h"
#include "mbutil/properties.h"
#include "mbutil/selinux.h"

#include "external/android_reboot.h"

namespace mb
{

static void log_output(const std::string &line, void *data)
{
    (void) data;
    std::string copy;
    if (!line.empty() && line.back() == '\n') {
        copy.assign(line.begin(), line.end() - 1);
    }
    LOGD("Command output: %s", copy.c_str());
}

static bool allow_reboot_via_framework()
{
    policydb_t pdb;

    if (policydb_init(&pdb) < 0) {
        LOGE("Failed to initialize policydb");
        return false;
    }

    if (!util::selinux_read_policy(SELINUX_POLICY_FILE, &pdb)) {
        LOGE("Failed to read SELinux policy file: %s", SELINUX_POLICY_FILE);
        policydb_destroy(&pdb);
        return false;
    }

    LOGD("Policy version: %u", pdb.policyvers);

    // Allow zygote to write to our stdout pipe
    util::selinux_add_rule(&pdb, "zygote", "init", "fifo_file", "write");

    util::selinux_add_rule(&pdb, "zygote", "activity_service", "service_manager", "find");
    util::selinux_add_rule(&pdb, "zygote", "init", "unix_stream_socket", "read");
    util::selinux_add_rule(&pdb, "zygote", "init", "unix_stream_socket", "write");
    util::selinux_add_rule(&pdb, "zygote", "servicemanager", "binder", "call");
    util::selinux_add_rule(&pdb, "zygote", "system_server", "binder", "call");

    util::selinux_add_rule(&pdb, "servicemanager", "zygote", "dir", "search");
    util::selinux_add_rule(&pdb, "servicemanager", "zygote", "file", "open");
    util::selinux_add_rule(&pdb, "servicemanager", "zygote", "file", "read");
    util::selinux_add_rule(&pdb, "servicemanager", "zygote", "process", "getattr");

    if (!util::selinux_write_policy(SELINUX_LOAD_FILE, &pdb)) {
        LOGE("Failed to write SELinux policy file: %s", SELINUX_LOAD_FILE);
        policydb_destroy(&pdb);
        return false;
    }

    policydb_destroy(&pdb);

    return true;
}

bool reboot_via_framework(bool confirm)
{
    std::string confirm_str("false");
    if (confirm) {
        confirm_str = "true";
    }

    allow_reboot_via_framework();

    int status = util::run_command_cb({
        "am", "start",
        //"-W",
        "--ez", "android.intent.extra.KEY_CONFIRM", confirm_str,
        "-a", "android.intent.action.REBOOT"
    }, &log_output, nullptr);

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

bool reboot_via_init(const std::string &reboot_arg)
{
    std::string value("reboot,");
    value.append(reboot_arg);

    if (value.size() >= MB_PROP_VALUE_MAX - 1) {
        LOGE("Reboot argument %zu bytes too long",
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
        LOGE("Failed to reboot: %s", strerror(errno));
        return false;
    }

    // Obviously shouldn't return
    while (1) {
        pause();
    }

    return true;
}


}
