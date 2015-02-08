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

#include "mount_fstab.h"

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>

#include "roms.h"
#include "sepolpatch.h"
#include "util/cmdline.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/fstab.h"
#include "util/logging.h"
#include "util/loopdev.h"
#include "util/mount.h"
#include "util/properties.h"


#define FORCE_SELINUX_PERMISSIVE 0


static int create_dir_and_mount(struct fstab_rec *rec,
                                struct fstab_rec *flags_from_rec,
                                char *mount_point);

static int create_dir_and_mount(struct fstab_rec *rec,
                                struct fstab_rec *flags_from_rec,
                                char *mount_point)
{
    struct stat st;
    int ret;
    mode_t perms;

    if (!rec) {
        return -1;
    }

    if (stat(rec->mount_point, &st) == 0) {
        perms = st.st_mode & 0xfff;
    } else {
        LOGW("%s found in fstab, but %s does not exist",
             rec->mount_point, rec->mount_point);
        perms = 0771;
    }

    if (stat(mount_point, &st) == 0) {
        if (chmod(mount_point, perms) < 0) {
            LOGE("Failed to chmod %s: %s", mount_point, strerror(errno));
            return -1;
        }
    } else {
        if (mkdir(mount_point, perms) < 0) {
            LOGE("Failed to create %s: %s", mount_point, strerror(errno));
            return -1;
        }
    }

    ret = mount(rec->blk_device,
                mount_point,
                rec->fs_type,
                flags_from_rec->flags,
                flags_from_rec->fs_options);
    if (ret < 0) {
        LOGE("Failed to mount %s at %s: %s",
             rec->blk_device, mount_point, strerror(errno));
        return -1;
    }

    return 0;
}

static int starts_with(const char *string, const char *prefix)
{
    return strlen(prefix) > strlen(string) ? 0
            : strncmp(string, prefix, strlen(prefix)) == 0;
}

static char * file_getprop(const char *path, const char *key)
{
    FILE *fp = NULL;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(path, "rb");
    if (!fp) {
        goto error;
    }

    while ((read = getline(&line, &len, fp)) >= 0) {
        if (line[0] == '\0' || line[0] == '#') {
            // Skip empty and comment lines
            continue;
        }

        char *equals = strchr(line, '=');
        if (!equals) {
            // No equals in line
            continue;
        }

        if ((size_t)(equals - line) != strlen(key)) {
            // Key is not the same length
            continue;
        }

        if (strncmp(line, key, strlen(key)) == 0) {
            // Strip newline
            if (line[read - 1] == '\n') {
                line[read - 1] = '\0';
                --read;
            }

            char *ret = strdup(equals + 1);
            fclose(fp);
            free(line);
            return ret;
        }
    }

error:
    if (fp) {
        fclose(fp);
    }

    free(line);
    return NULL;
}

static unsigned long get_api_version(void)
{
    char *api_str = file_getprop("/system/build.prop",
                                 "ro.build.version.sdk");
    char *temp;
    unsigned long api = strtoul(api_str, &temp, 0);
    free(api_str);
    if (*temp == '\0') {
        return api;
    } else {
        return 0;
    }
}

