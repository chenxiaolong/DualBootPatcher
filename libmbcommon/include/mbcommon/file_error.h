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

namespace mb
{

enum class FileError
{
    ArgumentOutOfRange      = 10,
    CannotConvertEncoding   = 11,

    InvalidState            = 20,

    UnsupportedRead         = 30,
    UnsupportedWrite        = 31,
    UnsupportedSeek         = 32,
    UnsupportedTruncate     = 33,

    IntegerOverflow         = 40,

    BadFileFormat           = 50,
};

enum class FileErrorC
{
    InvalidArgument         = 10,
    InvalidState            = 20,
    Unsupported             = 30,
    InternalError           = 40,
};

MB_EXPORT std::error_code make_error_code(FileError e);
MB_EXPORT std::error_condition make_error_condition(FileErrorC ec);

MB_EXPORT const std::error_category & file_error_category();
MB_EXPORT const std::error_category & file_errorc_category();

}

namespace std
{
    template<>
    struct MB_EXPORT is_error_code_enum<mb::FileError> : true_type
    {
    };

    template<>
    struct MB_EXPORT is_error_condition_enum<mb::FileErrorC> : true_type
    {
    };
}
