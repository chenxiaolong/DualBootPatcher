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

#include "update_binary.h"

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <fts.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <archive.h>
#include <archive_entry.h>

#include "main.h"
#include "multiboot.h"
#include "roms.h"
#include "external/sha.h"
#include "util/archive.h"
#include "util/command.h"
#include "util/copy.h"
#include "util/delete.h"
#include "util/directory.h"
#include "util/file.h"
#include "util/logging.h"
#include "util/loopdev.h"
#include "util/mount.h"
#include "util/properties.h"


// Set to 1 to spawn a shell after installation
// NOTE: This should ONLY be used through adb. For example:
//
//     $ adb push mbtool_recovery /tmp/updater
//     $ adb shell /tmp/updater 3 1 /path/to/file_patched.zip
#define DEBUG_SHELL 0


/* Lots of paths */

#define CHROOT          "/chroot"
#define MB_TEMP         "/multiboot"

#define HELPER_TOOL     "/update-binary-tool"

#define ZIP_UPDATER     "META-INF/com/google/android/update-binary"
#define ZIP_E2FSCK      "multiboot/e2fsck"
#define ZIP_RESIZE2FS   "multiboot/resize2fs"
#define ZIP_TUNE2FS     "multiboot/tune2fs"
#define ZIP_UNZIP       "multiboot/unzip"
#define ZIP_AROMA       "multiboot/aromawrapper.zip"
#define ZIP_BBWRAPPER   "multiboot/bb-wrapper.sh"
#define ZIP_DEVICE      "multiboot/device"
#define ZIP_MBP_MINI    "multiboot/libmbp-mini.so"


static const char *interface;
static const char *output_fd_str;
static const char *zip_file;

static int output_fd;


struct CBootImage;
typedef struct CBootImage CBootImage;

struct CCpioFile;
typedef struct CCpioFile CCpioFile;

struct CDevice;
typedef struct CDevice CDevice;

struct CPatcherError;
typedef struct CPatcherError CPatcherError;

struct CPatcherConfig;
typedef struct CPatcherConfig CPatcherConfig;

// dlopen()'ed from libmbp-mini.so
struct libmbp {
    void *handle;

    CBootImage *     (*mbp_bootimage_create)             (void);
    void             (*mbp_bootimage_destroy)            (CBootImage *bootImage);
    int              (*mbp_bootimage_load_file)          (CBootImage *bootImage,
                                                          const char *filename);
    void             (*mbp_bootimage_create_data)        (const CBootImage *bootImage,
                                                          void **data,
                                                          size_t *size);
    size_t           (*mbp_bootimage_ramdisk_image)      (const CBootImage *bootImage,
                                                          void **data);
    void             (*mbp_bootimage_set_ramdisk_image)  (CBootImage *bootImage,
                                                          const void *data,
                                                          size_t size);

    CCpioFile *      (*mbp_cpiofile_create)              (void);
    void             (*mbp_cpiofile_destroy)             (CCpioFile *cpio);
    int              (*mbp_cpiofile_load_data)           (CCpioFile *cpio,
                                                          const void *data,
                                                          size_t size);
    int              (*mbp_cpiofile_create_data)         (CCpioFile *cpio,
                                                          int gzip,
                                                          void **data,
                                                          size_t *size);
    int              (*mbp_cpiofile_add_file_from_data)  (CCpioFile *cpio,
                                                          const void *data,
                                                          size_t size,
                                                          const char *name,
                                                          unsigned int perms);

    CDevice *        (*mbp_device_create)                (void);
    void             (*mbp_device_destroy)               (CDevice *device);
    char *           (*mbp_device_codename)              (const CDevice *device);
    char **          (*mbp_device_boot_block_devs)       (const CDevice *device);

    CPatcherConfig * (*mbp_config_create)                (void);
    void             (*mbp_config_destroy)               (CPatcherConfig *pc);
    char *           (*mbp_config_version)               (const CPatcherConfig *pc);
    CDevice **       (*mbp_config_devices)               (const CPatcherConfig *pc);

    void             (*mbp_free)                         (void *data);
    void             (*mbp_free_array)                   (void **array);
};

