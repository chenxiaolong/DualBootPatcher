/*
 * Copyright (c) 2013 Dan Rosenberg. All rights reserved.
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in
 *      the documentation and/or other materials provided with the
 *      distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INFRAE OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "mbbootimg/header.h"

#define LOKI_MAGIC              "LOKI"
#define LOKI_MAGIC_SIZE         4
#define LOKI_MAGIC_OFFSET       0x400

#define LOKI_SHELLCODE          \
        "\xfe\xb5"              \
        "\x0d\x4d"              \
        "\xd5\xf8"              \
        "\x88\x04"              \
        "\xab\x68"              \
        "\x98\x42"              \
        "\x12\xd0"              \
        "\xd5\xf8"              \
        "\x90\x64"              \
        "\x0a\x4c"              \
        "\xd5\xf8"              \
        "\x8c\x74"              \
        "\x07\xf5\x80\x57"      \
        "\x0f\xce"              \
        "\x0f\xc4"              \
        "\x10\x3f"              \
        "\xfb\xdc"              \
        "\xd5\xf8"              \
        "\x88\x04"              \
        "\x04\x49"              \
        "\xd5\xf8"              \
        "\x8c\x24"              \
        "\xa8\x60"              \
        "\x69\x61"              \
        "\x2a\x61"              \
        "\x00\x20"              \
        "\xfe\xbd"              \
        "\xff\xff\xff\xff"      \
        "\xee\xee\xee\xee"

// This is supposed to include the NULL-terminator
#define LOKI_SHELLCODE_SIZE     65

#define LOKI_IS_LG_RAMDISK_ADDR(ADDR) \
        ((ADDR) > 0x88f00000 || (ADDR) < 0xfa00000)

#define LOKI_MAX_HEADER_OFFSET  32

#define LOKI_OLD_SUPPORTED_FIELDS \
        (MB_BI_HEADER_FIELD_KERNEL_SIZE \
        | MB_BI_HEADER_FIELD_KERNEL_ADDRESS \
        | MB_BI_HEADER_FIELD_RAMDISK_SIZE \
        | MB_BI_HEADER_FIELD_RAMDISK_ADDRESS \
        | MB_BI_HEADER_FIELD_SECONDBOOT_ADDRESS \
        | MB_BI_HEADER_FIELD_KERNEL_TAGS_ADDRESS \
        | MB_BI_HEADER_FIELD_PAGE_SIZE \
        | MB_BI_HEADER_FIELD_UNUSED \
        | MB_BI_HEADER_FIELD_BOARD_NAME \
        | MB_BI_HEADER_FIELD_KERNEL_CMDLINE \
        | MB_BI_HEADER_FIELD_ID)

#define LOKI_NEW_SUPPORTED_FIELDS \
        (MB_BI_HEADER_FIELD_KERNEL_SIZE \
        | MB_BI_HEADER_FIELD_KERNEL_ADDRESS \
        | MB_BI_HEADER_FIELD_RAMDISK_SIZE \
        | MB_BI_HEADER_FIELD_RAMDISK_ADDRESS \
        | MB_BI_HEADER_FIELD_SECONDBOOT_ADDRESS \
        | MB_BI_HEADER_FIELD_KERNEL_TAGS_ADDRESS \
        | MB_BI_HEADER_FIELD_PAGE_SIZE \
        | MB_BI_HEADER_FIELD_DEVICE_TREE_SIZE \
        | MB_BI_HEADER_FIELD_UNUSED \
        | MB_BI_HEADER_FIELD_BOARD_NAME \
        | MB_BI_HEADER_FIELD_KERNEL_CMDLINE \
        | MB_BI_HEADER_FIELD_ID)
