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

#include "mbutil/selinux.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>

#include <sepol/sepol.h>

#include "mblog/logging.h"
#include "mbutil/finally.h"
#include "mbutil/fts.h"
#include "mbutil/mount.h"

#define SELINUX_MOUNT_POINT     "/sys/fs/selinux"
#define SELINUX_FS_TYPE         "selinuxfs"

#define SELINUX_XATTR           "security.selinux"

#define DEFAULT_SEPOLICY_FILE   "/sepolicy"


namespace mb
{
namespace util
{

class RecursiveSetContext : public FTSWrapper {
public:
    RecursiveSetContext(std::string path, std::string context,
                        bool follow_symlinks)
        : FTSWrapper(path, FTS_GroupSpecialFiles),
        _context(std::move(context)),
        _follow_symlinks(follow_symlinks)
    {
    }

    virtual int on_reached_directory_post() override
    {
        return set_context() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_file() override
    {
        return set_context() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_symlink() override
    {
        return set_context() ? Action::FTS_OK : Action::FTS_Fail;
    }

    virtual int on_reached_special_file() override
    {
        return set_context() ? Action::FTS_OK : Action::FTS_Fail;
    }

private:
    std::string _context;
    bool _follow_symlinks;

    bool set_context()
    {
        if (_follow_symlinks) {
            return selinux_set_context(_curr->fts_accpath, _context);
        } else {
            return selinux_lset_context(_curr->fts_accpath, _context);
        }
    }
};

bool selinux_mount()
{
    // Try /sys/fs/selinux
    if (!util::mount(SELINUX_FS_TYPE, SELINUX_MOUNT_POINT,
                     SELINUX_FS_TYPE, 0, nullptr)) {
        LOGW("Failed to mount %s at %s: %s",
             SELINUX_FS_TYPE, SELINUX_MOUNT_POINT, strerror(errno));
        if (errno == ENODEV || errno == ENOENT) {
            LOGI("Kernel does not support SELinux");
        }
        return false;
    }

    // Load default policy
    struct stat sb;
    if (stat(DEFAULT_SEPOLICY_FILE, &sb) == 0) {
        policydb_t pdb;

        if (policydb_init(&pdb) < 0) {
            LOGE("Failed to initialize policydb");
            return false;
        }

        if (!selinux_read_policy(DEFAULT_SEPOLICY_FILE, &pdb)) {
            LOGE("Failed to read SELinux policy file: %s",
                 DEFAULT_SEPOLICY_FILE);
            policydb_destroy(&pdb);
            return false;
        }

        // Make all types permissive. Otherwise, some more restrictive policies
        // will prevent the real init from loading /sepolicy because init
        // (stage 1) runs under the `u:r:kernel:s0` context.
        util::selinux_make_all_permissive(&pdb);

        if (!selinux_write_policy(SELINUX_LOAD_FILE, &pdb)) {
            LOGE("Failed to write SELinux policy file: %s",
                 SELINUX_LOAD_FILE);
            policydb_destroy(&pdb);
            return false;
        }

        policydb_destroy(&pdb);

        return true;
    }

    return true;
}

bool selinux_unmount()
{
    if (!util::is_mounted(SELINUX_MOUNT_POINT)) {
        LOGI("No SELinux filesystem to unmount");
        return false;
    }

    if (!util::umount(SELINUX_MOUNT_POINT)) {
        LOGE("Failed to unmount %s: %s", SELINUX_MOUNT_POINT, strerror(errno));
        return false;
    }

    return true;
}

bool selinux_read_policy(const std::string &path, policydb_t *pdb)
{
    struct policy_file pf;
    struct stat sb;
    void *map;
    int fd;

    fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        LOGE("Failed to open %s: %s", path.c_str(), strerror(errno));
        return false;
    }

    auto close_fd = finally([&] {
        close(fd);
    });

    if (fstat(fd, &sb) < 0) {
        LOGE("Failed to stat %s: %s", path.c_str(), strerror(errno));
        return false;
    }

    map = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        LOGE("Failed to mmap %s: %s", path.c_str(), strerror(errno));
        return false;
    }

