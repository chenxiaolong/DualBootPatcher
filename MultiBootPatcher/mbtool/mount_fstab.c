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

#include "mount_fstab.h"

#ifndef __ANDROID__
#include <bsd/string.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "config.h"
#include "logging.h"


struct fstab
{
    int num_entries;
    struct fstab_rec *recs;
    char *fstab_filename;
};

struct fstab_rec
{
    char *blk_device;
    char *mount_point;
    char *fs_type;
    unsigned long flags;
    char *fs_options;
    char *vold_args;
    char *orig_line;
};

struct mount_flag
{
    const char *name;
    unsigned int flag;
};

static struct mount_flag mount_flags[] =
{
    { "active",         MS_ACTIVE },
    { "bind",           MS_BIND },
    { "dirsync",        MS_DIRSYNC },
    { "mandlock",       MS_MANDLOCK },
    { "move",           MS_MOVE },
    { "noatime",        MS_NOATIME },
    { "nodev",          MS_NODEV },
    { "nodiratime",     MS_NODIRATIME },
    { "noexec",         MS_NOEXEC },
    { "nosuid",         MS_NOSUID },
    { "nouser",         MS_NOUSER },
    { "posixacl",       MS_POSIXACL },
    { "rec",            MS_REC },
    { "ro",             MS_RDONLY },
    { "relatime",       MS_RELATIME },
    { "remount",        MS_REMOUNT },
    { "silent",         MS_SILENT },
    { "strictatime",    MS_STRICTATIME },
    { "sync",           MS_SYNCHRONOUS },
    { "unbindable",     MS_UNBINDABLE },
    { "private",        MS_PRIVATE },
    { "slave",          MS_SLAVE },
    { "shared",         MS_SHARED },
    // Flags that should be ignored
    { "ro",             0 },
    { "defaults",       0 },
    { NULL,             0 }
};


static struct fstab * read_fstab(const char *path);
static void free_fstab(struct fstab *fstab);
static int options_to_flags(char *args, char *new_args, int size);
static int create_dir_and_mount(struct fstab_rec *rec,
                                struct fstab_rec *flags_from_rec,
                                char *mount_point);
static int mkdirs(const char *dir, mode_t mode);
static int bind_mount(const char *source, const char *target);
static struct partconfig * find_partconfig();


// Much simplified version of fs_mgr's fstab parsing code
static struct fstab * read_fstab(const char *path)
{
    FILE *fp;
    int count, entries;
    char *line = NULL;
    size_t len = 0; // allocated memory size
    ssize_t bytes_read; // number of bytes read
    char *temp;
    char *save_ptr;
    const char *delim = " \t";
    struct fstab *fstab = NULL;
    char temp_mount_args[1024];

    fp = fopen(path, "rb");
    if (!fp) {
        LOGE("Failed to open file %s", path);
        return NULL;
    }

    entries = 0;
    while ((bytes_read = getline(&line, &len, fp)) != -1) {
        // Strip newlines
        if (bytes_read > 0 && line[bytes_read - 1] == '\n') {
            line[bytes_read - 1] = '\0';
        }

        // Strip leading
        temp = line;
        while (isspace(*temp)) {
            ++temp;
        }

        // Skip empty lines and comments
        if (*temp == '\0' || *temp == '#') {
            continue;
        }

        ++entries;
    }

    if (entries == 0) {
        LOGE("fstab contains no entries");
        goto error;
    }

    fstab = calloc(1, sizeof(struct fstab));
    fstab->num_entries = entries;
    fstab->fstab_filename = strdup(path);
    fstab->recs = calloc(fstab->num_entries, sizeof(struct fstab_rec));

    fseek(fp, 0, SEEK_SET);

