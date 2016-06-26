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

#include "mblog/logging.h"
#include "mbutil/reboot.h"
#include "mbutil/selinux.h"

namespace mb
{

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
    allow_reboot_via_framework();

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