    auto unmap_map = finally([&] {
        munmap(map, sb.st_size);
    });

    policy_file_init(&pf);
    pf.type = PF_USE_MEMORY;
    pf.data = (char *) map;
    pf.len = sb.st_size;

    auto destroy_pf = finally([&] {
        sepol_handle_destroy(pf.handle);
    });

    return policydb_read(pdb, &pf, 0) == 0;
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
    sepol_msg_set_callback(handle, nullptr, nullptr);

    auto destroy_handle = finally([&] {
        sepol_handle_destroy(handle);
    });

    if (policydb_to_image(handle, pdb, &data, &len) < 0) {
        LOGE("Failed to write policydb to memory");
        return false;
    }

    auto free_data = finally([&] {
        free(data);
    });

    fd = open(path.c_str(), O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (fd < 0) {
        LOGE("Failed to open %s: %s", path.c_str(), strerror(errno));
        return false;
    }

    auto close_fd = finally([&] {
        close(fd);
    });

    if (write(fd, data, len) < 0) {
        LOGE("Failed to write to %s: %s", path.c_str(), strerror(errno));
        return false;
    }

    return true;
}

void selinux_make_all_permissive(policydb_t *pdb)
{
    //char *name;

    for (unsigned int i = 0; i < pdb->p_types.nprim - 1; i++) {
        //name = pdb->p_type_val_to_name[i];
        //if (ebitmap_get_bit(&pdb->permissive_map, i + 1)) {
        //    LOGD("Type %s is already permissive", name);
        //} else {
            ebitmap_set_bit(&pdb->permissive_map, i + 1, 1);
        //    LOGD("Made %s permissive", name);
        //}
    }
}

bool selinux_make_permissive(policydb_t *pdb, const std::string &type_str)
{
    type_datum_t *type;

    type = (type_datum_t *) hashtab_search(
            pdb->p_types.table, (hashtab_key_t) type_str.c_str());
    if (!type) {
        LOGV("Type %s not found in policy", type_str.c_str());
        return false;
    }

    if (ebitmap_get_bit(&pdb->permissive_map, type->s.value)) {
        LOGV("Type %s is already permissive", type_str.c_str());
        return true;
    }

    if (ebitmap_set_bit(&pdb->permissive_map, type->s.value, 1) < 0) {
        LOGE("Failed to set bit for type %s in the permissive map",
             type_str.c_str());
        return false;
    }

    LOGD("Type %s is now permissive", type_str.c_str());

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
        LOGE("Source type %s does not exist", source_str.c_str());
        return false;
    }
    target = (type_datum_t *) hashtab_search(
            pdb->p_types.table, (hashtab_key_t) target_str.c_str());
    if (!target) {
        LOGE("Target type %s does not exist", target_str.c_str());
        return false;
    }
    clazz = (class_datum_t *) hashtab_search(
            pdb->p_classes.table, (hashtab_key_t) class_str.c_str());
    if (!clazz) {
        LOGE("Class %s does not exist", class_str.c_str());
        return false;
    }
    perm = (perm_datum_t *) hashtab_search(
            clazz->permissions.table, (hashtab_key_t) perm_str.c_str());
    if (!perm) {
        if (clazz->comdatum == nullptr) {
            LOGE("Perm %s does not exist in class %s",
                 perm_str.c_str(), class_str.c_str());
            return false;
        }
        perm = (perm_datum_t *) hashtab_search(
                clazz->comdatum->permissions.table,
                (hashtab_key_t) perm_str.c_str());
        if (!perm) {
            LOGE("Perm %s does not exist in class %s",
                 perm_str.c_str(), class_str.c_str());
            return false;
        }
    }

    // See if there is already a rule
    key.source_type = source->s.value;
    key.target_type = target->s.value;
    key.target_class = clazz->s.value;
    key.specified = AVTAB_ALLOWED;
    av = avtab_search(&pdb->te_avtab, &key);

    bool exists = false;

