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

#ifdef __cplusplus
#  include <cstdint>
#else
#  include <stdint.h>
#endif

#include "mbcommon/common.h"

#define MB_BI_ENTRY_KERNEL              (1 << 0)
#define MB_BI_ENTRY_RAMDISK             (1 << 1)
#define MB_BI_ENTRY_SECONDBOOT          (1 << 2)
#define MB_BI_ENTRY_DEVICE_TREE         (1 << 3)
#define MB_BI_ENTRY_ABOOT               (1 << 4)
#define MB_BI_ENTRY_MTK_KERNEL_HEADER   (1 << 5)
#define MB_BI_ENTRY_MTK_RAMDISK_HEADER  (1 << 6)
#define MB_BI_ENTRY_SONY_IPL            (1 << 7)
#define MB_BI_ENTRY_SONY_RPM            (1 << 8)
#define MB_BI_ENTRY_SONY_APPSBL         (1 << 9)

MB_BEGIN_C_DECLS

struct MbBiEntry;

// Basic object manipulation

MB_EXPORT struct MbBiEntry * mb_bi_entry_new();
MB_EXPORT void mb_bi_entry_free(struct MbBiEntry *entry);
MB_EXPORT void mb_bi_entry_clear(struct MbBiEntry *entry);
MB_EXPORT struct MbBiEntry * mb_bi_entry_clone(struct MbBiEntry *entry);

// Fields

// Entry type field

MB_EXPORT int mb_bi_entry_type_is_set(struct MbBiEntry *entry);
MB_EXPORT int mb_bi_entry_type(struct MbBiEntry *entry);
MB_EXPORT int mb_bi_entry_set_type(struct MbBiEntry *entry, int type);
MB_EXPORT int mb_bi_entry_unset_type(struct MbBiEntry *entry);

// Entry name field

MB_EXPORT const char * mb_bi_entry_name(struct MbBiEntry *entry);
MB_EXPORT int mb_bi_entry_set_name(struct MbBiEntry *entry,
                                   const char *name);

// Entry size field

MB_EXPORT int mb_bi_entry_size_is_set(struct MbBiEntry *entry);
MB_EXPORT uint64_t mb_bi_entry_size(struct MbBiEntry *entry);
MB_EXPORT int mb_bi_entry_set_size(struct MbBiEntry *entry,
                                   uint64_t size);
MB_EXPORT int mb_bi_entry_unset_size(struct MbBiEntry *entry);

MB_END_C_DECLS
