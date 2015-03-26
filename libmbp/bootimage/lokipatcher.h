/*
 * Copyright (c) 2013 Dan Rosenberg. All rights reserved.
 * Copyright (c) 2015 Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
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

/*
 * This is a C++ adaptation of Loki and is licensed under the same terms as the
 * original source code. It can be used standalone (outside of MultiBootPatcher)
 * under the terms above.
 */

#pragma once

#include <vector>
#include <cstdint>


#define LOKI_MAGIC              "LOKI"
#define LOKI_MAGIC_SIZE         4

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

#define LOKI_SHELLCODE_SIZE     65 // sizeof(LOKI_SHELLCODE)


struct LokiHeader {
    unsigned char magic[4]; /* 0x494b4f4c */
    uint32_t recovery;      /* 0 = boot.img, 1 = recovery.img */
    char build[128];        /* Build number */

    uint32_t orig_kernel_size;
    uint32_t orig_ramdisk_size;
    uint32_t ramdisk_addr;
};


class LokiPatcher
{
public:
    static bool patchImage(std::vector<unsigned char> *data,
                           std::vector<unsigned char> aboot);
};