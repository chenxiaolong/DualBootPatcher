/*
 * Copyright (C) 2017-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

namespace mb::bootimg::android
{

enum class AndroidError
{
    InvalidArgument         = 1,

    // Android header errors
    HeaderNotFound          = 10,
    HeaderOutOfBounds       = 11,
    InvalidPageSize         = 12,
    MissingPageSize         = 13,
    BoardNameTooLong        = 14,
    KernelCmdlineTooLong    = 15,

    // Bump errors
    BumpMagicNotFound       = 20,

    // Samsung SEAndroid errors
    SamsungMagicNotFound    = 30,

    // SHA1 errors
    Sha1InitError           = 40,
    Sha1UpdateError         = 41,
};

MB_EXPORT std::error_code make_error_code(AndroidError e);

MB_EXPORT const std::error_category & android_error_category();

}

namespace std
{
    template<>
    struct MB_EXPORT is_error_code_enum<mb::bootimg::android::AndroidError> : true_type
    {
    };
}
