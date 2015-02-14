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

#include "util/logging.h"
#include "util/selinux.h"


namespace mb
{

typedef std::unique_ptr<std::FILE, int (*)(std::FILE *)> file_ptr;

// Types to make permissive
static const char *permissive_types[] = {
    "init",
    "init_shell",
    "recovery",
    nullptr
};

static bool patch_sepolicy_internal(const std::string &source,
                                    const std::string &target)
{
    policydb_t pdb;

    if (policydb_init(&pdb) < 0) {
        LOGE("Failed to initialize policydb");
        return false;
    }

    if (!util::selinux_read_policy(source, &pdb)) {
        LOGE("Failed to read SELinux policy file: {}", source);
        policydb_destroy(&pdb);
        return false;
    }

    LOGD("Policy version: {}", pdb.policyvers);

    for (const char **iter = permissive_types; *iter; ++iter) {
        util::selinux_make_permissive(&pdb, *iter);
    }

    if (!util::selinux_write_policy(target, &pdb)) {
        LOGE("Failed to write SELinux policy file: {}", target);
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

    file_ptr fp(std::fopen(MB_SELINUX_ENFORCE_FILE, "rb"), std::fclose);
    if (!fp) {
        if (errno == ENOENT) {
            // If the file doesn't exist, then the kernel probably doesn't
            // support SELinux
            LOGV("Kernel does not support SELinux. Policy won't be patched");
            return true;
        } else {
            LOGE("Failed to open {}: {}", MB_SELINUX_ENFORCE_FILE, strerror(errno));
            return false;
        }
    }

    if (std::fscanf(fp.get(), "%d", &is_enforcing) != 1) {
        LOGE("Failed to parse {}: {}", MB_SELINUX_ENFORCE_FILE, strerror(errno));
        return false;
    }

    if (!is_enforcing) {
        LOGV("SELinux is globally set to permissive. Policy won't be patched");
        return true;
    }

    return patch_sepolicy_internal(MB_SELINUX_POLICY_FILE, MB_SELINUX_LOAD_FILE);
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
        source_file = MB_SELINUX_POLICY_FILE;
    }
    if (!target_file) {
        target_file = MB_SELINUX_LOAD_FILE;
    }

    return patch_sepolicy(source_file, target_file) ? EXIT_SUCCESS : EXIT_FAILURE;
}

}