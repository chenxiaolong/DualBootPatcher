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

#include "mbcommon/common.h"

#include <system_error>

namespace mb::bootimg::loki
{

enum class LokiError
{
    // Header errors
    PageSizeCannotBeZero                = 10,
    LokiHeaderTooSmall                  = 11,
    InvalidLokiMagic                    = 12,
    InvalidKernelAddress                = 13,

    // Ramdisk errors
    RamdiskOffsetGreaterThanAbootOffset = 20,
    FailedToDetermineRamdiskSize        = 21,
    NoRamdiskGzipHeaderFound            = 22,

    // Shellcode errors
    ShellcodeNotFound                   = 30,
    ShellcodePatchFailed                = 31,

    // Aboot errors
    AbootImageTooSmall                  = 40,
    AbootImageTooLarge                  = 41,
    UnsupportedAbootImage               = 42,
    AbootFunctionNotFound               = 43,
    AbootFunctionOutOfRange             = 44,
};

MB_EXPORT std::error_code make_error_code(LokiError e);

MB_EXPORT const std::error_category & loki_error_category();

}

namespace std
{
    template<>
    struct MB_EXPORT is_error_code_enum<mb::bootimg::loki::LokiError> : true_type
    {
    };
}
