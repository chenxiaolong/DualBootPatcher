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

#include "sepolpatch.h"

#include <memory>

#include <climits>
#include <cstdio>

#include <fcntl.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

// libsepol is not very C++ friendly. 'bool' is a struct field in conditional.h
#define bool bool2
#include <sepol/policydb/expand.h>
#include <sepol/policydb/policydb.h>
#include <sepol/sepol.h>
#undef bool

#include "mbcommon/common.h"
#include "mblog/logging.h"
#include "mbutil/autoclose/file.h"
#include "mbutil/finally.h"
#include "mbutil/selinux.h"
#include "mbutil/string.h"

#include "multiboot.h"


extern "C" int policydb_index_decls(policydb_t *p);

namespace mb
{

/*!
 * Add or remove rule.
 *
 * \param pdb Policy DB object
 * \param source_type_val Source type for rule
 * \param target_type_val Target type for rule
 * \param class_val Class for rule
 * \param perm_val Permission for rule
 * \param remove Whether to remove the rule
 *
 * \return Whether a change was made
 */
SELinuxResult selinux_raw_set_avtab_rule(policydb_t *pdb,
                                         uint16_t source_type_val,
                                         uint16_t target_type_val,
                                         uint16_t class_val,
                                         uint32_t perm_val,
                                         bool remove)
{
    avtab_datum_t *av;
    avtab_key_t key;

    key.source_type = source_type_val;
    key.target_type = target_type_val;
    key.target_class = class_val;
    key.specified = AVTAB_ALLOWED;
    av = avtab_search(&pdb->te_avtab, &key);

    if (!av) {
        if (remove) {
            return SELinuxResult::UNCHANGED;
        } else {
            avtab_datum_t av_new;
            av_new.data = (1U << (perm_val - 1));
            if (avtab_insert(&pdb->te_avtab, &key, &av_new) != 0) {
                return SELinuxResult::ERROR;
            }
            return SELinuxResult::CHANGED;
        }
    } else {
        auto old_data = av->data;

        if (remove) {
            av->data &= ~(1U << (perm_val - 1));
        } else {
            av->data |= (1U << (perm_val - 1));
        }

        return (av->data == old_data)
                ? SELinuxResult::UNCHANGED
                : SELinuxResult::CHANGED;
    }
}

SELinuxResult selinux_raw_set_type_trans(policydb_t *pdb,
                                         uint16_t source_type_val,
                                         uint16_t target_type_val,
                                         uint16_t class_val,
                                         uint16_t default_type_val)
{
    avtab_datum_t *av;
    avtab_key_t key;

    key.source_type = source_type_val;
    key.target_type = target_type_val;
    key.target_class = class_val;
    key.specified = AVTAB_TRANSITION;
    av = avtab_search(&pdb->te_avtab, &key);

    if (!av) {
        avtab_datum_t av_new;
        av_new.data = default_type_val;
        if (avtab_insert(&pdb->te_avtab, &key, &av_new) != 0) {
            return SELinuxResult::ERROR;
        }
        return SELinuxResult::CHANGED;
    } else {
        auto old_data = av->data;

        av->data = default_type_val;

        return (av->data == old_data)
                ? SELinuxResult::UNCHANGED
                : SELinuxResult::CHANGED;
    }
}

SELinuxResult selinux_raw_grant_all_perms(policydb_t *pdb,
                                          uint16_t source_type_val,
                                          uint16_t target_type_val,
                                          uint16_t class_val)
{
    SELinuxResult result(SELinuxResult::UNCHANGED);

    auto clazz = pdb->class_val_to_struct[class_val - 1];
    if (!clazz) {
        return SELinuxResult::ERROR;
    }

    // Class-specific permissions
    hashtab_t tables[] = { clazz->permissions.table, nullptr, nullptr };
    if (clazz->comdatum) {
        tables[1] = clazz->comdatum->permissions.table;
    }

    for (auto table = tables; *table; ++table) {
        for (uint32_t bucket = 0; bucket < (*table)->size; ++bucket) {
            for (hashtab_ptr_t cur = (*table)->htable[bucket]; cur;
                    cur = cur->next) {
                perm_datum_t *perm_datum = (perm_datum_t *) cur->datum;

                SELinuxResult ret = selinux_raw_set_avtab_rule(
                        pdb, source_type_val, target_type_val, class_val,
                        perm_datum->s.value, false);

                switch (ret) {
                case SELinuxResult::ERROR:
                    return ret;
                case SELinuxResult::CHANGED:
                    result = ret;
                    break;
                case SELinuxResult::UNCHANGED:
                    // Ignore
                    break;
                }
            }
        }
    }

    return result;
}

SELinuxResult selinux_raw_grant_all_perms(policydb_t *pdb,
                                          uint16_t source_type_val,
                                          uint16_t target_type_val)
{
    SELinuxResult result(SELinuxResult::UNCHANGED);

    for (uint32_t class_val = 1; class_val <= pdb->p_classes.nprim;
            ++class_val) {
        SELinuxResult ret = selinux_raw_grant_all_perms(
                pdb, source_type_val, target_type_val, class_val);

        switch (ret) {
        case SELinuxResult::ERROR:
            return ret;
        case SELinuxResult::CHANGED:
            result = ret;
            break;
        case SELinuxResult::UNCHANGED:
            // Ignore
            break;
        }
    }

    return result;
}

SELinuxResult selinux_raw_set_permissive(policydb_t *pdb,
                                         uint16_t type_val,
                                         bool permissive)
{
    int ret = ebitmap_get_bit(&pdb->permissive_map, type_val);
    if (!!ret == permissive) {
        return SELinuxResult::UNCHANGED;
    }

    if (ebitmap_set_bit(&pdb->permissive_map, type_val, permissive) < 0) {
        return SELinuxResult::ERROR;
    }

    return SELinuxResult::CHANGED;
}

/*!
 * \brief Set attribute for a type
 *
 * \param pdb Policy object
 * \param type_val Value of type
 * \param attr_val Value of attribute
 *
 * \return Whether the attribute was set
 */
SELinuxResult selinux_raw_set_attribute(policydb_t *pdb,
                                        uint16_t type_val,
                                        uint16_t attr_val)
{
    bool changed = false;

    int ret1 = ebitmap_get_bit(&pdb->type_attr_map[type_val - 1], attr_val - 1);
    int ret2 = ebitmap_get_bit(&pdb->attr_type_map[attr_val - 1], type_val - 1);

    if (ret1 != ret2) {
        // Maps are invalid
        return SELinuxResult::ERROR;
    }

    // Update type-attribute maps

    if (!ret1) {
        ret1 = ebitmap_set_bit(
                &pdb->type_attr_map[type_val - 1], attr_val - 1, 1);
        ret2 = ebitmap_set_bit(
                &pdb->attr_type_map[attr_val - 1], type_val - 1, 1);

        if (ret1 < 0 || ret2 < 0) {
            return SELinuxResult::ERROR;
        }

        changed = true;
    }

    // Update MLS constraints

    // Constraints are applied per class
    // See constraint_expr_to_string() in libsepol/src/module_to_cil.c
    for (uint32_t class_val = 1; class_val <= pdb->p_classes.nprim;
            ++class_val) {
        class_datum_t *clazz = pdb->class_val_to_struct[class_val - 1];

        for (constraint_node_t *node = clazz->constraints; node;
                node = node->next) {
            for (constraint_expr_t *expr = node->expr; expr;
                    expr = expr->next) {
                if (expr->expr_type == CEXPR_NAMES && ebitmap_get_bit(
                        &expr->type_names->types, attr_val - 1)) {
                    if (ebitmap_set_bit(&expr->names, type_val - 1, 1) < 0) {
                        return SELinuxResult::ERROR;
                    }

                    changed = true;
                }
            }
        }
    }

    return changed ? SELinuxResult::CHANGED : SELinuxResult::UNCHANGED;
}

SELinuxResult selinux_raw_add_to_role(policydb_t *pdb,
                                      uint16_t role_val,
                                      uint16_t type_val)
{
    role_datum_t *role = pdb->role_val_to_struct[role_val - 1];

    if (ebitmap_get_bit(&role->types.types, type_val - 1)) {
        return SELinuxResult::UNCHANGED;
    }

    if (ebitmap_set_bit(&role->types.types, type_val - 1, 1) < 0) {
        return SELinuxResult::ERROR;
    }

    // (See policydb_role_cache() in policydb.c)
#if 0
    ebitmap_destroy(&role->cache);
    return type_set_expand(&role->types, &role->cache, pdb, 1) < 0
            ? SELinuxResult::ERROR
            : SELinuxResult::CHANGED;
#else
    return selinux_raw_reindex(pdb)
            ? SELinuxResult::CHANGED
            : SELinuxResult::ERROR;
#endif
}

bool selinux_raw_reindex(policydb_t *pdb)
{
    // Recreate maps like type_val_to_struct. libsepol will handle memory
    // deallocation for the old maps
    return policydb_index_decls(pdb) == 0
            && policydb_index_classes(pdb) == 0
            && policydb_index_others(nullptr, pdb, 0) == 0;
}

// Static helper functions

static inline class_datum_t * find_class(policydb_t *pdb, const char *name)
{
    return (class_datum_t *) hashtab_search(
            pdb->p_classes.table, (hashtab_key_t) name);
}

static inline perm_datum_t * find_perm(class_datum_t *clazz, const char *name)
{
    perm_datum_t *perm;

    // Find class-specific permissions first
    perm = (perm_datum_t *) hashtab_search(
            clazz->permissions.table, (hashtab_key_t) name);

    // Then try common permissions
    if (!perm && clazz->comdatum) {
        perm = (perm_datum_t *) hashtab_search(
                clazz->comdatum->permissions.table, (hashtab_key_t) name);
    }

    return perm;
}

static inline type_datum_t * find_type(policydb_t *pdb, const char *name)
{
    return (type_datum_t *) hashtab_search(
            pdb->p_types.table, (hashtab_key_t) name);
}

static inline role_datum_t * find_role(policydb_t *pdb, const char *name)
{
    return (role_datum_t *) hashtab_search(
            pdb->p_roles.table, (hashtab_key_t) name);
}

// Helper functions

bool selinux_mount()
{
    // Try /sys/fs/selinux
    if (mount(SELINUX_FS_TYPE, SELINUX_MOUNT_POINT,
              SELINUX_FS_TYPE, 0, nullptr) < 0) {
        LOGW("Failed to mount %s at %s: %s",
             SELINUX_FS_TYPE, SELINUX_MOUNT_POINT, strerror(errno));
        if (errno == ENODEV || errno == ENOENT) {
            LOGI("Kernel does not support SELinux");
        }
        return false;
    }

    return true;
}

bool selinux_unmount()
{
    if (umount(SELINUX_MOUNT_POINT) < 0) {
        LOGE("Failed to unmount %s: %s", SELINUX_MOUNT_POINT, strerror(errno));
        return false;
    }

    return true;
}

bool selinux_make_all_permissive(policydb_t *pdb)
{
    SELinuxResult ret;

    for (uint32_t type_val = 1; type_val <= pdb->p_types.nprim; ++type_val) {
        ret = selinux_raw_set_permissive(pdb, type_val, true);
        if (ret == SELinuxResult::ERROR) {
            return false;
        }
    }

    return true;
}

bool selinux_make_permissive(policydb_t *pdb,
                             const char *type_str)
{
    type_datum_t *type = find_type(pdb, type_str);
    if (!type) {
        LOGV("Type %s not found in policy", type_str);
        return false;
    }

    SELinuxResult result = selinux_raw_set_permissive(pdb, type->s.value, true);
    switch (result) {
    case SELinuxResult::CHANGED:
    case SELinuxResult::UNCHANGED:
        return true;
    case SELinuxResult::ERROR:
        LOGE("Failed to set type %s to permissive", type_str);
    default:
        return false;
    }
}

bool selinux_set_allow_rule(policydb_t *pdb,
                            const char *source_str,
                            const char *target_str,
                            const char *class_str,
                            const char *perm_str,
                            bool remove)
{
    type_datum_t *source, *target;
    class_datum_t *clazz;
    perm_datum_t *perm;

    source = find_type(pdb, source_str);
    if (!source) {
        LOGE("Source type %s does not exist", source_str);
        return false;
    }

    target = find_type(pdb, target_str);
    if (!target) {
        LOGE("Target type %s does not exist", target_str);
        return false;
    }

    clazz = find_class(pdb, class_str);
    if (!clazz) {
        LOGE("Class %s does not exist", class_str);
        return false;
    }

    perm = find_perm(clazz, perm_str);
    if (!perm) {
        LOGE("Perm %s does not exist in class %s", perm_str, class_str);
        return false;
    }

    SELinuxResult result = selinux_raw_set_avtab_rule(
            pdb, source->s.value, target->s.value, clazz->s.value,
            perm->s.value, remove);

    switch (result) {
    case SELinuxResult::CHANGED:
    case SELinuxResult::UNCHANGED:
        return true;
    case SELinuxResult::ERROR:
        LOGE("Failed to add rule: allow %s %s:%s %s;",
             source_str, target_str, class_str, perm_str);
    }

    return false;
}

bool selinux_add_rule(policydb_t *pdb,
                      const char *source_str,
                      const char *target_str,
                      const char *class_str,
                      const char *perm_str)
{
    return selinux_set_allow_rule(
            pdb, source_str, target_str, class_str, perm_str, false);
}

bool selinux_remove_rule(policydb_t *pdb,
                         const char *source_str,
                         const char *target_str,
                         const char *class_str,
                         const char *perm_str)
{
    return selinux_set_allow_rule(
            pdb, source_str, target_str, class_str, perm_str, true);
}

bool selinux_set_type_trans(policydb_t *pdb,
                            const char *source_str,
                            const char *target_str,
                            const char *class_str,
                            const char *default_str)
{
    type_datum_t *source, *target, *def;
    class_datum_t *clazz;

    source = find_type(pdb, source_str);
    if (!source) {
        LOGE("Source type %s does not exist", source_str);
        return false;
    }

    target = find_type(pdb, target_str);
    if (!target) {
        LOGE("Target type %s does not exist", target_str);
        return false;
    }

    clazz = find_class(pdb, class_str);
    if (!clazz) {
        LOGE("Class %s does not exist", class_str);
        return false;
    }

    def = find_type(pdb, default_str);
    if (!def) {
        LOGE("Default type %s does not exist", default_str);
        return false;
    }

    SELinuxResult result = selinux_raw_set_type_trans(
            pdb, source->s.value, target->s.value, clazz->s.value,
            def->s.value);

    switch (result) {
    case SELinuxResult::CHANGED:
    case SELinuxResult::UNCHANGED:
        return true;
    case SELinuxResult::ERROR:
        LOGE("Failed to add type transition: type_transition %s %s:%s %s;",
             source_str, target_str, class_str, default_str);
    }

    return false;
}

bool selinux_grant_all_perms(policydb_t *pdb,
                             const char *source_type_str,
                             const char *target_type_str,
                             const char *class_str)
{
    type_datum_t *source = find_type(pdb, source_type_str);
    if (!source) {
        return false;
    }

    type_datum_t *target = find_type(pdb, target_type_str);
    if (!target) {
        return false;
    }

    class_datum_t *clazz = find_class(pdb, class_str);
    if (!clazz) {
        return false;
    }

    auto ret = selinux_raw_grant_all_perms(
            pdb, source->s.value, target->s.value, clazz->s.value);
    return ret != SELinuxResult::ERROR;
}

bool selinux_grant_all_perms(policydb_t *pdb,
                             const char *source_type_str,
                             const char *target_type_str)
{
    type_datum_t *source = find_type(pdb, source_type_str);
    if (!source) {
        return false;
    }

    type_datum_t *target = find_type(pdb, target_type_str);
    if (!target) {
        return false;
    }

    auto ret = selinux_raw_grant_all_perms(
            pdb, source->s.value, target->s.value);
    return ret != SELinuxResult::ERROR;
}

/*!
 * \brief Create type in SELinux binary policy
 *
 * \param pdb Policy object
 * \param name Name of type to add
 *
 * \return Whether the type was created
 */
SELinuxResult selinux_create_type(policydb_t *pdb,
                                  const char *name)
{
    if (find_type(pdb, name)) {
        // Type already exists
        return SELinuxResult::UNCHANGED;
    }

    // symtab_insert will take ownership of these allocations
    char *name_dup = strdup(name);
    if (!name_dup) {
        return SELinuxResult::ERROR;
    }

    type_datum_t *new_type = (type_datum_t *) malloc(sizeof(type_datum_t));
    if (!new_type) {
        free(name_dup);
        return SELinuxResult::ERROR;
    }

    // We're creating a type, not an attribute
    type_datum_init(new_type);
    new_type->primary = 1;
    new_type->flavor = TYPE_TYPE;

    // New value for the type
    uint32_t type_val;

    // Add type declaration to symbol table
    int ret = symtab_insert(
            pdb, SYM_TYPES, name_dup, new_type, SCOPE_DECL, 1, &type_val);
    if (ret != 0) {
        // Policy file is broken if, somehow, ret == 1
        free(name_dup);
        free(new_type);
        return SELinuxResult::ERROR;
    }

    new_type->s.value = type_val;

    if (ebitmap_set_bit(&pdb->global->branch_list->declared.scope[SYM_TYPES],
                        type_val - 1, 1) != 0) {
        return SELinuxResult::ERROR;
    }

    // Reallocate type-attribute maps for the new type
    // (see: policydb_read() in policydb.c)
    ebitmap_t *new_type_attr_map = (ebitmap_t *) realloc(
            pdb->type_attr_map, sizeof(ebitmap_t) * pdb->p_types.nprim);
    if (new_type_attr_map) {
        pdb->type_attr_map = new_type_attr_map;
    } else {
        return SELinuxResult::ERROR;
    }

    ebitmap_t *new_attr_type_map = (ebitmap_t *) realloc(
            pdb->attr_type_map, sizeof(ebitmap_t) * pdb->p_types.nprim);
    if (new_attr_type_map) {
        pdb->attr_type_map = new_attr_type_map;
    } else {
        return SELinuxResult::ERROR;
    }

    // Initialize bitmap
    ebitmap_init(&pdb->type_attr_map[type_val - 1]);
    ebitmap_init(&pdb->attr_type_map[type_val - 1]);

    // Handle degenerate case
    if (ebitmap_set_bit(&pdb->type_attr_map[type_val - 1],
                        type_val - 1, 1) < 0) {
        return SELinuxResult::ERROR;
    }

    if (!selinux_raw_reindex(pdb)) {
        return SELinuxResult::ERROR;
    }

    return SELinuxResult::CHANGED;
}

/*!
 * \brief Set attribute for a type
 *
 * \param pdb Policy object
 * \param type_name Name of type
 * \param attr_name Name of attribute
 *
 * \return True if the policy was successfully set or already set
 *         False if the specified type and attribute do not exist or if an
 *         error occurs
 */
bool selinux_set_attribute(policydb_t *pdb,
                           const char *type_name,
                           const char *attr_name)
{
    // Find type
    type_datum_t *type = find_type(pdb, type_name);
    if (!type || type->flavor != TYPE_TYPE) {
        return false;
    }

    // Find attribute
    type_datum_t *attr = find_type(pdb, attr_name);
    if (!attr || attr->flavor != TYPE_ATTRIB) {
        return false;
    }

    auto ret = selinux_raw_set_attribute(pdb, type->s.value, attr->s.value);
    return ret != SELinuxResult::ERROR;
}

bool selinux_add_to_role(policydb_t *pdb,
                         const char *role_name,
                         const char *type_name)
{
    role_datum_t *role = find_role(pdb, role_name);
    if (!role) {
        return false;
    }

    type_datum_t *type = find_type(pdb, type_name);
    if (!type) {
        return false;
    }

    auto ret = selinux_raw_add_to_role(pdb, role->s.value, type->s.value);
    return ret != SELinuxResult::ERROR;
}

void selinux_strip_no_audit(policydb_t *pdb)
{
#if 0
    // This implementation works, but is confusing since it won't be printed
    // correctly via libsepol (including sesearch):
    // https://github.com/SELinuxProject/selinux/issues/23

    for (uint32_t i = 0; i < pdb->te_avtab.nslot; i++) {
        for (avtab_ptr_t cur = pdb->te_avtab.htable[i]; cur; cur = cur->next) {
            if (cur->key.specified & AVTAB_AUDITDENY) {
                // Clear all permission bits
                // (auditdeny/dontaudit datum is a mask)
                cur->datum.data = ~0U;
            } else if (cur->key.specified & AVTAB_XPERMS_DONTAUDIT) {
                // xperms currently not handled
            }
        }
    }
#else
    // This alternative implementation removes the key from avtab, which will
    // work correctly with every tool.

    for (uint32_t i = 0; i < pdb->te_avtab.nslot; i++) {
        avtab_ptr_t prev = nullptr;
        for (avtab_ptr_t cur = pdb->te_avtab.htable[i]; cur;) {
            if ((cur->key.specified & AVTAB_AUDITDENY)
                    || (cur->key.specified & AVTAB_XPERMS_DONTAUDIT)) {
                avtab_ptr_t to_free = cur;

                if (prev) {
                    prev->next = cur = cur->next;
                } else {
                    pdb->te_avtab.htable[i] = cur = cur->next;
                }

                if (to_free->key.specified & AVTAB_XPERMS) {
                    free(to_free->datum.xperms);
                }
                free(to_free);

                --pdb->te_avtab.nel;

                // Don't advance pointer
            } else {
                prev = cur;
                cur = cur->next;
            }
        }
    }
#endif
}

// Patching functions

// Fail fast
#define ff(expr) \
    do { \
        if (!(expr)) return false; \
    } while (0)

static inline bool add_rules(policydb_t *pdb,
                             const char *source,
                             const char *target,
                             const char *clazz,
                             const std::vector<std::string> &perms)
{
    for (auto const &perm : perms) {
        if (!selinux_add_rule(pdb, source, target, clazz, perm.c_str())) {
            return false;
        }
    }

    return true;
}

MB_UNUSED
static inline bool remove_rules(policydb_t *pdb,
                                const char *source,
                                const char *target,
                                const char *clazz,
                                const std::vector<std::string> &perms)
{
    for (auto const &perm : perms) {
        if (!selinux_remove_rule(pdb, source, target, clazz, perm.c_str())) {
            return false;
        }
    }

    return true;
}

static bool apply_pre_boot_patches(policydb_t *pdb)
{
    // We are going to allow everything. The stage 1 policy is not a security
    // concern because the real (secure) policy will be loaded by the real /init
    // binary. This temporary policy exists solely for stage 2 to prepare the
    // environment for the real /init.

    // For most ROMs, we can just make the kernel domain permissive
    ff(selinux_make_permissive(pdb, "kernel"));

    // For TW 6.0 ROMs, the kernel #define's the permissive flag to be 0, so
    // per-type permissive flags are completely ignored. For these ROMs, we'll
    // allow every attribute to do everything.

    type_datum_t *kernel = find_type(pdb, "kernel");
    if (!kernel) {
        return false;
    }

    // For all attributes
    for (uint32_t type_val = 1; type_val <= pdb->p_types.nprim; ++type_val) {
        // Skip non-attributes
        if (pdb->type_val_to_struct[type_val - 1]->flavor != TYPE_ATTRIB) {
            continue;
        }

        auto ret = selinux_raw_grant_all_perms(pdb, kernel->s.value, type_val);
        switch (ret) {
        case SELinuxResult::CHANGED:
        case SELinuxResult::UNCHANGED:
            break;
        case SELinuxResult::ERROR:
            LOGE("Failed to grant all perms for: %s -> %s",
                 "kernel", pdb->p_type_val_to_name[type_val - 1]);
            return false;
        }
    }

    // Allow the real init to load the "secure" SELinux policy
    ff(selinux_add_rule(pdb, "kernel", "kernel", "security", "load_policy"));

    return true;
}

static bool copy_avtab_rules(policydb_t *pdb,
                             const char *source_type,
                             const char *target_type)
{
    std::vector<std::pair<avtab_key_t, avtab_datum_t>> to_add;

    type_datum_t *source, *target;

    if (strcmp(source_type, target_type) == 0) {
        LOGE("Source and target types are the same: %s", source_type);
        return false;
    }

    source = find_type(pdb, source_type);
    if (!source) {
        LOGE("Source type %s does not exist", source_type);
        return false;
    }

    target = find_type(pdb, target_type);
    if (!target) {
        LOGE("Target type %s does not exist", target_type);
        return false;
    }

    // Gather rules to copy
    for (uint32_t i = 0; i < pdb->te_avtab.nslot; ++i) {
        for (avtab_ptr_t cur = pdb->te_avtab.htable[i]; cur; cur = cur->next) {
            if (!(cur->key.specified & AVTAB_ALLOWED)) {
                continue;
            }

            if (cur->key.target_type == source->s.value) {
                avtab_key_t copy = cur->key;
                copy.target_type = target->s.value;

                to_add.push_back(std::make_pair(std::move(copy), cur->datum));
            }
        }
    }

    avtab_datum_t *datum;
    for (auto &pair : to_add) {
        datum = avtab_search(&pdb->te_avtab, &pair.first);

        if (!datum) {
            // Create new avtab rule if the key doesn't exist
            if (avtab_insert(&pdb->te_avtab, &pair.first, &pair.second) != 0) {
                LOGE("Failed to add rule to avtab");
                return false;
            }
        } else {
            // Add additional perms if the key already exists
            datum->data |= pair.second.data;
        }
    }

    return true;
}

/*!
 * \brief Patch SEPolicy to allow media_data_file-labeled /data/media to work on
 *        Android >= 5.0
 */
static bool fix_data_media_rules(policydb_t *pdb)
{
    static const char *expected_type = "media_rw_data_file";
    const char *path = INTERNAL_STORAGE;

    if (!find_type(pdb, expected_type)) {
        LOGW("Type %s doesn't exist. Won't touch %s related rules",
             expected_type, INTERNAL_STORAGE);
        return true;
    }

    std::string context;
    if (!util::selinux_lget_context(path, &context)) {
        LOGE("%s: Failed to get context: %s", path, strerror(errno));
        path = "/data/media";
        if (!util::selinux_lget_context(path, &context)) {
            LOGE("%s: Failed to get context: %s", path, strerror(errno));
            // Don't fail if /data/media does not exist
            return errno == ENOENT;
        }
    }

    std::vector<std::string> pieces = util::split(context, ":");
    if (pieces.size() < 3) {
        LOGE("%s: Malformed context string: %s", path, context.c_str());
        return false;
    }
    const std::string &type = pieces[2];

    if (type == expected_type) {
        return true;
    }

    LOGV("Copying %s rules to %s because of improper %s SELinux label",
         expected_type, type.c_str(), path);
    ff(copy_avtab_rules(pdb, expected_type, type.c_str()));

    // Required for MLS on Android 7.1
    ff(selinux_set_attribute(pdb, type.c_str(), "mlstrustedobject"));

    return true;
}

static bool create_mbtool_types(policydb_t *pdb)
{
    // Used for running any mbtool commands
    ff(selinux_create_type(pdb, "mb_exec") != SELinuxResult::ERROR);
    ff(selinux_add_to_role(pdb, "r", "mb_exec"));
    ff(selinux_set_attribute(pdb, "mb_exec", "domain"));
    ff(selinux_set_attribute(pdb, "mb_exec", "mlstrustedobject"));
    ff(selinux_set_attribute(pdb, "mb_exec", "mlstrustedsubject"));

    // Allow setting the current process context from init to mb_exec
    ff(add_rules(pdb, "init", "mb_exec", "process", {
        "noatsecure", "rlimitinh", "setcurrent", "siginh", "transition",
        //"dyntransition",
    }));

    // Allow installd to connect to appsync's socket
    ff(add_rules(pdb, "installd", "mb_exec", "unix_stream_socket", {
        "accept", "listen", "read", "write",
    }));
    if (find_type(pdb, "system_server")) {
        ff(add_rules(pdb, "system_server", "mb_exec", "unix_stream_socket", {
            "connectto",
        }));
    } else {
        ff(add_rules(pdb, "system", "mb_exec", "unix_stream_socket", {
            "connectto",
        }));
    }

    // Allow apps to connect to the daemon
    ff(add_rules(pdb, "untrusted_app", "mb_exec", "unix_stream_socket", {
        "connectto",
    }));

    // Allow zygote to write to our stdout pipe when rebooting
    ff(add_rules(pdb, "zygote", "init", "fifo_file", { "write" }));

    // Allow rebooting via the android.intent.action.REBOOT intent
    if (find_type(pdb, "activity_service")) {
        ff(add_rules(pdb, "zygote", "activity_service", "service_manager", { "find" }));
    }
    if (find_type(pdb, "system_server")) {
        ff(add_rules(pdb, "zygote", "system_server", "binder", { "call" }));
    }

    ff(add_rules(pdb, "zygote", "init", "unix_stream_socket", { "read", "write" }));
    ff(add_rules(pdb, "zygote", "servicemanager", "binder", { "call" }));

    ff(add_rules(pdb, "servicemanager", "mb_exec", "binder", { "transfer" }));
    ff(add_rules(pdb, "servicemanager", "mb_exec", "dir", { "search" }));
    ff(add_rules(pdb, "servicemanager", "mb_exec", "file", { "open", "read" }));
    ff(add_rules(pdb, "servicemanager", "mb_exec", "process", { "getattr" }));
    ff(add_rules(pdb, "servicemanager", "zygote", "dir", { "search" }));
    ff(add_rules(pdb, "servicemanager", "zygote", "file", { "open" }));
    ff(add_rules(pdb, "servicemanager", "zygote", "file", { "read" }));
    ff(add_rules(pdb, "servicemanager", "zygote", "process", { "getattr" }));

    // For in-app flashing
    ff(add_rules(pdb, "rootfs", "tmpfs", "filesystem", { "associate" }));
    ff(add_rules(pdb, "tmpfs",  "rootfs", "filesystem", { "associate" }));
    ff(add_rules(pdb, "kernel", "mb_exec", "fd", { "use" }));

    // Give mb_exec <insert diety here> permissions
    type_datum_t *mb_exec = find_type(pdb, "mb_exec");
    if (!mb_exec) {
        return false;
    }

    // For all attributes
    for (uint32_t type_val = 1; type_val <= pdb->p_types.nprim;
            ++type_val) {
        // Skip non-attributes
        if (pdb->type_val_to_struct[type_val - 1]->flavor
                != TYPE_ATTRIB) {
            continue;
        }

        auto ret2 = selinux_raw_grant_all_perms(
                pdb, mb_exec->s.value, type_val);
        switch (ret2) {
        case SELinuxResult::CHANGED:
        case SELinuxResult::UNCHANGED:
            break;
        case SELinuxResult::ERROR:
            LOGE("Failed to grant all perms for: %s -> %s",
                 "mb_exec", pdb->p_type_val_to_name[type_val - 1]);
            return false;
        }
    }

    return true;
}

static bool apply_main_patches(policydb_t *pdb)
{
    ff(fix_data_media_rules(pdb));
    ff(create_mbtool_types(pdb));

    return true;
}

static bool apply_cwm_recovery_patches(policydb_t *pdb)
{
    // Debugging rules (for CWM and Philz)
    ff(add_rules(pdb, "adbd",  "block_device",    "blk_file",   { "relabelto" }));
    ff(add_rules(pdb, "adbd",  "graphics_device", "chr_file",   { "relabelto" }));
    ff(add_rules(pdb, "adbd",  "graphics_device", "dir",        { "relabelto" }));
    ff(add_rules(pdb, "adbd",  "input_device",    "chr_file",   { "relabelto" }));
    ff(add_rules(pdb, "adbd",  "input_device",    "dir",        { "relabelto" }));
    ff(add_rules(pdb, "adbd",  "rootfs",          "dir",        { "relabelto" }));
    ff(add_rules(pdb, "adbd",  "rootfs",          "file",       { "relabelto" }));
    ff(add_rules(pdb, "adbd",  "rootfs",          "lnk_file",   { "relabelto" }));
    ff(add_rules(pdb, "adbd",  "system_file",     "file",       { "relabelto" }));
    ff(add_rules(pdb, "adbd",  "tmpfs",           "file",       { "relabelto" }));

    ff(add_rules(pdb, "rootfs", "tmpfs",          "filesystem", { "associate" }));
    ff(add_rules(pdb, "tmpfs",  "rootfs",         "filesystem", { "associate" }));

    return true;
}

bool selinux_apply_patch(policydb_t *pdb, SELinuxPatch patch)
{
    bool ret = false;

    switch (patch) {
    case SELinuxPatch::PRE_BOOT:
        ret = apply_pre_boot_patches(pdb);
        break;
    case SELinuxPatch::MAIN:
        ret = apply_main_patches(pdb);
        break;
    case SELinuxPatch::CWM_RECOVERY:
        ret = apply_cwm_recovery_patches(pdb);
        break;
    case SELinuxPatch::STRIP_NO_AUDIT:
        selinux_strip_no_audit(pdb);
        ret = true;
        break;
    case SELinuxPatch::NONE:
        break;
    }

    return ret;
}

bool patch_sepolicy(const std::string &source,
                    const std::string &target,
                    SELinuxPatch patch)
{
    policydb_t pdb;

    if (policydb_init(&pdb) < 0) {
        LOGE("Failed to initialize policydb");
        return false;
    }

    auto destroy_pdb = util::finally([&]{
        policydb_destroy(&pdb);
    });

    if (!util::selinux_read_policy(source, &pdb)) {
        LOGE("%s: Failed to load SELinux policy", source.c_str());
        return false;
    }

    LOGD("Policy version: %u", pdb.policyvers);

    if (!selinux_apply_patch(&pdb, patch)) {
        LOGE("%s: Failed to apply policy patch", source.c_str());
        return false;
    }

    if (!util::selinux_write_policy(target, &pdb)) {
        LOGE("%s: Failed to write SELinux policy", target.c_str());
        return false;
    }

    return true;
}

bool patch_loaded_sepolicy(SELinuxPatch patch)
{
    autoclose::file fp(autoclose::fopen(SELINUX_ENFORCE_FILE, "rbe"));
    if (!fp) {
        if (errno == ENOENT) {
            // If the file doesn't exist, then the kernel probably doesn't
            // support SELinux
            LOGV("Kernel does not support SELinux. Policy won't be patched");
            return true;
        } else {
            LOGE("%s: Failed to open file: %s",
                 SELINUX_ENFORCE_FILE, strerror(errno));
            return false;
        }
    }

    return patch_sepolicy(SELINUX_POLICY_FILE, SELINUX_LOAD_FILE, patch);
}

static void sepolpatch_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: sepolpatch [OPTION]...\n\n"
            "Options:\n"
            "  -s [SOURCE], --source [SOURCE]\n"
            "                      Source policy file to patch\n"
            "  -t [TARGET], --target [TARGET]\n"
            "                      Target policy file to patch\n"
            "  --loaded            Patch currently loaded policy\n"
            "  -p [PATCH], --patch [PATCH]\n"
            "                      Policy patch to apply\n"
            "  -l, --list-patches  List available policy patches\n"
            "  -h, --help          Display this help message\n"
            "\n"
            "If --source is omitted, the source path is set to /sys/fs/selinux/policy.\n"
            "If --target is omitted, the target path is set to /sys/fs/selinux/load.\n"
            "Note that, unlike --loaded, sepolpatch will not check if SELinux is\n"
            "supported, enabled, and enforcing before patching.\n\n"
            "Note: The source and target file can be set to the same path to patch\n"
            "the policy file in place.\n");
}