#define DLSYM(mbp, func) \
    dlerror(); \
    mbp->func = dlsym(mbp->handle, #func); \
    if ((error = dlerror())) { \
        LOGE("dlsym() failed for '%s': %s", #func, error); \
        return -1; \
    }

static int libmbp_init(struct libmbp *mbp, const char *path)
{
    mbp->handle = dlopen(path, RTLD_LAZY);
    if (!mbp->handle) {
        LOGE("%s: Failed to dlopen: %s", path, dlerror());
        return -1;
    }

    // Android's dlerror() returns const char *
    const char *error;

    DLSYM(mbp, mbp_bootimage_create);
    DLSYM(mbp, mbp_bootimage_destroy);
    DLSYM(mbp, mbp_bootimage_load_file);
    DLSYM(mbp, mbp_bootimage_create_data);
    DLSYM(mbp, mbp_bootimage_ramdisk_image);
    DLSYM(mbp, mbp_bootimage_set_ramdisk_image);

    DLSYM(mbp, mbp_cpiofile_create);
    DLSYM(mbp, mbp_cpiofile_destroy);
    DLSYM(mbp, mbp_cpiofile_load_data);
    DLSYM(mbp, mbp_cpiofile_create_data);
    DLSYM(mbp, mbp_cpiofile_add_file_from_data);

    DLSYM(mbp, mbp_device_create);
    DLSYM(mbp, mbp_device_destroy);
    DLSYM(mbp, mbp_device_codename);
    DLSYM(mbp, mbp_device_boot_block_devs);

    DLSYM(mbp, mbp_config_create);
    DLSYM(mbp, mbp_config_destroy);
    DLSYM(mbp, mbp_config_version);
    DLSYM(mbp, mbp_config_devices);

    DLSYM(mbp, mbp_free);
    DLSYM(mbp, mbp_free_array);

    return 0;
}

static int libmbp_destroy(struct libmbp *mbp)
{
    if (mbp->handle) {
        if (dlclose(mbp->handle) < 0) {
            LOGE("Failed to dlclose(): %s", dlerror());
            return -1;
        }
    }

    return 0;
}


static void ui_print(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    char newfmt[100];
    snprintf(newfmt, 100, "ui_print [MultiBoot] %s\n", fmt);
    vdprintf(output_fd, newfmt, ap);
    dprintf(output_fd, "ui_print\n");

    va_end(ap);
}

#define MKDIR_CHECKED(a, b) \
    if (mkdir(a, b) < 0) { \
        LOGE("Failed to create %s: %s", a, strerror(errno)); \
        goto error; \
    }
#define MOUNT_CHECKED(a, b, c, d, e) \
    if (mount(a, b, c, d, e) < 0) { \
        LOGE("Failed to mount %s (%s) at %s: %s", a, c, b, strerror(errno)); \
        goto error; \
    }
#define MKNOD_CHECKED(a, b, c) \
    if (mknod(a, b, c) < 0) { \
        LOGE("Failed to create special file %s: %s", a, strerror(errno)); \
        goto error; \
    }
#define IS_MOUNTED_CHECKED(a) \
    if (!mb_is_mounted(a)) { \
        LOGE("%s is not mounted", a); \
        goto error; \
    }
#define UNMOUNT_ALL_CHECKED(a) \
    if (mb_unmount_all(a) < 0) { \
        LOGE("Failed to unmount all mountpoints within %s", a); \
        goto error; \
    }
#define RECURSIVE_DELETE_CHECKED(a) \
    if (mb_delete_recursive(a) < 0) { \
        LOGE("Failed to recursively remove %s", a); \
        goto error; \
    }
#define COPY_DIR_CONTENTS_CHECKED(a, b, c) \
    if (mb_copy_dir(a, b, c) < 0) { \
        LOGE("Failed to copy contents of %s/ to %s/", a, b); \
        goto error; \
    }

static int create_chroot(void)
{
    // We'll just call the recovery's mount tools directly to avoid having to
    // parse TWRP and CWM's different fstab formats
    char *mount_system[] = { "mount", "/system", NULL };
    char *mount_cache[] = { "mount", "/cache", NULL };
    char *mount_data[] = { "mount", "/data", NULL };
    mb_run_command(mount_system);
    mb_run_command(mount_cache);
    mb_run_command(mount_data);

    // Make sure everything really is mounted
    IS_MOUNTED_CHECKED("/system");
    IS_MOUNTED_CHECKED("/cache");
    IS_MOUNTED_CHECKED("/data");

    // Unmount everything previously mounted in /chroot
    UNMOUNT_ALL_CHECKED(CHROOT);

    // Remove /chroot if it exists
    RECURSIVE_DELETE_CHECKED(CHROOT);

    // Setup directories
    MKDIR_CHECKED(CHROOT, 0755);
    MKDIR_CHECKED(CHROOT MB_TEMP, 0755);
    MKDIR_CHECKED(CHROOT "/dev", 0755);
    MKDIR_CHECKED(CHROOT "/etc", 0755);
    MKDIR_CHECKED(CHROOT "/proc", 0755);
    MKDIR_CHECKED(CHROOT "/sbin", 0755);
    MKDIR_CHECKED(CHROOT "/sys", 0755);
    MKDIR_CHECKED(CHROOT "/tmp", 0755);

    MKDIR_CHECKED(CHROOT "/data", 0755);
    MKDIR_CHECKED(CHROOT "/cache", 0755);
    MKDIR_CHECKED(CHROOT "/system", 0755);

    // Other mounts
    MOUNT_CHECKED("none", CHROOT "/dev",            "tmpfs",     0, "");
    MKDIR_CHECKED(CHROOT "/dev/pts", 0755);
    MOUNT_CHECKED("none", CHROOT "/dev/pts",        "devpts",    0, "");
    MOUNT_CHECKED("none", CHROOT "/proc",           "proc",      0, "");
    MOUNT_CHECKED("none", CHROOT "/sys",            "sysfs",     0, "");
    MOUNT_CHECKED("none", CHROOT "/sys/fs/selinux", "selinuxfs", 0, "");
    MOUNT_CHECKED("none", CHROOT "/tmp",            "tmpfs",     0, "");
    // Copy the contents of sbin since we need to mess with some of the binaries
    // there. Also, for whatever reason, bind mounting /sbin results in EINVAL
    // no matter if it's done from here or from busybox.
    MOUNT_CHECKED("none", CHROOT "/sbin",           "tmpfs",     0, "");
    COPY_DIR_CONTENTS_CHECKED("/sbin", CHROOT "/sbin",
                              MB_COPY_ATTRIBUTES
                              | MB_COPY_XATTRS
                              | MB_COPY_EXCLUDE_TOP_LEVEL);

    // Don't create unnecessary special files in /dev to avoid install scripts
    // from overwriting partitions
    MKNOD_CHECKED(CHROOT "/dev/console",      S_IFCHR | 0644, makedev(5, 1));
    MKNOD_CHECKED(CHROOT "/dev/null",         S_IFCHR | 0644, makedev(1, 3));
    MKNOD_CHECKED(CHROOT "/dev/ptmx",         S_IFCHR | 0644, makedev(5, 2));
    MKNOD_CHECKED(CHROOT "/dev/random",       S_IFCHR | 0644, makedev(1, 8));
    MKNOD_CHECKED(CHROOT "/dev/tty",          S_IFCHR | 0644, makedev(5, 0));
    MKNOD_CHECKED(CHROOT "/dev/urandom",      S_IFCHR | 0644, makedev(1, 9));
    MKNOD_CHECKED(CHROOT "/dev/zero",         S_IFCHR | 0644, makedev(1, 5));
    MKNOD_CHECKED(CHROOT "/dev/loop-control", S_IFCHR | 0644, makedev(10, 237));

    // Create a few loopback devices in case we need to use them
    MKDIR_CHECKED(CHROOT "/dev/block", 0755);
    MKNOD_CHECKED(CHROOT "/dev/block/loop0",  S_IFBLK | 0644, makedev(7, 0));
    MKNOD_CHECKED(CHROOT "/dev/block/loop1",  S_IFBLK | 0644, makedev(7, 1));
    MKNOD_CHECKED(CHROOT "/dev/block/loop2",  S_IFBLK | 0644, makedev(7, 2));

    // We need /dev/input/* and /dev/graphics/* for AROMA
#if 1
    COPY_DIR_CONTENTS_CHECKED("/dev/input", CHROOT "/dev/input",
                              MB_COPY_ATTRIBUTES
                              | MB_COPY_XATTRS
                              | MB_COPY_EXCLUDE_TOP_LEVEL);
    COPY_DIR_CONTENTS_CHECKED("/dev/graphics", CHROOT "/dev/graphics",
                              MB_COPY_ATTRIBUTES
                              | MB_COPY_XATTRS
                              | MB_COPY_EXCLUDE_TOP_LEVEL);
#else
    MKDIR_CHECKED(CHROOT "/dev/input", 0755);
    MKDIR_CHECKED(CHROOT "/dev/graphics", 0755);
    MOUNT_CHECKED("/dev/input", CHROOT "/dev/input", "", MS_BIND, "");
    MOUNT_CHECKED("/dev/graphics", CHROOT "/dev/graphics", "", MS_BIND, "");
#endif

    mb_create_empty_file(CHROOT "/.chroot");

    return 0;

error:
    return -1;
}

static int destroy_chroot(void)
{
    umount(CHROOT "/system");
    umount(CHROOT "/cache");
    umount(CHROOT "/data");

    umount(CHROOT MB_TEMP "/system.img");

    umount(CHROOT "/dev/pts");
    umount(CHROOT "/dev");
    umount(CHROOT "/proc");
    umount(CHROOT "/sys/fs/selinux");
    umount(CHROOT "/sys");
    umount(CHROOT "/tmp");
    umount(CHROOT "/sbin");

    // Unmount everything previously mounted in /chroot
    if (mb_unmount_all(CHROOT) < 0) {
        LOGE("Failed to unmount previous mount points in " CHROOT);
        return -1;
    }

    mb_delete_recursive(CHROOT);

    return 0;
}

/*!
 * \brief Extract needed multiboot files from the patched zip file
 *
 * \return 0 on success, -1 on failure
 */
static int extract_zip_files(void)
{
    static const struct extract_info files[] = {
        { ZIP_UPDATER ".orig",  MB_TEMP "/updater"          },
        { ZIP_E2FSCK,           MB_TEMP "/e2fsck"           },
        { ZIP_RESIZE2FS,        MB_TEMP "/resize2fs"        },
        { ZIP_TUNE2FS,          MB_TEMP "/tune2fs"          },
        { ZIP_UNZIP,            MB_TEMP "/unzip"            },
        { ZIP_AROMA,            MB_TEMP "/aromawrapper.zip" },
        { ZIP_BBWRAPPER,        MB_TEMP "/bb-wrapper.sh"    },
        { ZIP_DEVICE,           MB_TEMP "/device"           },
        { ZIP_MBP_MINI,         MB_TEMP "/libmbp-mini.so"   },
        { NULL, NULL }
    };

    if (mb_extract_files2(zip_file, files) < 0) {
        LOGE("Failed to extract all multiboot files");
        return -1;
    }

    return 0;
}

/*!
 * \brief Get the target device of the patched zip file
 *
 * \return Dynamically allocated string containing the device codename or NULL
 *         if the device could not be determined
 */
static char * get_target_device(void)
{
    char *device = NULL;
    if (mb_file_first_line(MB_TEMP "/device", &device) < 0) {
        LOGE("Failed to read " MB_TEMP "/device");
        return NULL;
    }

    return device;
}

/*!
 * \brief Make e2fsprogs files executable
 *
 * \return 0 on success, -1 on failure
 */
static int setup_e2fsprogs(void)
{
    if (chmod(MB_TEMP "/e2fsck", 0555) < 0) {
        LOGE("%s: Failed to chmod: %s", MB_TEMP "/e2fsck", strerror(errno));
        return -1;
    }

    if (chmod(MB_TEMP "/resize2fs", 0555) < 0) {
        LOGE("%s: Failed to chmod: %s", MB_TEMP "/resize2fs", strerror(errno));
        return -1;
    }

    if (chmod(MB_TEMP "/tune2fs", 0555) < 0) {
        LOGE("%s: Failed to chmod: %s", MB_TEMP "/tune2fs", strerror(errno));
        return -1;
    }

    return 0;
}

/*!
 * \brief Replace /sbin/unzip in the chroot with one supporting zip flags 1 & 8
 *
 * \return 0 on success, -1 on failure
 */
static int setup_unzip(void)
{
    remove(CHROOT "/sbin/unzip");

    if (mb_copy_file(MB_TEMP "/unzip",
                     CHROOT "/sbin/unzip",
                     MB_COPY_ATTRIBUTES | MB_COPY_XATTRS) < 0) {
        LOGE("Failed to copy %s to %s: %s",
             MB_TEMP "/unzip", CHROOT "/sbin/unzip", strerror(errno));
        return -1;
    }

    if (chmod(CHROOT "/sbin/unzip", 0555) < 0) {
        LOGE("Failed to chmod %s: %s",
             CHROOT "/sbin/unzip", strerror(errno));
        return -1;
    }

    return 0;
}

/*!
 * \brief Replace busybox in the chroot with a wrapper that disables certain
 *        functions
 *
 * \return 0 on success, -1 on failure
 */
static int setup_busybox_wrapper(void)
{
    rename(CHROOT "/sbin/busybox", CHROOT "/sbin/busybox_orig");

    if (mb_copy_file(MB_TEMP "/bb-wrapper.sh",
                     CHROOT "/sbin/busybox",
                     MB_COPY_ATTRIBUTES | MB_COPY_XATTRS) < 0) {
        LOGE("Failed to copy %s to %s: %s",
             MB_TEMP "/bb-wrapper.sh", CHROOT "/sbin/busybox", strerror(errno));
        return -1;
    }

    if (chmod(CHROOT "/sbin/busybox", 0555) < 0) {
        LOGE("Failed to chmod %s: %s", CHROOT "/sbin/busybox", strerror(errno));
        return -1;
    }

    return 0;
}

/*!
 * \brief Create new ext4 image or expand existing image
 *
 * Create a 3GB image image (or extend an existing one) for installing a ROM.
 * There isn't really a point to calculate an approximate size, since either
 * way, if there isn't enough space, the ROM will fail to install anyway. After
 * installation, the image will be shrunken to its minimum size.
 *
 * \param path Image file path
 *
 * \return 0 on success, -1 on failure
 */
static int create_or_enlarge_image(const char *path)
{
#define IMAGE_SIZE "3G"

    if (mb_mkdir_parent(path, S_IRWXU) < 0) {
        LOGE("%s: Failed to create parent directory: %s",
             path, strerror(errno));
        return -1;
    }

    struct stat sb;
    if (stat(path, &sb) < 0) {
        if (errno != ENOENT) {
            LOGE("%s: Failed to stat: %s", path, strerror(errno));
            return -1;
        } else {
            LOGD("%s: Creating new %s ext4 image", path, IMAGE_SIZE);

            // Create new image
            const char *argv[] =
                    { "make_ext4fs", "-l", IMAGE_SIZE, path, NULL };
            if (mb_run_command((char **) argv) != 0) {
                LOGE("%s: Failed to create image", path);
                return -1;
            }
            return 0;
        }
    }

    // Unset uninit_bg to avoid this bug, which is not patched in some Android
    // kernels (well, at least hammerhead's kernel)
    // http://www.redhat.com/archives/dm-devel/2012-June/msg00029.html
    const char *tune2fs_argv[] =
            { MB_TEMP "/tune2fs", "-O", "^uninit_bg", path, NULL };
    if (mb_run_command((char **) tune2fs_argv) != 0) {
        LOGE("%s: Failed to clear uninit_bg flag", path);
        return -1;
    }

    // Force an fsck to make resize2fs happy
    const char *e2fsck_argv[] =
            { MB_TEMP "/e2fsck", "-f", "-y", path, NULL };
    int ret = mb_run_command((char **) e2fsck_argv);
    // 0 = no errors; 1 = errors were corrected
    if (WEXITSTATUS(ret) != 0 && WEXITSTATUS(ret) != 1) {
        LOGE("%s: Failed to run e2fsck", path);
        return -1;
    }

    LOGD("%s: Enlarging to %s", path, IMAGE_SIZE);

    // Enlarge existing image
    const char *resize2fs_argv[] =
            { MB_TEMP "/resize2fs", path, IMAGE_SIZE, NULL };
    if (mb_run_command((char **) resize2fs_argv) != 0) {
        LOGE("%s: Failed to run resize2fs", path);
        return -1;
    }

    // Rerun e2fsck to ensure we don't get mounted read-only
    mb_run_command((char **) e2fsck_argv);

    return 0;
}

/*!
 * \brief Shrink an image to its minimum size + 10MiB
 *
 * \param path Image file path
 *
 * \return 0 on success, -1 on failure
 */
static int shrink_image(const char *path)
{
    // Force an fsck to make resize2fs happy
    const char *e2fsck_argv[] = { MB_TEMP "/e2fsck", "-f", "-y", path, NULL };
    int ret = mb_run_command((char **) e2fsck_argv);
    // 0 = no errors; 1 = errors were corrected
    if (WEXITSTATUS(ret) != 0 && WEXITSTATUS(ret) != 1) {
        LOGE("%s: Failed to run e2fsck", path);
        return -1;
    }

    // Shrink image
    // resize2fs will not go below the worst case scenario, so multiple resizes
    // are needed to achieve the "true" minimum
    const char *resize2fs_argv[] = { MB_TEMP "/resize2fs", "-M", path, NULL };
    for (int i = 0; i < 1 /* 5 */; ++i) {
        if (mb_run_command((char **) resize2fs_argv) != 0) {
            LOGE("%s: Failed to run resize2fs", path);
            return -1;
        }
    }

    // Enlarge by 10MB in case the user wants to modify /system/build.prop or
    // something
    struct stat sb;
    if (stat(path, &sb) == 0) {
        off_t size_mib = sb.st_size / 1024 / 1024 + 10;
        size_t len = 3; // 'M', '\0', and first digit
        for (int n = size_mib; n /= 10; ++len);
        char *size_str = malloc(len);
        snprintf(size_str, len, "%jdM", (intmax_t) size_mib);

        // Ignore errors here
        const char *argv[] = { MB_TEMP "/resize2fs", path, size_str, NULL };
        mb_run_command((char **) argv);

        free(size_str);
    }

    // Rerun e2fsck to ensure we don't get mounted read-only
    mb_run_command((char **) e2fsck_argv);

    return 0;
}

/*!
 * \brief Compute SHA1 hash of a file
 *
 * \param path Path to file
 * \param digest `unsigned char` array of size `SHA_DIGEST_SIZE` to store
 *               computed hash value
 *
 * \return 0 on success, -1 on failure and errno set appropriately
 */
static int sha1_hash(const char *path, unsigned char digest[SHA_DIGEST_SIZE])
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        LOGE("%s: Failed to open: %s", path, strerror(errno));
        return -1;
    }

    unsigned char buf[10240];
    size_t n;

    SHA_CTX ctx;
    SHA_init(&ctx);

    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) {
        SHA_update(&ctx, buf, n);
        if (n < sizeof(buf)) {
            break;
        }
    }

    if (ferror(fp)) {
        LOGE("%s: Failed to read file", path);
        fclose(fp);
        errno = EIO;
        return -1;
    }

    fclose(fp);

    memcpy(digest, SHA_final(&ctx), SHA_DIGEST_SIZE);

    return 0;
}