    count = 0;
    while ((bytes_read = getline(&line, &len, fp)) != -1) {
        // Strip newlines
        if (bytes_read > 0 && line[bytes_read - 1] == '\n') {
            line[bytes_read - 1] = '\0';
        }

        // Strip leading
        temp = line;
        while (isspace(*temp)) {
            ++temp;
        }

        // Skip empty lines and comments
        if (*temp == '\0' || *temp == '#') {
            continue;
        }

        // Avoid possible overflow if the file was changed
        if (count >= entries) {
            LOGE("Found more fstab entries on second read than first read");
            break;
        }

        struct fstab_rec *rec = &fstab->recs[count];

        rec->orig_line = strdup(line);

        if ((temp = strtok_r(line, delim, &save_ptr)) == NULL) {
            LOGE("No source path/device found in entry: %s", line);
            goto error;
        }
        rec->blk_device = strdup(temp);

        if ((temp = strtok_r(NULL, delim, &save_ptr)) == NULL) {
            LOGE("No mount point found in entry: %s", line);
            goto error;
        }
        rec->mount_point = strdup(temp);

        if ((temp = strtok_r(NULL, delim, &save_ptr)) == NULL) {
            LOGE("No filesystem type found in entry: %s", line);
            goto error;
        }
        rec->fs_type = strdup(temp);

        if ((temp = strtok_r(NULL, delim, &save_ptr)) == NULL) {
            LOGE("No mount options found in entry: %s", line);
            goto error;
        }
        rec->flags = options_to_flags(temp, temp_mount_args, 1024);

        if (temp_mount_args[0]) {
            rec->fs_options = strdup(temp_mount_args);
        } else {
            rec->fs_options = NULL;
        }

        if ((temp = strtok_r(NULL, delim, &save_ptr)) == NULL) {
            LOGE("No fs_mgr/vold options found in entry: %s", line);
            goto error;
        }
        rec->vold_args = strdup(temp);

        ++count;
    }

    fclose(fp);
    free(line);
    return fstab;

error:
    fclose(fp);
    free(line);
    if (fstab) {
        free_fstab(fstab);
    }
    return NULL;
}

static void free_fstab(struct fstab *fstab)
{
    if (!fstab) {
        return;
    }

    for (int i = 0; i < fstab->num_entries; ++i) {
        struct fstab_rec *rec = &fstab->recs[i];

        if (rec->blk_device) {
            free(rec->blk_device);
        }
        if (rec->mount_point) {
            free(rec->mount_point);
        }
        if (rec->fs_type) {
            free(rec->fs_type);
        }
        if (rec->fs_options) {
            free(rec->fs_options);
        }
        if (rec->vold_args) {
            free(rec->vold_args);
        }
        if (rec->orig_line) {
            free(rec->orig_line);
        }
    }

    free(fstab->recs);
    free(fstab->fstab_filename);
    free(fstab);
}

static int options_to_flags(char *args, char *new_args, int size)
{
    char *temp;
    char *save_ptr;
    int flags = 0;
    int i;

    if (new_args && size > 0) {
        new_args[0] = '\0';
    }

    temp = strtok_r(args, ",", &save_ptr);
    while (temp) {
        for (i = 0; mount_flags[i].name; ++i) {
            if (strcmp(temp, mount_flags[i].name) == 0) {
                flags |= 0;
                break;
            }
        }

        if (!mount_flags[i].name) {
            if (new_args) {
                strlcat(new_args, temp, size);
                strlcat(new_args, ",", size);
            } else {
                LOGW("Only universal mount options expected, but found %s", temp);
            }
        }

        temp = strtok_r(NULL, ",", &save_ptr);
    }

    if (new_args && new_args[0]) {
        new_args[strlen(new_args) - 1] = '\0';
    }

    return flags;
}

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

static int mkdirs(const char *dir, mode_t mode)
{
    struct stat st;
    char *p;
    char *save_ptr;
    char *temp;
    char *copy;
    int len;

    if (!dir || strlen(dir) == 0) {
        return -1;
    }

    copy = strdup(dir);
    len = strlen(dir);
    temp = malloc(len + 2);
    temp[0] = '\0';

    if (dir[0] == '/') {
        strcat(temp, "/");
    }

    p = strtok_r(copy, "/", &save_ptr);
    while (p != NULL) {
        strcat(temp, p);
        strcat(temp, "/");

        printf("Creating %s\n", temp);
        if (stat(temp, &st) < 0 && mkdir(temp, mode) < 0) {
            LOGE("Failed to create directory %s: %s\n", temp, strerror(errno));
            free(copy);
            free(temp);
            return -1;
        }

        p = strtok_r(NULL, "/", &save_ptr);
    }

    free(copy);
    free(temp);
    return 0;
}

static int bind_mount(const char *source, const char *target)
{
    if (mkdirs(source, 0771)) {
        LOGE("Failed to create %s", source);
        return -1;
    }

    if (mkdirs(target, 0771) < 0) {
        LOGE("Failed to create %s", target);
        return -1;
    }

    if (mount(source, target, NULL, MS_BIND, NULL) < 0) {
        LOGE("Failed to bind mount %s to %s: %s",
             source, target, strerror(errno));
        return -1;
    }

    return 0;
}

static struct partconfig * find_partconfig()
{
    struct mainconfig *config = get_mainconfig();

    for (int i = 0; i < config->partconfigs_len; ++i) {
       if (strcmp(config->installed, config->partconfigs[i].id) == 0) {
           return &config->partconfigs[i];
       }
    }

