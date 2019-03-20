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

#include <array>

#include <cstddef>
#include <cstdint>

#include <sodium/crypto_generichash_blake2b.h>
#include <sodium/crypto_pwhash_scryptsalsa208sha256.h>
#include <sodium/crypto_sign_ed25519.h>
#include <sodium/utils.h>

#include "mbcommon/outcome.h"

#include "mbsign/defs.h"

namespace mb::sign::detail
{

// Type traits to get the size of an std::array at compile time

template<typename T>
struct array_size;

template<typename T, size_t N>
struct array_size<std::array<T, N>> : public std::integral_constant<size_t, N>
{
};

template<typename T>
inline constexpr size_t array_size_v = array_size<T>::value;

// Comment max sizes
constexpr size_t UNTRUSTED_LINE_SIZE = 1024;
constexpr size_t TRUSTED_LINE_SIZE = 8192;

// Comment prefixes
static const inline char UNTRUSTED_PREFIX[] = "untrusted comment: ";
static const inline char TRUSTED_PREFIX[] = "trusted comment: ";

// The public header does not include libsodium headers, so make sure the array
// sizes are correct
static_assert(array_size_v<RawSecretKey> == crypto_sign_ed25519_SECRETKEYBYTES);
static_assert(array_size_v<RawPublicKey> == crypto_sign_ed25519_PUBLICKEYBYTES);
static_assert(array_size_v<RawSignature> == crypto_sign_ed25519_BYTES);
static_assert(array_size_v<UntrustedComment>
        == UNTRUSTED_LINE_SIZE - sizeof(UNTRUSTED_PREFIX) + 1);
static_assert(array_size_v<TrustedComment>
        == TRUSTED_LINE_SIZE - sizeof(TRUSTED_PREFIX) + 1);

//! Two bytes representing an algorithm
using AlgId = std::array<unsigned char, 2>;

//! Default signature algorithm (ed25519 PureEdDSA)
static const inline AlgId SIG_ALG = {'E', 'd'};
//! Default key derivation algorithm (scrypt)
static const inline AlgId KDF_ALG = {'S', 'c'};
//! Default checksum algorithm (blake2b)
static const inline AlgId CHK_ALG = {'B', '2'};

/*!
 * Checksum of:
 * * SKPayload::sig_alg
 * * EncSKPayload::id
 * * EncSKPayload::key
 */
using SKChecksum = std::array<unsigned char, crypto_generichash_blake2b_BYTES>;

//! Array large enough to hold the KDF salt
using KdfSalt = std::array<
    unsigned char,
    crypto_pwhash_scryptsalsa208sha256_SALTBYTES
>;

//! Array to hold little-endian 64-bit integer
using LittleEndian64 = std::array<unsigned char, 8>;

inline uint64_t from_le64(const LittleEndian64 &le64)
{
    return static_cast<uint64_t>(le64[0])
            | (static_cast<uint64_t>(le64[1]) << 8)
            | (static_cast<uint64_t>(le64[2]) << 16)
            | (static_cast<uint64_t>(le64[3]) << 24)
            | (static_cast<uint64_t>(le64[4]) << 32)
            | (static_cast<uint64_t>(le64[5]) << 40)
            | (static_cast<uint64_t>(le64[6]) << 48)
            | (static_cast<uint64_t>(le64[7]) << 56);
}

inline LittleEndian64 to_le64(const uint64_t n)
{
    return {
        static_cast<unsigned char>(n),
        static_cast<unsigned char>(n >> 8),
        static_cast<unsigned char>(n >> 16),
        static_cast<unsigned char>(n >> 24),
        static_cast<unsigned char>(n >> 32),
        static_cast<unsigned char>(n >> 40),
        static_cast<unsigned char>(n >> 48),
        static_cast<unsigned char>(n >> 56),
    };
}

//! Encrypted portion of secret key payload
struct EncSKPayload
{
    //! Unique key ID (little endian)
    LittleEndian64 id;
    //! Raw secret key
    RawSecretKey key;
    //! Checksum
    SKChecksum chk;
};

//! Layout of secret key payload
struct SKPayload
{
    //! Two bytes representing the signature algorithm
    AlgId sig_alg;
    //! Two bytes representing the key derivation algorithm
    AlgId kdf_alg;
    //! Two bytes representing the checksum algorithm
    AlgId chk_alg;
    //! Salt for the key derivation algorithm
    KdfSalt kdf_salt;
    //! CPU operations limit for the key derivation algorithm (little endian)
    LittleEndian64 kdf_opslimit;
    //! Memory limit for the key derivation algorithm (little endian)
    LittleEndian64 kdf_memlimit;
    //! Encrypted portion of secret key payload
    EncSKPayload enc;
};

//! Array to hold KDF-derived key for encrypting EncSKPayload
using EncSKPayloadKey = std::array<unsigned char, sizeof(EncSKPayload)>;

//! Layout of public key payload
struct PKPayload
{
    //! Two bytes representing the signature algorithm
    AlgId sig_alg;
    //! Unique key ID (little endian)
    LittleEndian64 id;
    //! Raw public key
    RawPublicKey key;
};

//! Layout of signature payload
struct SigPayload
{
    //! Two bytes representing the signature algorithm
    AlgId sig_alg;
    //! Unique key ID (little endian)
    LittleEndian64 id;
    //! Raw signature
    RawSignature sig;
};

//! Layout of global signature payload
struct GlobalSigPayload
{
    //! Raw signature
    RawSignature sig;
    //! Trusted comment
    TrustedComment trusted;
};

//! Array large enough to hold base64-encoded RawSignature
using RawSignatureBase64 = std::array<
    char,
    sodium_base64_ENCODED_LEN(sizeof(RawSignature),
                              sodium_base64_VARIANT_ORIGINAL)
>;

SKChecksum compute_checksum(const SKPayload &payload) noexcept;

void set_kdf_limits(SKPayload &payload, KdfSecurityLevel kdf_sec) noexcept;

oc::result<void>
apply_xor_cipher(SKPayload &payload, const char *passphrase) noexcept;

}