/*!
 * \brief Convert binary data to its hex string representation
 *
 * The size of the output string should be at least `2 * size + 1` bytes.
 * (Two characters in string for each byte in the binary data + one byte for
 * the NULL terminator.)
 *
 * \param data Binary data
 * \param size Size of binary data
 * \param out Output string
 */
static void to_hex_string(unsigned char *data, size_t size, char *out)
{
    static const char digits[] = "0123456789abcdef";

    for (unsigned int i = 0; i < size; ++i) {
        out[2 * i] = digits[(data[i] >> 4) & 0xf];
        out[2 * i + 1] = digits[data[i] & 0xf];
    }

    out[2 * size] = '\0';
}

/*!
 * \brief Guess if an installer file in an AROMA installer
 *
 * \return 1 if file in an AROMA installer, 0 if the file is not, -1 on error
 *         (and errno set appropriately)
 */
static int is_aroma(const char *path)
{
    // Possible strings we can search for
    static const char *AROMA_MAGIC[] = {
        "AROMA Installer",
        "support@amarullz.com",
        "(c) 2013 by amarullz xda-developers",
        "META-INF/com/google/android/aroma-config",
        "META-INF/com/google/android/aroma",
        "AROMA_NAME",
        "AROMA_BUILD",
        "AROMA_VERSION",
        NULL
    };

    struct stat sb;
    void *map = MAP_FAILED;
    int fd = -1;
    const char **magic;
    int found = 0;
    int saved_errno;

    if ((fd = open(path, O_RDONLY)) < 0) {
        goto error;
    }

    if (fstat(fd, &sb) < 0) {
        goto error;
    }

    map = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        goto error;
    }

    for (magic = AROMA_MAGIC; *magic; ++magic) {
        if (memmem(map, sb.st_size, *magic, strlen(*magic))) {
            found = 1;
            break;
        }
    }

    if (munmap(map, sb.st_size) < 0) {
        map = MAP_FAILED;
        goto error;
    }

    if (close(fd) < 0) {
        fd = -1;
        goto error;
    }

    return found;

