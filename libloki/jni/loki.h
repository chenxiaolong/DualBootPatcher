#ifndef __LOKI_H_
#define __LOKI_H_

#include <android/log.h>

#define LOG_TAG "loki"

#define LOG(prio, tag, fmt...) __android_log_print(prio, tag, fmt)
#define LOGD(...) LOG(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) LOG(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) LOG(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGV(...) LOG(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGW(...) LOG(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

#define VERSION "2.1"

#define BOOT_MAGIC_SIZE 8
#define BOOT_NAME_SIZE 16
#define BOOT_ARGS_SIZE 512

#define BOOT_PARTITION      "/dev/block/platform/msm_sdcc.1/by-name/boot"
#define RECOVERY_PARTITION  "/dev/block/platform/msm_sdcc.1/by-name/recovery"
#define ABOOT_PARTITION     "/dev/block/platform/msm_sdcc.1/by-name/aboot"

#define PATTERN1 "\xf0\xb5\x8f\xb0\x06\x46\xf0\xf7"
#define PATTERN2 "\xf0\xb5\x8f\xb0\x07\x46\xf0\xf7"
#define PATTERN3 "\x2d\xe9\xf0\x41\x86\xb0\xf1\xf7"
#define PATTERN4 "\x2d\xe9\xf0\x4f\xad\xf5\xc6\x6d"
#define PATTERN5 "\x2d\xe9\xf0\x4f\xad\xf5\x21\x7d"
#define PATTERN6 "\x2d\xe9\xf0\x4f\xf3\xb0\x05\x46"

#define ABOOT_BASE_SAMSUNG 0x88dfffd8
#define ABOOT_BASE_LG 0x88efffd8
#define ABOOT_BASE_G2 0xf7fffd8
#define ABOOT_BASE_VIPER 0x40100000

struct boot_img_hdr {
    unsigned char magic[BOOT_MAGIC_SIZE];
    unsigned kernel_size;   /* size in bytes */
    unsigned kernel_addr;   /* physical load addr */
    unsigned ramdisk_size;  /* size in bytes */
    unsigned ramdisk_addr;  /* physical load addr */
    unsigned second_size;   /* size in bytes */
    unsigned second_addr;   /* physical load addr */
    unsigned tags_addr;     /* physical addr for kernel tags */
    unsigned page_size;     /* flash page size we assume */
    unsigned dt_size;       /* device_tree in bytes */
    unsigned unused;        /* future expansion: should be 0 */
    unsigned char name[BOOT_NAME_SIZE];    /* asciiz product name */
    unsigned char cmdline[BOOT_ARGS_SIZE];
    unsigned id[8];         /* timestamp / checksum / sha1 / etc */
};

struct loki_hdr {
    unsigned char magic[4];     /* 0x494b4f4c */
    unsigned int recovery;      /* 0 = boot.img, 1 = recovery.img */
    char build[128];   /* Build number */

    unsigned int orig_kernel_size;
    unsigned int orig_ramdisk_size;
    unsigned int ramdisk_addr;
};

int loki_patch(const char* partition_label, const char* aboot_image, const char* in_image, const char* out_image);
int loki_flash(const char* partition_label, const char* loki_image);
int loki_find(const char* aboot_image);
int loki_unlok(const char* in_image, const char* out_image);

#define PATCH	"\xfe\xb5"			\
				"\x0d\x4d"			\
				"\xd5\xf8"			\
				"\x88\x04"			\
				"\xab\x68"			\
				"\x98\x42"			\
				"\x12\xd0"			\
				"\xd5\xf8"			\
				"\x90\x64"			\
				"\x0a\x4c"			\
				"\xd5\xf8"			\
				"\x8c\x74"			\
				"\x07\xf5\x80\x57"	\
				"\x0f\xce"			\
				"\x0f\xc4"			\
				"\x10\x3f"			\
				"\xfb\xdc"			\
				"\xd5\xf8"			\
				"\x88\x04"			\
				"\x04\x49"			\
				"\xd5\xf8"			\
				"\x8c\x24"			\
				"\xa8\x60"			\
				"\x69\x61"			\
				"\x2a\x61"			\
				"\x00\x20"			\
				"\xfe\xbd"			\
				"\xff\xff\xff\xff"	\
				"\xee\xee\xee\xee"

#endif //__LOKI_H_
