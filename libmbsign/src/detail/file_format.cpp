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

#include "mbsign/detail/file_format.h"

#include <cstring>

#include "mbcommon/error_code.h"

namespace mb::sign::detail
{

/*!
 * \brief Compute checksum of the algorithm ID, key ID, and secret key
 *
 * The checksum is used to present the user with a helpful error if an invalid
 * passphrase is provided.
 *
 * \param payload Secret key payload to checksum
 *
 * \return The checksum. This function does not fail.
 */
SKChecksum compute_checksum(const SKPayload &payload) noexcept
{
    SKChecksum chk;

    crypto_generichash_blake2b_state s;

    crypto_generichash_blake2b_init(&s, nullptr, 0U, chk.size());
    crypto_generichash_blake2b_update(
            &s, payload.sig_alg.data(), payload.sig_alg.size());
    crypto_generichash_blake2b_update(
            &s, reinterpret_cast<const unsigned char *>(&payload.enc.id),
            sizeof payload.enc.id);
    crypto_generichash_blake2b_update(
            &s, payload.enc.key.data(), payload.enc.key.size());
    crypto_generichash_blake2b_final(&s, chk.data(), chk.size());

    return chk;
}

/*!
 * \brief Set key derivation algorithm CPU and memory limits
 *
 * \param[out] payload Reference to secret key payload
 * \param[in] kdf_sec KDF security level
 */
void set_kdf_limits(SKPayload &payload, KdfSecurityLevel kdf_sec) noexcept
{
    switch (kdf_sec) {
    case KdfSecurityLevel::Sensitive:
        payload.kdf_opslimit = to_le64(
                crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_SENSITIVE);
        payload.kdf_memlimit = to_le64(
                crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_SENSITIVE);
        break;
    case KdfSecurityLevel::Interactive:
        payload.kdf_opslimit = to_le64(
                crypto_pwhash_scryptsalsa208sha256_OPSLIMIT_INTERACTIVE);
        payload.kdf_memlimit = to_le64(
                crypto_pwhash_scryptsalsa208sha256_MEMLIMIT_INTERACTIVE);
        break;
    default:
        MB_UNREACHABLE("Invalid KDF security level: %d",
                       static_cast<int>(kdf_sec));
    }
}

/*!
 * \brief Implementation of the basic XOR cipher
 *
 * \param dst Buffer in which to apply the XOR cipher
 * \param src Encryption key
 * \param size Size of encryption key and buffer
 */
static void xor_buf(void * const dst, const void * const src,
                    size_t size) noexcept
{
    auto const dst_ptr = static_cast<unsigned char *>(dst);
    auto const src_ptr = static_cast<const unsigned char *>(src);

    for (size_t i = 0; i < size; ++i) {
        dst_ptr[i] ^= src_ptr[i];
    }
}

/*!
 * \brief Apply XOR cipher to payload to encrypt or decrypt the EncSKPayload
 *
 * \pre \p payload.kdf_salt, \p payload.kdf_opslimit, and
 *      \p payload.kdf_memlimit must be set appropriately before calling this
 *      function
 *
 * \post \p payload.enc will be encrypted or decrypted
 *
 * \param payload Secret key payload to encrypt or decrypt
 * \param passphrase Passphrase for key derivation function
 *
 * \return
 *   * Nothing if successful
 *   * `std::errc::not_enough_memory` if memory allocation fails
 *   * Otherwise, a specific error code related to the key derivation function
 */
oc::result<void>
apply_xor_cipher(SKPayload &payload, const char *passphrase) noexcept
{
    auto enc_key = make_secure_unique_ptr<EncSKPayloadKey>();
    if (!enc_key) {
        return std::errc::not_enough_memory;
    }

    if (crypto_pwhash_scryptsalsa208sha256(
            enc_key->data(), enc_key->size(), passphrase, strlen(passphrase),
            payload.kdf_salt.data(), from_le64(payload.kdf_opslimit),
            static_cast<size_t>(from_le64(payload.kdf_memlimit))) != 0) {
        return ec_from_errno();
    }

    xor_buf(&payload.enc, enc_key->data(), enc_key->size());

    return oc::success();
}

}