error:
    saved_errno = errno;

    if (map != MAP_FAILED) {
        munmap(map, sb.st_size);
    }

    if (fd >= 0) {
        close(fd);
    }

    errno = saved_errno;

    return -1;
}

/*!
 * \brief Run AROMA wrapper for ROM installation selection
 *
 * \return 0 on success, -1 on failure
 */
static int run_aroma_selection(void)
{
    struct extract_info aroma_files[] = {
        { ZIP_UPDATER, CHROOT MB_TEMP "/updater" },
        { NULL, NULL }
    };

    if (mb_extract_files2(MB_TEMP "/aromawrapper.zip", aroma_files) < 0) {
        LOGE("Failed to extract %s", ZIP_UPDATER);
        return -1;
    }

    if (chmod(CHROOT MB_TEMP "/updater", 0555) < 0) {
        LOGE("%s: Failed to chmod: %s",
             CHROOT MB_TEMP "/updater", strerror(errno));
        return -1;
    }

    if (mb_copy_file(MB_TEMP "/aromawrapper.zip",
                     CHROOT MB_TEMP "/aromawrapper.zip",
                     MB_COPY_ATTRIBUTES | MB_COPY_XATTRS) < 0) {
        LOGE("Failed to copy %s to %s: %s",
             MB_TEMP "/aromawrapper.zip", CHROOT MB_TEMP "/aromawrapper.zip",
             strerror(errno));
        return -1;
    }

    const char *argv[] = {
        MB_TEMP "/updater",
        interface,
        // Force output to stderr
        //output_fd_str,
        "2",
        MB_TEMP "/aromawrapper.zip",
        NULL
    };

    // Stop parent process (/sbin/recovery), so AROMA doesn't fight over the
    // framebuffer. AROMA normally already does this, but when we wrap it, it
    // just stops the wraapper.
    pid_t parent = getppid();

    kill(parent, SIGSTOP);

    int ret;
    if ((ret = mb_run_command_chroot(CHROOT, (char **) argv)) != 0) {
        kill(parent, SIGCONT);

        if (ret < 0) {
            LOGE("Failed to execute %s: %s",
                 MB_TEMP "/updater", strerror(errno));
        } else {
            LOGE("%s returned non-zero exit status",
                 MB_TEMP "/updater");
        }
        return -1;
    }

    kill(parent, SIGCONT);

    return 0;
}

