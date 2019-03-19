/*
 * Copyright (C) 2018-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
    case Error::Base64DecodeError:
        return "base64 decode error";
    case Error::IncorrectChecksum:
        return "incorrect checksum";
    case Error::InvalidPayloadSize:
        return "invalid data payload size";
    case Error::InvalidGlobalSigSize:
        return "invalid global signature size";
    case Error::InvalidUntrustedComment:
        return "invalid untrusted comment";
    case Error::InvalidTrustedComment:
        return "invalid trusted comment";
    case Error::MismatchedKey:
        return "mismatched key";
    case Error::UnsupportedSigAlg:
        return "unsupported signature algorithm";
    case Error::UnsupportedKdfAlg:
        return "unsupported key derivation function";
    case Error::UnsupportedChkAlg:
        return "unsupported checksum algorithm";
    case Error::ComputeSigFailed:
        return "failed to compute signature";
    case Error::DataFileTooLarge:
        return "data file is too large";
    case Error::SignatureVerifyFailed:
        return "signature verification failed";
    default:
        return "(unknown sign/verify error)";
    }
}

}
