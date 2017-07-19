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

#include "mbbootimg/format/mtk_defs.h"

struct MtkHeader
{
    unsigned char magic[MTK_MAGIC_SIZE];    // MTK magic
    uint32_t size;                          // Image size
    char type[MTK_TYPE_SIZE];               // Image type
    char unused[MTK_UNUSED_SIZE];           // Unused (all 0xff)
};

MB_BEGIN_C_DECLS

static inline void mtk_fix_header_byte_order(struct MtkHeader *header)
{
    header->size = mb_le32toh(header->size);
}

MB_END_C_DECLS