/*!
 * \brief Run real update-binary in the chroot
 *
 * \return 0 on success, -1 on failure
 */
static int run_real_updater(void)
{
    if (mb_copy_file(MB_TEMP "/updater", CHROOT MB_TEMP "/updater",
                     MB_COPY_ATTRIBUTES | MB_COPY_XATTRS) < 0) {
        LOGE("Failed to copy %s to %s: %s",
             MB_TEMP "/updater", CHROOT MB_TEMP "/updater", strerror(errno));
        return -1;
    }

    if (chmod(CHROOT MB_TEMP "/updater", 0555) < 0) {
        LOGE("%s: Failed to chmod: %s",
             CHROOT MB_TEMP "/updater", strerror(errno));
        return -1;
    }

    const char *argv[] = {
        MB_TEMP "/updater",
        interface,
        output_fd_str,
        MB_TEMP "/install.zip",
        NULL
    };

    pid_t parent = getppid();
    int aroma = is_aroma(CHROOT MB_TEMP "/updater") > 0;

    LOGD("update-binary is AROMA: %d", aroma);

    if (aroma) {
        kill(parent, SIGSTOP);
    }

    int ret;
    if ((ret = mb_run_command_chroot(CHROOT, (char **) argv)) != 0) {
        if (aroma) {
            kill(parent, SIGCONT);
        }

        if (ret < 0) {
            LOGE("Failed to execute %s: %s",
                 MB_TEMP "/updater", strerror(errno));
        } else {
            LOGE("%s returned non-zero exit status",
                 MB_TEMP "/updater");
        }
        return -1;
    }

    if (aroma) {
        kill(parent, SIGCONT);
    }

    return 0;
}

/*!
 * \brief Copy /system directory excluding multiboot files
 *
 * \param source Source directory
 * \param target Target directory
 *
 * \return 0 on success, -1 on error
 */
static int copy_system(const char *source, const char *target)
{
    int ret = 0;
    FTS *ftsp = NULL;
    FTSENT *curr;

    char *files[] = { (char *) source, NULL };

    ftsp = fts_open(files, FTS_NOCHDIR | FTS_PHYSICAL | FTS_XDEV, NULL);
    if (!ftsp) {
        LOGE("%s: fts_open failed: %s", source, strerror(errno));
        ret = -1;
        goto finish;
    }

    size_t pathsize;
    char *pathbuf = NULL;

    while ((curr = fts_read(ftsp))) {
        switch (curr->fts_info) {
        case FTS_NS:
        case FTS_DNR:
        case FTS_ERR:
            LOGW("%s: fts_read error: %s",
                 curr->fts_accpath, strerror(curr->fts_errno));
            continue;

        case FTS_DC:
        case FTS_DOT:
        case FTS_NSOK:
            // Not reached
            continue;
        }

        if (curr->fts_level != 1) {
            continue;
        }

        if (strcmp(curr->fts_name, "multiboot") == 0) {
            fts_set(ftsp, curr, FTS_SKIP);
            continue;
        }

        pathsize = strlen(target) + 1 + strlen(curr->fts_name) + 1;

        char *temp_pathbuf;
        if ((temp_pathbuf = realloc(pathbuf, pathsize))) {
            pathbuf = temp_pathbuf;
        } else {
            ret = -1;
            errno = ENOMEM;
            goto finish;
        }
        pathbuf[0] = '\0';

        strlcat(pathbuf, target, pathsize);
        strlcat(pathbuf, "/", pathsize);
        strlcat(pathbuf, curr->fts_name, pathsize);

        switch (curr->fts_info) {
        case FTS_D:
            // target is the correct parameter here (or pathbuf and
            // MB_COPY_EXCLUDE_TOP_LEVEL flag)
            if (mb_copy_dir(curr->fts_accpath, target,
                            MB_COPY_ATTRIBUTES | MB_COPY_XATTRS) < 0) {
                LOGW("%s: Failed to copy directory: %s",
                     curr->fts_path, strerror(errno));
                ret = -1;
            }
            fts_set(ftsp, curr, FTS_SKIP);
            continue;

        case FTS_DP:
            continue;

        case FTS_F:
        case FTS_SL:
        case FTS_SLNONE:
        case FTS_DEFAULT:
            if (mb_copy_file(curr->fts_accpath, pathbuf,
                             MB_COPY_ATTRIBUTES | MB_COPY_XATTRS) < 0) {
                LOGW("%s: Failed to copy file: %s",
                     curr->fts_path, strerror(errno));
                ret = -1;
            }
            continue;
        }
    }

finish:
    if (ftsp) {
        fts_close(ftsp);
    }

    return ret;
}

