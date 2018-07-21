/*
 * Copyright (C) 2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include "mbcommon/outcome.h"

namespace mb::sign
{

enum class Error
{
    InvalidKeyFormat        = 10,
    InvalidSignatureVersion = 11,
    InvalidSignatureMagic   = 12,

    BadSignature            = 20,

    PrivateKeyLoadError     = 30,
    PublicKeyLoadError      = 31,
    Pkcs12LoadError         = 32,
    Pkcs12MacVerifyError    = 33,

    IoError                 = 40,

    OpensslError            = 50,

    InternalError           = 60,
};

MB_EXPORT std::error_code make_error_code(Error e);

MB_EXPORT const std::error_category & error_category();

struct ErrorInfo
{
    std::error_code ec;
    bool has_openssl_error;
};

inline const std::error_code & make_error_code(const ErrorInfo &ei)
{
    return ei.ec;
}

// https://github.com/ned14/outcome/issues/118
inline void outcome_throw_as_system_error_with_payload(const ErrorInfo &ei)
{
    (void) ei;
    OUTCOME_THROW_EXCEPTION(std::system_error(make_error_code(ei)));
}

template <typename T>
using Result = oc::result<T, ErrorInfo>;

}

namespace std
{
    template<>
    struct MB_EXPORT is_error_code_enum<mb::sign::Error> : true_type
    {
    };
}
