/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "mbbootimg/guard_p.h"

#ifdef __cplusplus
#  include <cstdint>
#else
#  include <stdint.h>
#endif

#include "mbcommon/common.h"
#include "mbcommon/endian.h"

#include "mbbootimg/format/loki_defs.h"
#include "mbbootimg/writer.h"

struct LokiHeader
{
    unsigned char magic[4];         // 0x494b4f4c
    uint32_t recovery;              // 0 = boot.img, 1 = recovery.img
    char build[128];                // Build number

    uint32_t orig_kernel_size;
    uint32_t orig_ramdisk_size;
    uint32_t ramdisk_addr;
};

MB_BEGIN_C_DECLS

static inline void loki_fix_header_byte_order(struct LokiHeader *header)
{
    header->recovery = mb_le32toh(header->recovery);
    header->orig_kernel_size = mb_le32toh(header->orig_kernel_size);
    header->orig_ramdisk_size = mb_le32toh(header->orig_ramdisk_size);
    header->ramdisk_addr = mb_le32toh(header->ramdisk_addr);
}

int _loki_patch_file(struct MbBiWriter *biw, struct MbFile *file,
                     const void *aboot, size_t aboot_size);

MB_END_C_DECLS
