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

#include "mbsign/error.h"

#include <string>

namespace mb::sign
{

struct ErrorCategory : std::error_category
{
    const char * name() const noexcept override;

    std::string message(int ev) const override;
};

const std::error_category & error_category()
{
    static ErrorCategory c;
    return c;
}

std::error_code make_error_code(Error e)
{
    return {static_cast<int>(e), error_category()};
}

const char * ErrorCategory::name() const noexcept
{
    return "sign";
}

std::string ErrorCategory::message(int ev) const
{
    switch (static_cast<Error>(ev)) {
    case Error::InvalidKeyFormat:
        return "invalid key format";
    case Error::InvalidSignatureVersion:
        return "invalid signature version";
    case Error::InvalidSignatureMagic:
        return "invalid signature magic";
    case Error::BadSignature:
        return "bad signature";
    case Error::PrivateKeyLoadError:
        return "failed to load private key";
    case Error::PublicKeyLoadError:
        return "failed to load public key";
    case Error::Pkcs12LoadError:
        return "failed to load PKCS12 file";
    case Error::Pkcs12MacVerifyError:
        return "failed to verify MAC in PKCS12 file";
    case Error::IoError:
        return "I/O error";
    case Error::OpensslError:
        return "OpenSSL error";
    case Error::InternalError:
        return "internal error";
    default:
        return "(unknown sign/verify error)";
    }
}

}
