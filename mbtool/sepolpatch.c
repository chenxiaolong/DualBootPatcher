/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#include <fcntl.h>
#include <getopt.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sepol/policydb/policydb.h>
#include <sepol/sepol.h>

#include "util/logging.h"


#define SELINUX_ENFORCE_FILE "/sys/fs/selinux/enforce"
#define SELINUX_POLICY_FILE "/sys/fs/selinux/policy"
#define SELINUX_LOAD_FILE "/sys/fs/selinux/load"


// Types to make permissive
static char *permissive_types[] = {
    "init",
    "init_shell",
    "recovery",
    NULL
};

static int read_selinux_policy(const char *path, policydb_t *pdb)
{
    struct policy_file pf;
    struct stat sb;
    void *map;
    int fd;
    int ret;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOGE("Failed to open %s: %s", path, strerror(errno));
        return -1;
    }

    if (fstat(fd, &sb) < 0) {
        LOGE("Failed to stat %s: %s", path, strerror(errno));
        close(fd);
        return -1;
    }

    map = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        LOGE("Failed to mmap %s: %s", path, strerror(errno));
        close(fd);
        return -1;
    }

    policy_file_init(&pf);
    pf.type = PF_USE_MEMORY;
    pf.data = map;
    pf.len = sb.st_size;

    ret = policydb_read(pdb, &pf, 0);

    sepol_handle_destroy(pf.handle);
    munmap(map, sb.st_size);
    close(fd);

    return ret;
}

// /sys/fs/selinux/load requires the entire policy to be written in a single
// write(2) call.
// See: http://marc.info/?l=selinux&m=141882521027239&w=2
static int write_selinux_policy(const char *path, policydb_t *pdb)
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
        return -1;
    }

    fd = open(path, O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (fd < 0) {
        LOGE("Failed to open %s: %s", path, strerror(errno));
        sepol_handle_destroy(handle);
        free(data);
        return -1;
    }

    if (write(fd, data, len) < 0) {
        LOGE("Failed to write to %s: %s", path, strerror(errno));
        sepol_handle_destroy(handle);
        close(fd);
        free(data);
        return -1;
    }

    sepol_handle_destroy(handle);
    close(fd);
    free(data);

    return 0;
}

static void make_permissive(policydb_t *pdb, char **types)
{
    type_datum_t *type;
    char **iter;

    for (iter = types; *iter; iter++) {
        type = hashtab_search(pdb->p_types.table, *iter);
        if (!type) {
            LOGV("Type %s not found in policy", *iter);
            continue;
        }

        if (ebitmap_get_bit(&pdb->permissive_map, type->s.value)) {
            LOGV("Type %s is already permissive", *iter);
            continue;
        }

        if (ebitmap_set_bit(&pdb->permissive_map, type->s.value, 1) < 0) {
            LOGE("Failed to set bit for type %s in the permissive map", *iter);
            continue;
        }

        LOGD("Type %s is now permissive", *iter);
    }

    return;
}

static int patch_sepolicy_internal(const char *source, const char *target)
{
    policydb_t pdb;

    if (policydb_init(&pdb) < 0) {
        LOGE("Failed to initialize policydb");
        return -1;
    }

    if (read_selinux_policy(source, &pdb) < 0) {
        LOGE("Failed to read SELinux policy file: %s", source);
        policydb_destroy(&pdb);
        return -1;
    }

    LOGD("Policy version: %lu", pdb.policyvers);

    make_permissive(&pdb, permissive_types);

    if (write_selinux_policy(target, &pdb) < 0) {
        LOGE("Failed to write SELinux policy file: %s", target);
        policydb_destroy(&pdb);
        return -1;
    }

    policydb_destroy(&pdb);

    return 0;
}

int patch_sepolicy(const char *source, const char *target)
{
    return patch_sepolicy_internal(source, target);
}

int patch_loaded_sepolicy()
{
    FILE *fp;
    int is_enforcing = 0;

    fp = fopen(SELINUX_ENFORCE_FILE, "rb");
    if (!fp) {
        if (errno == ENOENT) {
            // If the file doesn't exist, then the kernel probably doesn't
            // support SELinux
            LOGV("Kernel does not support SELinux. Policy won't be patched");
            return 0;
        } else {
            LOGE("Failed to open %s: %s", SELINUX_ENFORCE_FILE, strerror(errno));
            return -1;
        }
    }

    if (fscanf(fp, "%u", &is_enforcing) != 1) {
        LOGE("Failed to parse %s: %s", SELINUX_ENFORCE_FILE, strerror(errno));
        fclose(fp);
        return -1;
    }

    fclose(fp);

    if (!is_enforcing) {
        LOGV("SELinux is globally set to permissive. Policy won't be patched");
        return 0;
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
    char *source_file = NULL;
    char *target_file = NULL;

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

    return patch_sepolicy(source_file, target_file) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
