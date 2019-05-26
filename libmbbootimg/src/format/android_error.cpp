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

#include "mbbootimg/format/android_error.h"

#include <string>

namespace mb::bootimg::android
{

struct AndroidErrorCategory : std::error_category
{
    const char * name() const noexcept override;

    std::string message(int ev) const override;
};

const std::error_category & android_error_category()
{
    static AndroidErrorCategory c;
    return c;
}

std::error_code make_error_code(AndroidError e)
{
    return {static_cast<int>(e), android_error_category()};
}

const char * AndroidErrorCategory::name() const noexcept
{
    return "android_format";
}

std::string AndroidErrorCategory::message(int ev) const
{
    switch (static_cast<AndroidError>(ev)) {
    case AndroidError::InvalidArgument:
        return "invalid argument";
    case AndroidError::HeaderNotFound:
        return "header not found";
    case AndroidError::HeaderOutOfBounds:
        return "header out of bounds";
    case AndroidError::InvalidPageSize:
        return "invalid page size";
    case AndroidError::MissingPageSize:
        return "missing page size field";
    case AndroidError::BoardNameTooLong:
        return "board name too long";
    case AndroidError::KernelCmdlineTooLong:
        return "kernel cmdline too long";
    case AndroidError::BumpMagicNotFound:
        return "bump magic not found";
    case AndroidError::SamsungMagicNotFound:
        return "Samsung SEAndroid magic not found";
    case AndroidError::Sha1InitError:
        return "SHA1 hash initialization error";
    case AndroidError::Sha1UpdateError:
        return "failed to update SHA1 hash";
    default:
        return "(unknown Android format error)";
    }
}

}
