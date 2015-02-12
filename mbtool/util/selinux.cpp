/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "util/selinux.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>

#include <sepol/sepol.h>

#include "util/logging.h"


namespace mb
{
namespace util
{

bool selinux_read_policy(const std::string &path, policydb_t *pdb)
{
    struct policy_file pf;
    struct stat sb;
    void *map;
    int fd;
    int ret;

    fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        LOGE("Failed to open %s: %s", path, strerror(errno));
        return false;
    }

    if (fstat(fd, &sb) < 0) {
        LOGE("Failed to stat %s: %s", path, strerror(errno));
        close(fd);
        return false;
    }

    map = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        LOGE("Failed to mmap %s: %s", path, strerror(errno));
        close(fd);
        return false;
    }

    policy_file_init(&pf);
    pf.type = PF_USE_MEMORY;
    pf.data = (char *) map;
    pf.len = sb.st_size;

    ret = policydb_read(pdb, &pf, 0);

    sepol_handle_destroy(pf.handle);
    munmap(map, sb.st_size);
    close(fd);

    return ret == 0 ? true : false;
}

// /sys/fs/selinux/load requires the entire policy to be written in a single
// write(2) call.
// See: http://marc.info/?l=selinux&m=141882521027239&w=2
bool selinux_write_policy(const std::string &path, policydb_t *pdb)
{
    void *data;
    size_t len;
    sepol_handle_t *handle;
    int fd;

    // Don't print warnings to stderr
    handle = sepol_handle_create();
    sepol_msg_set_callback(handle, NULL, NULL);

    if (policydb_to_image(handle, pdb, &data, &len) < 0) {
        LOGE("Failed to write policydb to memory");
        sepol_handle_destroy(handle);
        return false;
    }

    fd = open(path.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (fd < 0) {
        LOGE("Failed to open %s: %s", path, strerror(errno));
        sepol_handle_destroy(handle);
        free(data);
        return false;
    }

    if (write(fd, data, len) < 0) {
        LOGE("Failed to write to %s: %s", path, strerror(errno));
        sepol_handle_destroy(handle);
        close(fd);
        free(data);
        return false;
    }

    sepol_handle_destroy(handle);
    close(fd);
    free(data);

    return true;
}

bool selinux_make_permissive(policydb_t *pdb, const std::string &type_str)
{
    type_datum_t *type;

    type = (type_datum_t *) hashtab_search(
            pdb->p_types.table, (hashtab_key_t) type_str.c_str());
    if (!type) {
        LOGV("Type %s not found in policy", type_str);
        return false;
    }

    if (ebitmap_get_bit(&pdb->permissive_map, type->s.value)) {
        LOGV("Type %s is already permissive", type_str);
        return true;
    }

    if (ebitmap_set_bit(&pdb->permissive_map, type->s.value, 1) < 0) {
        LOGE("Failed to set bit for type %s in the permissive map", type_str);
        return false;
    }

    LOGD("Type %s is now permissive", type_str);

    return true;
}

// Based on public domain code from sepolicy-inject:
// https://bitbucket.org/joshua_brindle/sepolicy-inject/
// See the following commit about the hashtab_key_t casts:
// https://github.com/TresysTechnology/setools/commit/2994d1ca1da9e6f25f082c0dd1a49b5f958bd2ca
bool selinux_add_rule(policydb_t *pdb,
                      const std::string &source_str,
                      const std::string &target_str,
                      const std::string &class_str,
                      const std::string &perm_str)
{
    type_datum_t *source, *target;
    class_datum_t *clazz;
    perm_datum_t *perm;
    avtab_datum_t *av;
    avtab_key_t key;

    source = (type_datum_t *) hashtab_search(
            pdb->p_types.table, (hashtab_key_t) source_str.c_str());
    if (!source) {
        LOGE("Source type %s does not exist", source_str);
        return false;
    }
    target = (type_datum_t *) hashtab_search(
            pdb->p_types.table, (hashtab_key_t) target_str.c_str());
    if (!target) {
        LOGE("Target type %s does not exist", target_str);
        return false;
    }
    clazz = (class_datum_t *) hashtab_search(
            pdb->p_classes.table, (hashtab_key_t) class_str.c_str());
    if (!clazz) {
        LOGE("Class %s does not exist", class_str);
        return false;
    }
    perm = (perm_datum_t *) hashtab_search(
            clazz->permissions.table, (hashtab_key_t) perm_str.c_str());
    if (!perm) {
        if (clazz->comdatum == NULL) {
            LOGE("Perm %s does not exist in class %s", perm_str, class_str);
            return false;
        }
        perm = (perm_datum_t *) hashtab_search(
                clazz->comdatum->permissions.table,
                (hashtab_key_t) perm_str.c_str());
        if (!perm) {
            LOGE("Perm %s does not exist in class %s", perm_str, class_str);
            return false;
        }
    }

    // See if there is already a rule
    key.source_type = source->s.value;
    key.target_type = target->s.value;
    key.target_class = clazz->s.value;
    key.specified = AVTAB_ALLOWED;
    av = avtab_search(&pdb->te_avtab, &key);

    if (!av) {
        av = (avtab_datum_t *) malloc(sizeof(av));
        if (!av) {
            LOGE("Out of memory");
            return false;
        }

        av->data |= 1U << (perm->s.value - 1);
        if (avtab_insert(&pdb->te_avtab, &key, av) != 0) {
            free(av);
            LOGE("Failed to add rule to avtab");
            return false;
        }

        free(av);
    }

    av->data |= 1U << (perm->s.value - 1);

    LOGD("Added rule: \"allow %s %s:%s %s;\"",
         source_str, target_str, class_str, perm_str);

    return true;
}

bool selinux_set_context(const std::string &path, const std::string &context)
{
    return setxattr(path.c_str(), "security.selinux",
                    context.c_str(), context.size() + 1, 0);
}

bool selinux_lset_context(const std::string &path, const std::string &context)
{
    return lsetxattr(path.c_str(), "security.selinux",
                     context.c_str(), context.size() + 1, 0);
}

}
}