int mount_fstab(const char *fstab_path)
{
    static const char *fmt_fstab_gen = "%s/.%s.gen";
    static const char *fmt_completed = "%s/.%s.completed";
    static const char *fmt_failed = "%s/.%s.failed";

    struct fstab *fstab = NULL;
    FILE *out = NULL;
    struct fstab_rec *rec_system = NULL;
    struct fstab_rec *rec_cache = NULL;
    struct fstab_rec *rec_data = NULL;
    struct fstab_rec *flags_system = NULL;
    struct fstab_rec *flags_cache = NULL;
    struct fstab_rec *flags_data = NULL;
    char copy1[256];
    char copy2[256];
    char path_fstab_gen[256];
    char path_completed[256];
    char path_failed[256];
    char *base_name;
    char *dir_name;
    struct stat st;
    int share_app = 0;
    int share_app_asec = 0;

    struct roms r;
    mb_roms_init(&r);
    mb_roms_get_builtin(&r);

    // basename() and dirname() modify the source string
    strlcpy(copy1, fstab_path, sizeof(copy1));
    strlcpy(copy2, fstab_path, sizeof(copy2));
    base_name = basename(fstab_path);
    dir_name = dirname(fstab_path);

    snprintf(path_fstab_gen, sizeof(path_fstab_gen), fmt_fstab_gen, dir_name, base_name);
    snprintf(path_completed, sizeof(path_completed), fmt_completed, dir_name, base_name);
    snprintf(path_failed, sizeof(path_failed), fmt_failed, dir_name, base_name);

    // This is a oneshot operation
    if (stat(path_completed, &st) == 0) {
        LOGV("Filesystems already successfully mounted");
        return 0;
    }

    if (stat(path_failed, &st) == 0) {
        LOGE("Failed to mount partitions ealier. No further attempts will be made");
        return -1;
    }

    // Remount rootfs as read-write so a new fstab file can be written
    if (mount("", "/", "", MS_REMOUNT, "") < 0) {
        LOGE("Failed to remount rootfs as rw: %s", strerror(errno));
    }

    // Read original fstab
    fstab = mb_read_fstab(fstab_path);
    if (!fstab) {
        LOGE("Failed to read %s", fstab_path);
        goto error;
    }

    // Generate new fstab without /system, /cache, or /data entries
    out = fopen(path_fstab_gen, "wb");
    if (!out) {
        LOGE("Failed to open %s for writing: %s", path_fstab_gen, strerror(errno));
        goto error;
    }

    if (fstab != NULL) {
        for (int i = 0; i < fstab->num_entries; ++i) {
            struct fstab_rec *rec = &fstab->recs[i];

            if (strcmp(rec->mount_point, "/system") == 0) {
                rec_system = rec;
            } else if (strcmp(rec->mount_point, "/cache") == 0) {
                rec_cache = rec;
            } else if (strcmp(rec->mount_point, "/data") == 0) {
                rec_data = rec;
            } else {
                fprintf(out, "%s\n", rec->orig_line);
            }
        }
    }

    fclose(out);

    // /system and /data are always in the fstab. The patcher should create
    // an entry for /cache for the ROMs that mount it manually in one of the
    // init scripts
    if (!rec_system || !rec_cache || !rec_data) {
        LOGE("fstab does not contain all of /system, /cache, and /data!");
        goto error;
    }

    // Mount raw partitions to /raw-*
    char *rom_id;
    if (mb_kernel_cmdline_get_option("romid", &rom_id) < 0) {
        LOGE("Failed to determine ROM ID");
        goto error;
    }

    struct rom *rom = mb_find_rom_by_id(&r, rom_id);
    if (!rom) {
        LOGE("Unknown ROM ID: %s", rom_id);
        free(rom_id);
        goto error;
    }

    // Set property for the Android app to use
    if (mb_set_property("ro.multiboot.romid", rom_id) < 0) {
        LOGE("Failed to set 'ro.multiboot.romid' to '%s'", rom_id);
    }

    free(rom_id);

    // Because of how Android deals with partitions, if, say, the source path
    // for the /system bind mount resides on /cache, then the cache partition
    // must be mounted with the system partition's flags. In this future, this
    // may be avoided by mounting every partition with some more liberal flags,
    // since the current setup does not allow two bind mounted locations to
    // reside on the same partition.

    if (starts_with(rom->system_path, "/cache")) {
        flags_system = rec_cache;
    } else {
        flags_system = rec_system;
    }

    if (starts_with(rom->cache_path, "/system")) {
        flags_cache = rec_system;
    } else {
        flags_cache = rec_cache;
    }

    flags_data = rec_data;

    if (mkdir("/raw", 0755) < 0) {
        LOGE("Failed to create /raw");
        goto error;
    }

    if (create_dir_and_mount(rec_system, flags_system, "/raw/system") < 0) {
        LOGE("Failed to mount /raw/system");
        goto error;
    }
    if (create_dir_and_mount(rec_cache, flags_cache, "/raw/cache") < 0) {
        LOGE("Failed to mount /raw/cache");
        goto error;
    }
    if (create_dir_and_mount(rec_data, flags_data, "/raw/data") < 0) {
        LOGE("Failed to mount /raw/data");
        goto error;
    }

    // Make paths use /raw-...
    if (!rom->system_path || !rom->system_path[0]
            || !rom->cache_path || !rom->cache_path[0]
            || !rom->data_path || !rom->data_path[0]) {
        LOGE("Invalid or null paths");
        goto error;
    }

    char target_system[100] = "/raw-";
    char target_cache[100] = "/raw-";
    char target_data[100] = "/raw-";
    strlcat(target_system, rom->system_path + 1, sizeof(target_system));
    strlcat(target_cache, rom->cache_path + 1, sizeof(target_cache));
    strlcat(target_data, rom->data_path + 1, sizeof(target_data));

    if (mb_bind_mount(target_system, 0771, "/system", 0771) < 0) {
        goto error;
    }

    if (mb_bind_mount(target_cache, 0771, "/cache", 0771) < 0) {
        goto error;
    }

    if (mb_bind_mount(target_data, 0771, "/data", 0771) < 0) {
        goto error;
    }

    // Bind mount internal SD directory
    if (mb_bind_mount("/raw/data/media", 0771, "/data/media", 0771)) {
        goto error;
    }

    // Prevent installd from dying because it can't unmount /data/media for
    // multi-user migration. Since <= 4.2 devices aren't supported anyway,
    // we'll bypass this.
    FILE* lv = fopen("/data/.layout_version", "wb");
    if (lv) {
        const char *layout_version;
        if (get_api_version() >= 21) {
            layout_version = "3";
        } else {
            layout_version = "2";
        }

        fwrite(layout_version, 1, strlen(layout_version), lv);
        fclose(lv);
    } else {
        LOGE("Failed to open /data/.layout_version to disable migration");
    }

    static const char *context = "u:object_r:install_data_file:s0";
    if (lsetxattr("/data/.layout_version", "security.selinux",
                  context, strlen(context) + 1, 0) < 0) {
        LOGE("%s: Failed to set SELinux context: %s",
             "/data/.layout_version", strerror(errno));
    }


    // Set version property
    if (mb_set_property("ro.multiboot.version", MBP_VERSION) < 0) {
        LOGE("Failed to set 'ro.multiboot.version' to '%s'", MBP_VERSION);
    }


    // Global app sharing
    share_app = stat("/data/patcher.share-app", &st) == 0;
    share_app_asec = stat("/data/patcher.share-app-asec", &st) == 0;

    if (share_app || share_app_asec) {
        if (mb_bind_mount("/raw/data/app-lib", 0771,
                          "/data/app-lib", 0771) < 0) {
            goto error;
        }
    }

    if (share_app) {
        if (mb_bind_mount("/raw/data/app", 0771,
                          "/data/app", 0771) < 0) {
            goto error;
        }
    }

    if (share_app_asec) {
        if (mb_bind_mount("/raw/data/app-asec", 0771,
                          "/data/app-asec", 0771) < 0) {
            goto error;
        }
    }

    mb_free_fstab(fstab);

    mb_roms_cleanup(&r);

    mb_create_empty_file(path_completed);

    LOGI("Successfully mounted partitions");

    return 0;

error:
    if (fstab) {
        mb_free_fstab(fstab);
    }

    mb_roms_cleanup(&r);

    mb_create_empty_file(path_failed);

    return -1;
}