    if (!av) {
        avtab_datum_t av_new;
        av_new.data = (1U << (perm->s.value - 1));
        if (avtab_insert(&pdb->te_avtab, &key, &av_new) != 0) {
            LOGE("Failed to add rule to avtab");
            return false;
        }
    } else {
        if (av->data & (1U << (perm->s.value - 1))) {
            exists = true;
        }
        av->data |= (1U << (perm->s.value - 1));
    }

    if (exists) {
        LOGD("Rule already exists: \"allow %s %s:%s %s;\"",
             source_str.c_str(), target_str.c_str(), class_str.c_str(),
             perm_str.c_str());
    } else {
        LOGD("Added rule: \"allow %s %s:%s %s;\"",
             source_str.c_str(), target_str.c_str(), class_str.c_str(),
             perm_str.c_str());
    }

    return true;
}

bool selinux_get_context(const std::string &path, std::string *context)
{
    ssize_t size;
    std::vector<char> value;

    size = getxattr(path.c_str(), SELINUX_XATTR, nullptr, 0);
    if (size < 0) {
        return false;
    }

    value.resize(size);

    size = getxattr(path.c_str(), SELINUX_XATTR, value.data(), size);
    if (size < 0) {
        return false;
    }

    value.push_back('\0');
    *context = value.data();

    return true;
}

bool selinux_lget_context(const std::string &path, std::string *context)
{
    ssize_t size;
    std::vector<char> value;

    size = lgetxattr(path.c_str(), SELINUX_XATTR, nullptr, 0);
    if (size < 0) {
        return false;
    }

    value.resize(size);

    size = lgetxattr(path.c_str(), SELINUX_XATTR, value.data(), size);
    if (size < 0) {
        return false;
    }

    value.push_back('\0');
    *context = value.data();

    return true;
}

bool selinux_fget_context(int fd, std::string *context)
{
    ssize_t size;
    std::vector<char> value;

    size = fgetxattr(fd, SELINUX_XATTR, nullptr, 0);
    if (size < 0) {
        return false;
    }

    value.resize(size);

    size = fgetxattr(fd, SELINUX_XATTR, value.data(), size);
    if (size < 0) {
        return false;
    }

    value.push_back('\0');
    *context = value.data();

    return true;
}

bool selinux_set_context(const std::string &path, const std::string &context)
{
    return setxattr(path.c_str(), SELINUX_XATTR,
                    context.c_str(), context.size() + 1, 0) == 0;
}

bool selinux_lset_context(const std::string &path, const std::string &context)
{
    return lsetxattr(path.c_str(), SELINUX_XATTR,
                     context.c_str(), context.size() + 1, 0) == 0;
}

bool selinux_fset_context(int fd, const std::string &context)
{
    return fsetxattr(fd, SELINUX_XATTR,
                     context.c_str(), context.size() + 1, 0) == 0;
}

bool selinux_set_context_recursive(const std::string &path,
                                   const std::string &context)
{
    return RecursiveSetContext(path, context, true).run();
}

bool selinux_lset_context_recursive(const std::string &path,
                                    const std::string &context)
{
    return RecursiveSetContext(path, context, false).run();
}

bool selinux_get_enforcing(int *value)
{
    int fd = open(SELINUX_ENFORCE_FILE, O_RDONLY);
    if (fd < 0) {
        return false;
    }

    char buf[20];
    memset(buf, 0, sizeof(buf));
    int ret = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (ret < 0) {
        return false;
    }

    int enforce = 0;
    if (sscanf(buf, "%d", &enforce) != 1) {
        return false;
    }

    *value = enforce;

    return true;
}

bool selinux_set_enforcing(int value)
{
    int fd = open(SELINUX_ENFORCE_FILE, O_RDWR);
    if (fd < 0) {
        return false;
    }

    char buf[20];
    snprintf(buf, sizeof(buf), "%d", value);
    int ret = write(fd, buf, strlen(buf));
    close(fd);
    if (ret < 0) {
        return false;
    }

    return true;
}

}
}
