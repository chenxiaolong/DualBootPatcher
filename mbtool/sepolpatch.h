/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

namespace mb
{

enum class SELinuxResult
{
    CHANGED,
    UNCHANGED,
    ERROR,
};

SELinuxResult selinux_raw_set_allow_rule(policydb_t *pdb,
                                         uint16_t source_type_val,
                                         uint16_t target_type_val,
                                         uint16_t class_type_val,
                                         uint32_t perm_val,
                                         bool remove);
SELinuxResult selinux_raw_set_type_trans(policydb_t *pdb,
                                         uint16_t source_type_val,
                                         uint16_t target_type_val,
                                         uint16_t class_val,
                                         uint16_t default_type_val);
SELinuxResult selinux_raw_grant_all_perms(policydb_t *pdb,
                                          uint16_t source_type_val,
                                          uint16_t target_type_val,
                                          uint16_t class_val);
SELinuxResult selinux_raw_grant_all_perms(policydb_t *pdb,
                                          uint16_t source_type_val,
                                          uint16_t target_type_val);
SELinuxResult selinux_raw_set_permissive(policydb_t *pdb,
                                         uint16_t type_val,
                                         bool permissive);
SELinuxResult selinux_raw_set_attribute(policydb_t *pdb,
                                        uint16_t type_val,
                                        uint16_t attr_val);
SELinuxResult selinux_raw_add_to_role(policydb_t *pdb,
                                      uint16_t role_val,
                                      uint16_t type_val);
bool selinux_raw_reindex(policydb_t *pdb);

// Helper functions

bool selinux_mount();
bool selinux_unmount();
bool selinux_make_all_permissive(policydb_t *pdb);
bool selinux_make_permissive(policydb_t *pdb,
                             const char *type_str);
bool selinux_set_allow_rule(policydb_t *pdb,
                            const char *source_str,
                            const char *target_str,
                            const char *class_str,
                            const char *perm_str,
                            bool remove);
bool selinux_add_rule(policydb_t *pdb,
                      const char *source_str,
                      const char *target_str,
                      const char *class_str,
                      const char *perm_str);
bool selinux_remove_rule(policydb_t *pdb,
                         const char *source_str,
                         const char *target_str,
                         const char *class_str,
                         const char *perm_str);
bool selinux_set_type_trans(policydb_t *pdb,
                            const char *source_str,
                            const char *target_str,
                            const char *class_str,
                            const char *default_str);
bool selinux_grant_all_perms(policydb_t *pdb,
                             const char *source_type_str,
                             const char *target_type_str,
                             const char *class_str);
bool selinux_grant_all_perms(policydb_t *pdb,
                             const char *source_type_str,
                             const char *target_type_str);
SELinuxResult selinux_create_type(policydb_t *pdb,
                                  const char *name);
bool selinux_set_attribute(policydb_t *pdb,
                           const char *type_name,
                           const char *attr_name);
bool selinux_add_to_role(policydb_t *pdb,
                         const char *role_name,
                         const char *type_name);

// Patching functions

enum class SELinuxPatch
{
    NONE = 0,
    PRE_BOOT,
    MAIN,
    CWM_RECOVERY,
    STRIP_NO_AUDIT,
};

bool selinux_apply_patch(policydb_t *pdb, SELinuxPatch patch);

bool patch_sepolicy(const std::string &source,
                    const std::string &target,
                    SELinuxPatch patch);
bool patch_loaded_sepolicy(SELinuxPatch patch);

int sepolpatch_main(int argc, char *argv[]);

}