/*!
 * \brief Copy a /system directory to an image file
 *
 * \param source Source directory
 * \param image Target image file
 * \param reverse If non-zero, then the image file is the source and the
 *                directory is the target
 *
 * \return 0 on success, -1 on error
 */
static int system_image_copy(const char *source, const char *image, int reverse)
{
    struct stat sb;
    char *loopdev = NULL;

    if (stat(MB_TEMP "/.system.tmp", &sb) < 0
            && mkdir(MB_TEMP "/.system.tmp", 0755) < 0) {
        LOGE("Failed to create %s: %s",
             MB_TEMP "/.system.tmp", strerror(errno));
        goto error;
    }

    if (!(loopdev = mb_loopdev_find_unused())) {
        LOGE("Failed to find unused loop device: %s", strerror(errno));
        goto error;
    }

    if (mb_loopdev_setup_device(loopdev, image, 0, 0) < 0) {
        LOGE("Failed to setup loop device %s: %s", loopdev, strerror(errno));
        goto error;
    }

    if (mount(loopdev, MB_TEMP "/.system.tmp", "ext4", 0, "") < 0) {
        LOGE("Failed to mount %s: %s", loopdev, strerror(errno));
        goto error;
    }

    if (reverse) {
        if (copy_system(MB_TEMP "/.system.tmp", source) < 0) {
            LOGE("Failed to copy system files from %s to %s",
                 MB_TEMP "/.system.tmp", source);
            goto error;
        }
    } else {
        if (copy_system(source, MB_TEMP "/.system.tmp") < 0) {
            LOGE("Failed to copy system files from %s to %s",
                 source, MB_TEMP "/.system.tmp");
            goto error;
        }
    }

    if (umount(MB_TEMP "/.system.tmp") < 0) {
        LOGE("Failed to unmount %s: %s",
             MB_TEMP "/.system.tmp", strerror(errno));
        goto error;
    }

    if (mb_loopdev_remove_device(loopdev) < 0) {
        LOGE("Failed to remove loop device %s: %s", loopdev, strerror(errno));
        goto error;
    }

    free(loopdev);

    return 0;

error:
    if (loopdev) {
        umount(MB_TEMP "/.system.tmp");
        mb_loopdev_remove_device(loopdev);
        free(loopdev);
    }
    return -1;
}

/*!
 * \brief Main wrapper function
 *
 * \return 0 on success, -1 on error
 */
