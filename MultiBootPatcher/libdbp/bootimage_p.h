#ifndef BOOTIMAGE_P_H
#define BOOTIMAGE_P_H

#include "bootimage.h"
#include "patchererror.h"

#include <QtCore/QString>


static const char *BOOT_MAGIC = "ANDROID!";
static const int BOOT_MAGIC_SIZE = 8;
static const int BOOT_NAME_SIZE = 16;
static const int BOOT_ARGS_SIZE = 512;

static const char *DEFAULT_BOARD = "";
static const char *DEFAULT_CMDLINE = "";
static const uint DEFAULT_PAGE_SIZE = 2048u;
static const uint DEFAULT_BASE = 0x10000000u;
static const uint DEFAULT_KERNEL_OFFSET = 0x00008000u;
static const uint DEFAULT_RAMDISK_OFFSET = 0x01000000u;
static const uint DEFAULT_SECOND_OFFSET = 0x00f00000u;
static const uint DEFAULT_TAGS_OFFSET = 0x00000100u;


struct BootImageHeader
{
    unsigned char magic[BOOT_MAGIC_SIZE];

    unsigned kernel_size;   /* size in bytes */
    unsigned kernel_addr;   /* physical load addr */

    unsigned ramdisk_size;  /* size in bytes */
    unsigned ramdisk_addr;  /* physical load addr */

    unsigned second_size;   /* size in bytes */
    unsigned second_addr;   /* physical load addr */

    unsigned tags_addr;     /* physical addr for kernel tags */
    unsigned page_size;     /* flash page size we assume */
    unsigned dt_size;       /* device tree in bytes */
    unsigned unused;        /* future expansion: should be 0 */
    unsigned char name[BOOT_NAME_SIZE]; /* asciiz product name */

    unsigned char cmdline[BOOT_ARGS_SIZE];

    unsigned id[8]; /* timestamp / checksum / sha1 / etc */
};


static const char *LOKI_MAGIC = "LOKI";

struct LokiHeader {
    unsigned char magic[4]; /* 0x494b4f4c */
    unsigned int recovery;  /* 0 = boot.img, 1 = recovery.img */
    char build[128];        /* Build number */

    unsigned int orig_kernel_size;
    unsigned int orig_ramdisk_size;
    unsigned int ramdisk_addr;
};


// From loki.h in the original source code:
// https://raw.githubusercontent.com/djrbliss/loki/master/loki.h
const char *SHELL_CODE =
        "\xfe\xb5"
        "\x0d\x4d"
        "\xd5\xf8"
        "\x88\x04"
        "\xab\x68"
        "\x98\x42"
        "\x12\xd0"
        "\xd5\xf8"
        "\x90\x64"
        "\x0a\x4c"
        "\xd5\xf8"
        "\x8c\x74"
        "\x07\xf5\x80\x57"
        "\x0f\xce"
        "\x0f\xc4"
        "\x10\x3f"
        "\xfb\xdc"
        "\xd5\xf8"
        "\x88\x04"
        "\x04\x49"
        "\xd5\xf8"
        "\x8c\x24"
        "\xa8\x60"
        "\x69\x61"
        "\x2a\x61"
        "\x00\x20"
        "\xfe\xbd"
        "\xff\xff\xff\xff"
        "\xee\xee\xee\xee";


class BootImagePrivate
{
public:
    enum COMPRESSION_TYPES {
        GZIP,
        LZ4
    };

    // Android boot image header
    BootImageHeader header;

    // Various images stored in the boot image
    QByteArray kernelImage;
    QByteArray ramdiskImage;
    QByteArray secondBootloaderImage;
    QByteArray deviceTreeImage;

    // Whether the ramdisk is LZ4 or gzip compressed
    int ramdiskCompression = GZIP;

    PatcherError::Error errorCode;
    QString errorString;
};

#endif // BOOTIMAGE_P_H