static void mount_fstab_usage(int error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: mount_fstab [fstab file]\n\n"
            "This takes only one argument, the path to the fstab file. If the\n"
            "mounting succeeds, the generated \"/.<orig fstab>.gen\" file should\n"
            "be passed to the Android init's mount_all command.\n");
}

int mount_fstab_main(int argc, char *argv[])
{
    int opt;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            mount_fstab_usage(0);
            return EXIT_SUCCESS;

        default:
            mount_fstab_usage(1);
            return EXIT_FAILURE;
        }
    }

    // We only expect one argument
    if (argc - optind != 1) {
        mount_fstab_usage(1);
        return EXIT_FAILURE;
    }

    // Use the kernel log since logcat hasn't run yet
    mb_klog_init();
    mb_log_use_kernel_output();

    // Patch SELinux policy
    if (patch_loaded_sepolicy() < 0) {
        LOGE("Failed to patch loaded SELinux policy. Continuing anyway");
    } else {
        LOGV("SELinux policy patching completed. Waiting 1 second for policy reload");
        sleep(1);
    }

#if FORCE_SELINUX_PERMISSIVE
    int fd = open("/sys/fs/selinux/enforce", O_RDWR);
    if (fd > 0) {
        write(fd, "0", 1);
        close(fd);
    }
#endif

    return mount_fstab(argv[optind]) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