struct {
    const char *name;
    SELinuxPatch patch;
} patches[] = {
    { "pre_boot",       SELinuxPatch::PRE_BOOT },
    { "main",           SELinuxPatch::MAIN },
    { "cwm_recovery",   SELinuxPatch::CWM_RECOVERY },
    { "strip_no_audit", SELinuxPatch::STRIP_NO_AUDIT },
    { nullptr,          SELinuxPatch::NONE },
};

int sepolpatch_main(int argc, char *argv[])
{
    int opt;
    const char *source_file = nullptr;
    const char *target_file = nullptr;
    const char *patch = nullptr;
    bool flag_loaded = false;
    bool flag_list_patches = false;

    enum {
        OPT_LOADED = CHAR_MAX + 1,
    };

    static struct option long_options[] = {
        {"source",       required_argument, 0, 's'},
        {"target",       required_argument, 0, 't'},
        {"loaded",       no_argument,       0, OPT_LOADED},
        {"patch",        required_argument, 0, 'p'},
        {"list-patches", no_argument,       0, 'l'},
        {"help",         no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    static const char short_options[] = "s:t:p:lh";

    int long_index = 0;

    while ((opt = getopt_long(
            argc, argv, short_options, long_options, &long_index)) != -1) {
        switch (opt) {
        case 's':
            source_file = optarg;
            break;

        case 't':
            target_file = optarg;
            break;

        case OPT_LOADED:
            flag_loaded = true;
            break;

        case 'p':
            patch = optarg;
            break;

        case 'l':
            flag_list_patches = true;
            break;

        case 'h':
            sepolpatch_usage(stdout);
            return EXIT_SUCCESS;

        default:
            sepolpatch_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    // There should be no other arguments
    if (argc - optind != 0) {
        sepolpatch_usage(stderr);
        return EXIT_FAILURE;
    }

    if (flag_list_patches
            && (source_file || target_file || flag_loaded || patch)) {
        fprintf(stderr, "--list-patches cannot be used with other options\n");
        return EXIT_FAILURE;
    }

    if (flag_list_patches) {
        for (auto it = patches; it->name; ++it) {
            printf("%s\n", it->name);
        }

        return EXIT_SUCCESS;
    } else {
        if (!patch) {
            fprintf(stderr, "A patch must be specified via --patch\n");
            return EXIT_FAILURE;
        }

        SELinuxPatch patch_type(SELinuxPatch::NONE);

        for (auto it = patches; it->name; ++it) {
            if (strcmp(it->name, patch) == 0) {
                patch_type = it->patch;
                break;
            }
        }

        if (patch_type == SELinuxPatch::NONE) {
            fprintf(stderr, "Invalid patch: %s\n", patch);
            return EXIT_FAILURE;
        }

        if (flag_loaded) {
            if ((source_file || target_file)) {
                fprintf(stderr, "--loaded cannot be used with --source or --target\n");
                return EXIT_FAILURE;
            }

            return patch_loaded_sepolicy(patch_type)
                    ? EXIT_SUCCESS : EXIT_FAILURE;
        } else {
            if (!source_file) {
                source_file = SELINUX_POLICY_FILE;
            }
            if (!target_file) {
                target_file = SELINUX_LOAD_FILE;
            }

            return patch_sepolicy(source_file, target_file, patch_type)
                    ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }
}

}
