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

#include <cstddef>
#include <cstdint>

#include "mbbootimg/header.h"

namespace mb::bootimg::loki
{

constexpr char LOKI_MAGIC[] = "LOKI";
constexpr size_t LOKI_MAGIC_SIZE = 4;
constexpr size_t LOKI_MAGIC_OFFSET = 0x400;

constexpr unsigned char LOKI_SHELLCODE[] =
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

// This is supposed to include the NULL-terminator
constexpr size_t LOKI_SHELLCODE_SIZE = 65;

constexpr bool is_lg_ramdisk_address(uint32_t address)
{
    return address > 0x88f00000 || address < 0xfa00000;
}

constexpr size_t LOKI_MAX_HEADER_OFFSET = 32;

constexpr HeaderFields OLD_SUPPORTED_FIELDS =
        HeaderField::KernelSize
        | HeaderField::KernelAddress
        | HeaderField::RamdiskSize
        | HeaderField::RamdiskAddress
        | HeaderField::SecondbootAddress
        | HeaderField::KernelTagsAddress
        | HeaderField::PageSize
        | HeaderField::Unused
        | HeaderField::BoardName
        | HeaderField::KernelCmdline
        | HeaderField::Id;

constexpr HeaderFields NEW_SUPPORTED_FIELDS =
        HeaderField::KernelSize
        | HeaderField::KernelAddress
        | HeaderField::RamdiskSize
        | HeaderField::RamdiskAddress
        | HeaderField::SecondbootAddress
        | HeaderField::KernelTagsAddress
        | HeaderField::PageSize
        | HeaderField::DeviceTreeSize
        | HeaderField::Unused
        | HeaderField::BoardName
        | HeaderField::KernelCmdline
        | HeaderField::Id;

}