static int update_binary(void)
{
    int ret = 0;

    struct roms r;
    mb_roms_init(&r);
    mb_roms_get_builtin(&r);

    struct libmbp mbp;
    memset(&mbp, 0, sizeof(mbp));

    CPatcherConfig * pc = NULL;

    char *device = NULL;
    char *boot_block_dev = NULL;


    ui_print("Creating chroot environment");

    if (create_chroot() < 0) {
        ui_print("Failed to create chroot environment");
        goto error;
    }


    MKDIR_CHECKED(MB_TEMP, 0755);

    if (extract_zip_files() < 0) {
        ui_print("Failed to extract multiboot files from zip");
        goto error;
    }


    // Verify device
    device = get_target_device();
    char prop1[MB_PROP_VALUE_MAX] = { 0 };
    char prop2[MB_PROP_VALUE_MAX] = { 0 };
    mb_get_property("ro.product.device", prop1, "");
    mb_get_property("ro.build.product", prop2, "");

    if (!device) {
        ui_print("Failed to determine target device");
        goto error;
    }

    LOGD("ro.product.device = %s", prop1);
    LOGD("ro.build.product = %s", prop2);
    LOGD("Target device = %s", device);

    if (!strstr(prop1, device) && !strstr(prop2, device)) {
        ui_print("Patched zip is for '%s', but device is '%s'", device, prop1);
        goto error;
    }


    // dlopen libmbp-mini.so
    if (libmbp_init(&mbp, MB_TEMP "/libmbp-mini.so") < 0) {
        ui_print("Failed to load " MB_TEMP "/libmbp-mini.so");
        goto error;
    }

    pc = mbp.mbp_config_create();

    char *version = mbp.mbp_config_version(pc);
    if (version) {
        LOGD("libmbp-mini version: %s", version);
        mbp.mbp_free(version);
    }

    // Due to optimizations in libc, strlen() may trigger valgrind errors like
    //     Address 0x4c0bf04 is 4 bytes inside a block of size 6 alloc'd
    // It's an annoyance, but not a big deal
    CDevice **device_list = mbp.mbp_config_devices(pc);
    for (CDevice **iter = device_list; *iter; ++iter) {
        // My C wrapper really needs some work...
        char *codename = mbp.mbp_device_codename(*iter);
        if (strcmp(codename, device) == 0) {
            mbp.mbp_free(codename);

            char **boot_devs = mbp.mbp_device_boot_block_devs(*iter);
            boot_block_dev = strdup(*boot_devs);

            // Ensure these are available in the chroot
            for (char **iter2 = boot_devs; *iter2; ++iter2) {
                size_t dev_path_size = strlen(*iter2) + strlen(CHROOT) + 2;
                char *dev_path = malloc(dev_path_size);
                dev_path[0] = '\0';
                strlcat(dev_path, CHROOT, dev_path_size);
                strlcat(dev_path, "/", dev_path_size);
                strlcat(dev_path, *iter2, dev_path_size);

                if (mb_mkdir_parent(dev_path, 0755) < 0) {
                    LOGE("Failed to create parent directory of %s", dev_path);
                }

                // Follow symlinks just in case the symlink source isn't in the
                // list
                if (mb_copy_file(*iter2, dev_path, MB_COPY_ATTRIBUTES
                                                 | MB_COPY_XATTRS
                                                 | MB_COPY_FOLLOW_SYMLINKS) < 0) {
                    LOGE("Failed to copy %s. Continuing anyway", *iter2);
                }

                free(dev_path);
            }

            mbp.mbp_free_array((void **) boot_devs);
        } else {
            mbp.mbp_free(codename);
        }
    }
    mbp.mbp_free(device_list);

    if (!boot_block_dev) {
        ui_print("Could not determine the boot block device");
        goto error;
    }

    LOGD("Boot block device: %s", boot_block_dev);


    // Choose install type
    if (run_aroma_selection() < 0) {
        ui_print("Failed to run AROMA");
        goto error;
    }

    char *install_type;
    if (mb_file_first_line(CHROOT MB_TEMP "/installtype", &install_type) < 0) {
        ui_print("Failed to determine install type");
        goto error;
    }

    if (strcmp(install_type, "cancelled") == 0) {
        ui_print("Cancelled installation");
        free(install_type);
        goto success;
    }

    struct rom *rom = mb_find_rom_by_id(&r, install_type);
    if (!rom) {
        ui_print("Unknown ROM ID: %s", install_type);
        free(install_type);
        goto error;
    }

    free(install_type);

    ui_print("ROM ID: %s", rom->id);
    ui_print("- /system: %s", rom->system_path);
    ui_print("- /cache: %s", rom->cache_path);
    ui_print("- /data: %s", rom->data_path);


    // Calculate SHA1 hash of the boot partition
    unsigned char hash[SHA_DIGEST_SIZE];
    if (sha1_hash(boot_block_dev, hash) < 0) {
        ui_print("Failed to compute sha1sum of boot partition");
        goto error;
    }

    char digest[2 * SHA_DIGEST_SIZE + 1];
    to_hex_string(hash, SHA_DIGEST_SIZE, digest);
    LOGD("Boot partition SHA1sum: %s", digest);


    // Extract e2fsck and resize2fs
    if (setup_e2fsprogs() < 0) {
        ui_print("Failed to extract e2fsprogs");
        goto error;
    }


    // Extract busybox's unzip tool with support for zip file data descriptors
    if (setup_unzip() < 0) {
        ui_print("Failed to extract unzip tool");
        goto error;
    }


    // Mount target filesystems
    if (mb_bind_mount(rom->cache_path, 0771, CHROOT "/cache", 0771) < 0) {
        ui_print("Failed to bind mount %s to %s",
                 rom->cache_path, CHROOT "/cache");
        goto error;
    }

    if (mb_bind_mount(rom->data_path, 0771, CHROOT "/data", 0771) < 0) {
        ui_print("Failed to bind mount %s to %s",
                 rom->data_path, CHROOT "/data");
        goto error;
    }

    if (rom->system_uses_image) {
        if (create_or_enlarge_image(rom->system_path) < 0) {
            ui_print("Failed to create or enlarge image %s", rom->system_path);
            goto error;
        }

        // The installer will handle the mounting and unmounting
        mb_create_empty_file(CHROOT MB_TEMP "/system.img");
        MOUNT_CHECKED(rom->system_path, CHROOT MB_TEMP "/system.img",
                      "", MS_BIND, "");
    } else {
        ui_print("Copying system to temporary image");

        // Create temporary image in /data
        if (create_or_enlarge_image("/data/.system.img.tmp") < 0) {
            ui_print("Failed to create temporary image %s",
                     "/data/.system.img.tmp");
            goto error;
        }

        // Copy current /system files to the image
        if (system_image_copy(rom->system_path, "/data/.system.img.tmp", 0) < 0) {
            ui_print("Failed to copy %s to %s",
                     rom->system_path, "/data/.system.img.tmp");
            goto error;
        }

        // Install to the image
        mb_create_empty_file(CHROOT MB_TEMP "/system.img");
        MOUNT_CHECKED("/data/.system.img.tmp", CHROOT MB_TEMP "/system.img",
                      "", MS_BIND, "");
    }


    // Bind-mount zip file
    mb_create_empty_file(CHROOT MB_TEMP "/install.zip");
    MOUNT_CHECKED(zip_file, CHROOT MB_TEMP "/install.zip", "", MS_BIND, "");


    // Wrap busybox to disable some applets
    setup_busybox_wrapper();


    // Copy ourself for the real update-binary to use
    mb_copy_file(mb_self_get_path(), CHROOT HELPER_TOOL,
                 MB_COPY_ATTRIBUTES | MB_COPY_XATTRS);
    chmod(CHROOT HELPER_TOOL, 0555);


    // Copy /default.prop
    mb_copy_file("/default.prop", CHROOT "/default.prop",
                 MB_COPY_ATTRIBUTES | MB_COPY_XATTRS);


    // Run real update-binary
    ui_print("Running real update-binary");
    ui_print("Here we go!");

    int updater_ret;
    if ((updater_ret = run_real_updater()) < 0) {
        ui_print("Failed to run real update-binary");
    }

#if DEBUG_SHELL
    {
        const char *argv[] = { "/sbin/sh", "-i", NULL };
        mb_run_command_chroot(CHROOT, (char **) argv);
    }
#endif


    // Umount filesystems from inside the chroot
    const char *umount_system[] = { HELPER_TOOL, "unmount", "/system", NULL };
    const char *umount_cache[] = { HELPER_TOOL, "unmount", "/cache", NULL };
    const char *umount_data[] = { HELPER_TOOL, "unmount", "/data", NULL };
    mb_run_command_chroot(CHROOT, (char **) umount_system);
    mb_run_command_chroot(CHROOT, (char **) umount_cache);
    mb_run_command_chroot(CHROOT, (char **) umount_data);


    if (rom->system_uses_image) {
        // Shrink system image file
        umount(CHROOT "/system");
        if (shrink_image(rom->system_path) < 0) {
            ui_print("Failed to shrink system image");
            goto error;
        }
    } else {
        ui_print("Copying temporary image to system");

        // Format system directory
        if (mb_wipe_directory(rom->system_path, 1) < 0) {
            ui_print("Failed to wipe %s", rom->system_path);
            goto error;
        }

        // Copy image back to system directory
        if (system_image_copy(rom->system_path,
                              "/data/.system.img.tmp", 1) < 0) {
            ui_print("Failed to copy %s to %s",
                     "/data/.system.img.tmp", rom->system_path);
            goto error;
        }
    }


    if (updater_ret < 0) {
        goto error;
    }


    // Calculate SHA1 hash of the boot partition after installation
    unsigned char new_hash[SHA_DIGEST_SIZE];
    if (sha1_hash(boot_block_dev, new_hash) < 0) {
        ui_print("Failed to compute sha1sum of boot partition");
        goto error;
    }

    char new_digest[2 * SHA_DIGEST_SIZE + 1];
    to_hex_string(new_hash, SHA_DIGEST_SIZE, new_digest);
    LOGD("Old boot partition SHA1sum: %s", digest);
    LOGD("New boot partition SHA1sum: %s", new_digest);

    // Set kernel if it was changed
    if (memcmp(hash, new_hash, SHA_DIGEST_SIZE) != 0) {
        ui_print("Boot partition was modified. Setting kernel");

        // Add /romid with the ROM ID to the ramdisk

        CBootImage *bi = mbp.mbp_bootimage_create();
        if (mbp.mbp_bootimage_load_file(bi, boot_block_dev) < 0) {
            ui_print("Failed to load boot partition image");
            mbp.mbp_bootimage_destroy(bi);
            goto error;
        }

        void *ramdisk_data;
        size_t ramdisk_size;
        ramdisk_size = mbp.mbp_bootimage_ramdisk_image(bi, &ramdisk_data);

        CCpioFile *cpio = mbp.mbp_cpiofile_create();
        if (mbp.mbp_cpiofile_load_data(cpio, ramdisk_data, ramdisk_size) < 0) {
            ui_print("Failed to load ramdisk");
            mbp.mbp_free(ramdisk_data);
            mbp.mbp_cpiofile_destroy(cpio);
            mbp.mbp_bootimage_destroy(bi);
            goto error;
        }

        // cpiofile keeps a copy of what it needs
        mbp.mbp_free(ramdisk_data);

        if (mbp.mbp_cpiofile_add_file_from_data(
                cpio, rom->id, strlen(rom->id), "romid", 0444) < 0) {
            ui_print("Failed to add ROM ID to ramdisk");
            mbp.mbp_cpiofile_destroy(cpio);
            mbp.mbp_bootimage_destroy(bi);
            goto error;
        }

        if (mbp.mbp_cpiofile_create_data(
                cpio, 1, &ramdisk_data, &ramdisk_size) < 0) {
            ui_print("Failed to create new ramdisk");
            mbp.mbp_cpiofile_destroy(cpio);
            mbp.mbp_bootimage_destroy(bi);
            goto error;
        }

        mbp.mbp_cpiofile_destroy(cpio);

        mbp.mbp_bootimage_set_ramdisk_image(bi, ramdisk_data, ramdisk_size);
        mbp.mbp_free(ramdisk_data);

        void *bootimg_data;
        size_t bootimg_size;
        mbp.mbp_bootimage_create_data(bi, &bootimg_data, &bootimg_size);

        mbp.mbp_bootimage_destroy(bi);

        // Backup kernel

        char path[100];
        snprintf(path, sizeof(path), "/data/media/0/MultiBoot/%s", rom->id);
        if (mb_mkdir_recursive(path, 0775) < 0) {
            ui_print("Failed to create %s", path);
            goto error;
        }

        strlcat(path, "/boot.img", sizeof(path));

        if (mb_file_write_data(path, bootimg_data, bootimg_size) < 0) {
            LOGE("Failed to write %s: %s", path, strerror(errno));
            ui_print("Failed to write %s", path);
            mbp.mbp_free(bootimg_data);
            goto error;
        }

        if (chmod(path, 0775) < 0) {
            // Non-fatal
            LOGE("%s: Failed to chmod: %s", path, strerror(errno));
        }

        if (mb_file_write_data(boot_block_dev, bootimg_data, bootimg_size) < 0) {
            LOGE("Failed to write %s: %s", boot_block_dev, strerror(errno));
            ui_print("Failed to write %s", boot_block_dev);
            mbp.mbp_free(bootimg_data);
            goto error;
        }

        mbp.mbp_free(bootimg_data);
    }

finish:
    ui_print("Destroying chroot environment");

    remove("/data/.system.img.tmp");

    mb_roms_cleanup(&r);

    mbp.mbp_config_destroy(pc);
    libmbp_destroy(&mbp);

    free(device);
    free(boot_block_dev);

    if (destroy_chroot() < 0) {
        ui_print("Failed to destroy chroot environment. You should reboot into"
                 " recovery again to avoid flashing issues.");
        return -1;
    }

    return ret;

error:
    ui_print("Failed to flash zip file.");
    ret = -1;
    goto finish;

success:
    ret = 0;
    goto finish;
}

static void update_binary_usage(int error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: update-binary [interface version] [output fd] [zip file]\n\n"
            "This tool wraps the real update-binary program by mounting the correct\n"
            "partitions in a chroot environment and then calls the real program.\n"
            "The real update-binary must be " ZIP_UPDATER ".orig\n"
            "in the zip file.\n\n"
            "Note: The interface version argument is completely ignored.\n");
}

int update_binary_main(int argc, char *argv[])
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
            update_binary_usage(0);
            return EXIT_SUCCESS;

        default:
            update_binary_usage(1);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind != 3) {
        update_binary_usage(1);
        return EXIT_FAILURE;
    }

    char *ptr;
    output_fd = strtol(argv[2], &ptr, 10);
    if (*ptr != '\0' || output_fd < 0) {
        fprintf(stderr, "Invalid output fd");
        return EXIT_FAILURE;
    }

    interface = argv[1];
    output_fd_str = argv[2];
    zip_file = argv[3];

    // stdout is messed up when it's appended to /tmp/recovery.log
    mb_log_set_standard_stream_all(stderr);

    return update_binary() == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