    return NULL;
}

int mount_fstab(const char *fstab_path)
{
    struct fstab *fstab = NULL;
    FILE *out = NULL;
    struct fstab_rec *rec_system = NULL;
    struct fstab_rec *rec_cache = NULL;
    struct fstab_rec *rec_data = NULL;
    struct fstab_rec *flags_system = NULL;
    struct fstab_rec *flags_cache = NULL;
    struct fstab_rec *flags_data = NULL;
    struct stat st;
    int share_app = 0;
    int share_app_asec = 0;

    // Read original fstab
    fstab = read_fstab(fstab_path);
    if (!fstab) {
        LOGE("Failed to read %s", fstab_path);
        goto error;
    }

    // Generate new fstab without /system, /cache, or /data entries
    out = fopen("fstab.gen", "wb");
    if (!out) {
        LOGE("Failed to open fstab.gen for writing");
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
    struct partconfig *partconfig = find_partconfig();
    if (!partconfig) {
        LOGE("Could not determine partition configuration");
        goto error;
    }

    // Because of how Android deals with partitions, if, say, the source path
    // for the /system bind mount resides on /cache, then the cache partition
    // must be mounted with the system partition's flags. In this future, this
    // may be avoided by mounting every partition with some more liberal flags,
    // since the current setup does not allow two bind mounted locations to
    // reside on the same partition.

    if (strcmp(partconfig->target_system_partition, "/cache") == 0) {
        flags_system = rec_cache;
    } else {
        flags_system = rec_system;
    }

    if (strcmp(partconfig->target_cache_partition, "/system") == 0) {
        flags_cache = rec_system;
    } else {
        flags_cache = rec_cache;
    }

    flags_data = rec_data;

    if (create_dir_and_mount(rec_system, flags_system, "/raw-system") < 0) {
        LOGE("Failed to mount /raw-system");
        goto error;
    }
    if (create_dir_and_mount(rec_cache, flags_cache, "/raw-cache") < 0) {
        LOGE("Failed to mount /raw-cache");
        goto error;
    }
    if (create_dir_and_mount(rec_data, flags_data, "/raw-data") < 0) {
        LOGE("Failed to mount /raw-data");
        goto error;
    }

    // Bind mount directories to /system, /cache, and /data
    if (bind_mount(partconfig->target_system, "/system") < 0) {
        goto error;
    }

    if (bind_mount(partconfig->target_cache, "/cache") < 0) {
        goto error;
    }

    if (bind_mount(partconfig->target_data, "/data") < 0) {
        goto error;
    }

    // Bind mount internal SD directory
    if (bind_mount("/raw-data/media", "/data/media")) {
        goto error;
    }

    // Prevent installd from dying because it can't unmount /data/media for
    // multi-user migration. Since <= 4.2 devices aren't supported anyway,
    // we'll bypass this.
    FILE* lv = fopen("/data/.layout_version", "wb");
    if (lv) {
        fwrite("2", 1, 1, lv);
        fclose(lv);
    } else {
        LOGE("Failed to open /data/.layout_version to disable migration");
    }

    // Global app sharing
    share_app = stat("/data/patcher.share-app", &st) == 0;
    share_app_asec = stat("/data/patcher.share-app-asec", &st) == 0;

    if (share_app || share_app_asec) {
        if (bind_mount("/raw-data/app-lib", "/data/app-lib") < 0) {
            goto error;
        }
    }

    if (share_app) {
        if (bind_mount("/raw-data/app", "/data/app") < 0) {
            goto error;
        }
    }

    if (share_app_asec) {
        if (bind_mount("/raw-data/app-asec", "/data/app-asec") < 0) {
            goto error;
        }
    }

    // TODO: Hack to run additional script
    if (stat("/init.additional.sh", &st) == 0) {
        system("sh /init.additional.sh");
    }

    free_fstab(fstab);

    LOGI("Successfully mounted partitions");

    return 0;

error:
    if (fstab) {
        free_fstab(fstab);
    }

    return -1;
}

static void mount_fstab_usage(int error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: mount_fstab [fstab file]\n\n"
            "This takes only one argument, the path to the fstab file. If the\n"
            "mounting succeeds, the generated fstab.gen file should be passed\n"
            "to the Android init's mount_all command.\n");
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
    klog_init();
    use_kernel_log_output();

    // Read configuration file
    if (mainconfig_init() < 0) {
        LOGE("Failed to load main configuration file");
        return EXIT_FAILURE;
    }

    return mount_fstab(argv[1]) == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
