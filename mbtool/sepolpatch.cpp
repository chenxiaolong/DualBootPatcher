/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "sepolpatch.h"

#include <memory>
#include <cstdio>
#include <fcntl.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sepol/policydb/policydb.h>
#include <sepol/sepol.h>

#include "mblog/logging.h"
#include "mbutil/autoclose/file.h"
#include "mbutil/selinux.h"
#include "mbutil/string.h"

#include "multiboot.h"


namespace mb
{

// Types to make permissive
static const char *permissive_types[] = {
    "init",
    //"init_shell",
    "kernel",
    //"recovery",
    nullptr
};

static bool copy_avtab_rules(policydb_t *pdb,
                             const char *from_target_name,
                             const char *to_target_name)
{
    std::vector<std::pair<avtab_key_t, avtab_datum_t>> to_add;

    type_datum_t *from_target, *to_target;

    if (strcmp(from_target_name, to_target_name) == 0) {
        LOGW("Types %s and %s are equal. Not copying rules",
             from_target_name, to_target_name);
        return true;
    }

    from_target = (type_datum_t *) hashtab_search(
            pdb->p_types.table, (hashtab_key_t) from_target_name);
    if (!from_target) {
        LOGE("(From) Target type %s does not exist", from_target_name);
        return false;
    }

    to_target = (type_datum_t *) hashtab_search(
            pdb->p_types.table, (hashtab_key_t) to_target_name);
    if (!to_target) {
        LOGE("(To) Target type %s does not exist", to_target_name);
        return false;
    }

    // Gather rules to copy
    for (uint32_t i = 0; i < pdb->te_avtab.nslot; ++i) {
        for (avtab_ptr_t cur = pdb->te_avtab.htable[i]; cur; cur = cur->next) {
            if (!(cur->key.specified & AVTAB_ALLOWED)) {
                continue;
            }

            if (cur->key.target_type == from_target->s.value) {
                avtab_key_t copy = cur->key;
                copy.target_type = to_target->s.value;

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
                // This should absolutely never happen unless libsepol has a bug
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

    if (!hashtab_search(pdb->p_types.table, (hashtab_key_t) expected_type)) {
        LOGW("Type %s doesn't exist. Won't touch /data/media related rules",
             expected_type);
        return true;
    }

    std::string context;
    if (!util::selinux_lget_context(path, &context)) {
        LOGE("%s: Failed to get context: %s", path, strerror(errno));
        path = "/data/media";
        if (!util::selinux_lget_context(path, &context)) {
            LOGE("%s: Failed to set context: %s", path, strerror(errno));
            return false;
        }
    }

    std::vector<std::string> pieces = util::split(context, ":");
    if (pieces.size() < 3) {
        LOGE("%s: Malformed context string: %s", path, context.c_str());
        return false;
    }
    std::string type = pieces[2];

    LOGV("Copying %s rules to %s because of improper %s SELinux label",
         expected_type, type.c_str(), path);
    return copy_avtab_rules(pdb, expected_type, type.c_str());
}

static bool patch_sepolicy_internal(const std::string &source,
                                    const std::string &target)
{
    policydb_t pdb;

    if (policydb_init(&pdb) < 0) {
        LOGE("Failed to initialize policydb");
        return false;
    }

    if (!util::selinux_read_policy(source, &pdb)) {
        LOGE("Failed to read SELinux policy file: %s", source.c_str());
        policydb_destroy(&pdb);
        return false;
    }

    LOGD("Policy version: %u", pdb.policyvers);

    for (const char **iter = permissive_types; *iter; ++iter) {
        util::selinux_make_permissive(&pdb, *iter);
    }

    fix_data_media_rules(&pdb);

    if (!util::selinux_write_policy(target, &pdb)) {
        LOGE("Failed to write SELinux policy file: %s", target.c_str());
        policydb_destroy(&pdb);
        return false;
    }

    policydb_destroy(&pdb);

    return true;
}

bool patch_sepolicy(const std::string &source,
                    const std::string &target)
{
    return patch_sepolicy_internal(source, target);
}

bool patch_loaded_sepolicy()
{
    int is_enforcing = 0;

    autoclose::file fp(autoclose::fopen(SELINUX_ENFORCE_FILE, "rbe"));
    if (!fp) {
        if (errno == ENOENT) {
            // If the file doesn't exist, then the kernel probably doesn't
            // support SELinux
            LOGV("Kernel does not support SELinux. Policy won't be patched");
            return true;
        } else {
            LOGE("Failed to open %s: %s", SELINUX_ENFORCE_FILE, strerror(errno));
            return false;
        }
    }

    if (std::fscanf(fp.get(), "%d", &is_enforcing) != 1) {
        LOGE("Failed to parse %s: %s", SELINUX_ENFORCE_FILE, strerror(errno));
        return false;
    }

    if (!is_enforcing) {
        LOGV("SELinux is globally set to permissive. Policy won't be patched");
        return true;
    }

    return patch_sepolicy_internal(SELINUX_POLICY_FILE, SELINUX_LOAD_FILE);
}

static void sepolpatch_usage(int error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: sepolpatch [OPTION]...\n\n"
            "Options:\n"
            "  -l, --loaded  Patch currently loaded policy\n"
            "  -s [SOURCE], --source [SOURCE]\n"
            "                Source policy file to patch\n"
            "  -t [TARGET], --target [TARGET]\n"
            "                Target policy file to patch\n"
            "  -h, --help    Display this help message\n"
            "\n"
            "This tool patches an SELinux binary policy file to make the \"init\",\n"
            "\"init_shell\", and \"recovery\" contexts permissive. This is necessary\n"
            "for the mount_fstab tool to create generated fstab files on \"user\"\n"
            "Android 5.0 ROMs, where there is no way to set SELinux to be globally\n"
            "permissive.\n\n"
            "If --source is omitted, the source path is set to /sys/fs/selinux/policy.\n"
            "If --target is omitted, the target path is set to /sys/fs/selinux/load.\n"
            "Note that, unlike --loaded, sepolpatch will not check if SELinux is\n"
            "supported, enabled, and enforcing before patching.\n\n"
            "Note: The source and target file can be set to the same path to patch\n"
            "the policy file in place.\n");
}

int sepolpatch_main(int argc, char *argv[])
{
    int opt;
    int loaded_flag = 0;
    const char *source_file = nullptr;
    const char *target_file = nullptr;

    static struct option long_options[] = {
        {"loaded", no_argument,       0, 'l'},
        {"source", required_argument, 0, 's'},
        {"target", required_argument, 0, 't'},
        {"help",   no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "ls:t:h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'l':
            loaded_flag = 1;
            break;

        case 's':
            source_file = optarg;
            break;

        case 't':
            target_file = optarg;
            break;

        case 'h':
            sepolpatch_usage(0);
            return EXIT_SUCCESS;

        default:
            sepolpatch_usage(1);
            return EXIT_FAILURE;
        }
    }

    // There should be no other arguments
    if (argc - optind != 0) {
        sepolpatch_usage(1);
        return EXIT_FAILURE;
    }

    if (!loaded_flag && !source_file && !target_file) {
        sepolpatch_usage(1);
        return EXIT_FAILURE;
    }

    if (loaded_flag) {
        if (source_file || target_file) {
            fprintf(stderr, "'--source' and '--target' cannot be used with '--loaded'\n");
            return EXIT_FAILURE;
        }

        return patch_loaded_sepolicy() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    if (!source_file) {
        source_file = SELINUX_POLICY_FILE;
    }
    if (!target_file) {
        target_file = SELINUX_LOAD_FILE;
    }

    return patch_sepolicy(source_file, target_file) ? EXIT_SUCCESS : EXIT_FAILURE;
}

}
