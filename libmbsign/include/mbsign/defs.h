/*
 * Copyright (C) 2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <cstdint>

#include "mbsign/memory.h"

namespace mb::sign
{

//! Untrusted comment size (excluding prefix, but including NULL-terminator)
static const inline size_t UNTRUSTED_COMMENT_SIZE = 1024 - 19;
//! Trusted comment size (excluding prefix, but including NULL-terminator)
static const inline size_t TRUSTED_COMMENT_SIZE = 8192 - 17;

//! Array holding NULL-terminated untrusted comment
using UntrustedComment = std::array<char, UNTRUSTED_COMMENT_SIZE>;
//! Array holding NULL-terminated trusted comment
using TrustedComment = std::array<char, TRUSTED_COMMENT_SIZE>;

//! Array holding raw secret key
using RawSecretKey = std::array<unsigned char, 64>;
//! Array holding raw public key
using RawPublicKey = std::array<unsigned char, 32>;
//! Array holding raw signature
using RawSignature = std::array<unsigned char, 64>;

struct MB_EXPORT SecretKey
{
    //! Friendly key ID
    uint64_t id;
    //! Raw secret key
    SecureUniquePtr<RawSecretKey> key;
    //! Untrusted comment
    UntrustedComment untrusted;
};

struct MB_EXPORT PublicKey
{
    //! Friendly key ID
    uint64_t id;
    //! Raw secret key
    RawPublicKey key;
    //! Untrusted comment
    UntrustedComment untrusted;
};

struct MB_EXPORT KeyPair
{
    //! Secret key
    SecretKey skey;
    //! Public key
    PublicKey pkey;
};

struct MB_EXPORT Signature
{
    //! Associated friendly key ID
    uint64_t id;
    //! Raw signature
    RawSignature sig;
    //! Untrusted comment
    UntrustedComment untrusted;
    //! Trusted comment
    TrustedComment trusted;
    //! Global signature
    RawSignature global_sig;
};

enum class KdfSecurityLevel
{
    Sensitive,
    Interactive,
};

}
