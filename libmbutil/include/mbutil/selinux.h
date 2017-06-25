/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <string>

#include <sepol/policydb/policydb.h>

#define SELINUX_ENFORCE_FILE            "/sys/fs/selinux/enforce"
#define SELINUX_POLICY_FILE             "/sys/fs/selinux/policy"
#define SELINUX_LOAD_FILE               "/sys/fs/selinux/load"

#define SELINUX_MOUNT_POINT             "/sys/fs/selinux"
#define SELINUX_FS_TYPE                 "selinuxfs"
#define SELINUX_DEFAULT_POLICY_FILE     "/sepolicy"

namespace mb
{
namespace util
{

enum class SELinuxAttr
{
    CURRENT,
    EXEC,
    FSCREATE,
    KEYCREATE,
    PREV,
    SOCKCREATE,
};

bool selinux_read_policy(const std::string &path, policydb_t *pdb);
bool selinux_write_policy(const std::string &path, policydb_t *pdb);
bool selinux_get_context(const std::string &path, std::string *context);
bool selinux_lget_context(const std::string &path, std::string *context);
bool selinux_fget_context(int fd, std::string *context);
bool selinux_set_context(const std::string &path, const std::string &context);
bool selinux_lset_context(const std::string &path, const std::string &context);
bool selinux_fset_context(int fd, const std::string &context);
bool selinux_set_context_recursive(const std::string &path,
                                   const std::string &context);
bool selinux_lset_context_recursive(const std::string &path,
                                    const std::string &context);
bool selinux_get_enforcing(int *value);
bool selinux_set_enforcing(int value);
bool selinux_get_process_attr(pid_t pid, SELinuxAttr attr,
                              std::string *context_out);
bool selinux_set_process_attr(pid_t pid, SELinuxAttr attr,
                              const std::string &context);

}
}
