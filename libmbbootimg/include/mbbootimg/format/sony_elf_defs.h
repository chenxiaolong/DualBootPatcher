/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbbootimg/header.h"

#define SONY_ELF_SUPPORTED_FIELDS \
        (MB_BI_HEADER_FIELD_KERNEL_ADDRESS \
        | MB_BI_HEADER_FIELD_RAMDISK_ADDRESS \
        | MB_BI_HEADER_FIELD_SONY_IPL_ADDRESS \
        | MB_BI_HEADER_FIELD_SONY_RPM_ADDRESS \
        | MB_BI_HEADER_FIELD_SONY_APPSBL_ADDRESS \
        | MB_BI_HEADER_FIELD_KERNEL_CMDLINE \
        | MB_BI_HEADER_FIELD_ENTRYPOINT)
