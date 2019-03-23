/*
 * Copyright (C) 2016-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbsign/sign.h"

#include <algorithm>
#include <chrono>

#include <cinttypes>
#include <cstring>

#include <sodium/core.h>
#include <sodium/crypto_sign_ed25519.h>
#include <sodium/randombytes.h>
#include <sodium/utils.h>

#include "mbcommon/error_code.h"
#include "mbcommon/file/buffered.h"
#include "mbcommon/file_util.h"
#include "mbcommon/string.h"

#include "mbsign/detail/file_format.h"
#include "mbsign/detail/raw_file.h"
#include "mbsign/error.h"

namespace mb::sign
{

using namespace detail;

/*!
 * \brief Initialize global state in underlying crypto library
 *
 * \return Whether the underlying crypto library successfully initialized. If
 *         false is returned, do NOT call any other functions in this library or
 *         else the whole program might abort.
 */
bool initialize() noexcept
{
    return sodium_init() >= 0;
}

/*!
 * \brief Generate ed25519 keypair
 *
 * \return
 *   * The ed25519 keypair if successful
 *   * `std::errc::not_enough_memory` if secure memory allocation fails
 */
oc::result<KeyPair> generate_keypair() noexcept
{
    KeyPair kp;
    kp.skey.key = make_secure_unique_ptr<RawSecretKey>();
    if (!kp.skey.key) {
        return std::errc::not_enough_memory;
    }

    // Generate friendly key ID
    randombytes_buf(&kp.skey.id, sizeof kp.skey.id);
    kp.pkey.id = kp.skey.id;

    // Generate keypair
    crypto_sign_ed25519_keypair(kp.pkey.key.data(), kp.skey.key->data());

    // Write comments
    snprintf(kp.skey.untrusted.data(), kp.skey.untrusted.size(),
             "libmbsign secret key %" PRIX64, kp.skey.id);
    snprintf(kp.pkey.untrusted.data(), kp.pkey.untrusted.size(),
             "libmbsign public key %" PRIX64, kp.pkey.id);

    return std::move(kp);
}

/*!
 * \brief Write encrypted secret key to file
 *
 * \param file Output file
 * \param key Secret key to save
 * \param passphrase Passphrase for encryption
 * \param kdf_sec Key derivation function security level
 *
 * \return
 *   * Nothing if successful
 *   * `std::errc::not_enough_memory` if memory allocation fails
 *   * Error::InvalidUntrustedComment if the untrusted comment contains newlines
 *   * Otherwise, a specific I/O error code
 */
oc::result<void>
save_secret_key(File &file, const SecretKey &key, const char *passphrase,
                KdfSecurityLevel kdf_sec) noexcept
{
    auto payload = make_secure_unique_ptr<SKPayload>();
    if (!payload) {
        return std::errc::not_enough_memory;
    }

    payload->sig_alg = SIG_ALG;
    payload->kdf_alg = KDF_ALG;
    payload->chk_alg = CHK_ALG;
    randombytes_buf(payload->kdf_salt.data(), payload->kdf_salt.size());
    set_kdf_limits(*payload, kdf_sec);
    payload->enc.id = to_le64(key.id);
    payload->enc.key = *key.key;
    payload->enc.chk = compute_checksum(*payload);

    OUTCOME_TRYV(apply_xor_cipher(*payload, passphrase));

    return save_raw_file(file, as_uchars(*payload), key.untrusted, nullptr,
                         nullptr);
}

/*!
 * \brief Load encrypted secret key from file
 *
 * \param file Input file
 * \param passphrase Passphrase for decryption
 *
 * \return
 *   * Secret key if successful
 *   * `std::errc::not_enough_memory` if memory allocation fails
 *   * Error::InvalidUntrustedComment if the untrusted comment is too long
 *   * Error::Base64DecodeError if invalid base64 data is encountered
 *   * Error::InvalidPayloadSize if the payload size is incorrect
 *   * Otherwise, a specific I/O error code
 */
oc::result<SecretKey>
load_secret_key(File &file, const char *passphrase) noexcept
{
    auto payload = make_secure_unique_ptr<SKPayload>();
    if (!payload) {
        return std::errc::not_enough_memory;
    }

    SecretKey key;

    OUTCOME_TRYV(load_raw_file(file, as_writable_uchars(*payload),
                               key.untrusted, nullptr, nullptr));

    if (payload->sig_alg != SIG_ALG) {
        return Error::UnsupportedSigAlg;
    }

    if (payload->kdf_alg != KDF_ALG) {
        return Error::UnsupportedKdfAlg;
    }

    if (payload->chk_alg != CHK_ALG) {
        return Error::UnsupportedChkAlg;
    }

    OUTCOME_TRYV(apply_xor_cipher(*payload, passphrase));

    auto checksum = compute_checksum(*payload);

    if (checksum != payload->enc.chk) {
        return Error::IncorrectChecksum;
    }

    key.key = make_secure_unique_ptr<RawSecretKey>();
    if (!key.key) {
        return std::errc::not_enough_memory;
    }

    key.id = from_le64(payload->enc.id);
    *key.key = payload->enc.key;

    return std::move(key);
}

/*!
 * \brief Write public key to file
 *
 * \param file Output file
 * \param key Public key to save
 *
 * \return
 *   * Nothing if successful
 *   * `std::errc::not_enough_memory` if memory allocation fails
 *   * Error::InvalidUntrustedComment if the untrusted comment contains newlines
 *   * Otherwise, a specific I/O error code
 */
oc::result<void>
save_public_key(File &file, const PublicKey &key) noexcept
{
    PKPayload payload = {};
    payload.sig_alg = SIG_ALG;
    payload.id = to_le64(key.id);
    payload.key = key.key;

    return save_raw_file(file, as_uchars(payload), key.untrusted, nullptr,
                         nullptr);
}

/*!
 * \brief Load public key from file
 *
 * \param file Input file
 *
 * \return
 *   * Public key if successful
 *   * `std::errc::not_enough_memory` if memory allocation fails
 *   * Error::InvalidUntrustedComment if the untrusted comment is too long
 *   * Error::Base64DecodeError if invalid base64 data is encountered
 *   * Error::InvalidPayloadSize if the payload size is incorrect
 *   * Otherwise, a specific I/O error code
 */
oc::result<PublicKey>
load_public_key(File &file) noexcept
{
    PKPayload payload;
    PublicKey key;

    OUTCOME_TRYV(load_raw_file(file, as_writable_uchars(payload), key.untrusted,
                               nullptr, nullptr));

    if (payload.sig_alg != SIG_ALG) {
        return Error::UnsupportedSigAlg;
    }

    key.id = from_le64(payload.id);
    key.key = payload.key;

    return std::move(key);
}

/*!
 * \brief Write signature to file
 *
 * \param file Output file
 * \param sig Signature to save
 *
 * \return
 *   * Nothing if successful
 *   * `std::errc::not_enough_memory` if memory allocation fails
 *   * Error::InvalidUntrustedComment if the untrusted comment contains newlines
 *   * Error::InvalidTrustedComment if the trusted comment contains newlines
 *   * Otherwise, a specific I/O error code
 */
oc::result<void>
save_signature(File &file, const Signature &sig) noexcept
{
    SigPayload payload = {};
    payload.sig_alg = SIG_ALG;
    payload.id = to_le64(sig.id);
    payload.sig = sig.sig;

    return save_raw_file(file, as_uchars(payload), sig.untrusted, &sig.trusted,
                         &sig.global_sig);
}

/*!
 * \brief Load signature from file
 *
 * \param file Input file
 *
 * \return
 *   * Signature if successful
 *   * `std::errc::not_enough_memory` if memory allocation fails
 *   * Error::InvalidUntrustedComment if the untrusted comment is too long
 *   * Error::InvalidTrustedComment if the trusted comment is too long
 *   * Error::Base64DecodeError if invalid base64 data is encountered
 *   * Error::InvalidPayloadSize if the payload size is incorrect
 *   * Otherwise, a specific I/O error code
 */
oc::result<Signature>
load_signature(File &file) noexcept
{
    SigPayload payload;
    Signature sig;

    OUTCOME_TRYV(load_raw_file(file, as_writable_uchars(payload), sig.untrusted,
                               &sig.trusted, &sig.global_sig));

    if (payload.sig_alg != SIG_ALG) {
        return Error::UnsupportedSigAlg;
    }

    sig.id = from_le64(payload.id);
    sig.sig = payload.sig;

    return std::move(sig);
}

static void set_default_trusted_comment(TrustedComment &trusted)
{
    using namespace std::chrono;

    auto tp = system_clock::now().time_since_epoch();
    auto t = duration_cast<seconds>(tp);

    snprintf(trusted.data(), trusted.size(), "timestamp:%lu",
             static_cast<unsigned long>(t.count()));
}

static oc::result<std::vector<unsigned char>> read_to_memory(File &file)
{
    std::vector<unsigned char> data;

    OUTCOME_TRY(size, file.seek(0, SEEK_END));

    if (size > 1ul << 30) {
        return Error::DataFileTooLarge;
    }

    data.reserve(static_cast<size_t>(size));
    data.resize(static_cast<size_t>(size));

    OUTCOME_TRYV(file.seek(0, SEEK_SET));
    OUTCOME_TRYV(file_read_exact(file, data));

    return std::move(data);
}

/*!
 * \brief Compute signature of file
 *
 * The untrusted comment will default to a message including the key ID and
 * trusted comment will default to a message containing the current timestamp.
 * Both comments can be changed by updating Signature::untrusted or
 * Signature::trusted. If Signature::trusted is updated, the
 * update_global_signature() must be called to recompute Signature::global_sig.
 *
 * \param file Input file
 * \param key Secret key to use for signing
 *
 * \return
 *   * Signature if successful
 *   * Error::ComputeSigFailed if signature computation fails
 *   * Otherwise, a specific I/O error code
 */
oc::result<Signature>
sign_file(File &file, const SecretKey &key) noexcept
{
    Signature sig;
    sig.id = key.id;

    // Set default untrusted comment
    snprintf(sig.untrusted.data(), sig.untrusted.size(),
             "libmbsign signature signed by secret key %" PRIX64, key.id);

    // Set default trusted comment
    set_default_trusted_comment(sig.trusted);

    {
        OUTCOME_TRY(data, read_to_memory(file));

        if (crypto_sign_ed25519_detached(sig.sig.data(), nullptr, data.data(),
                                         data.size(), key.key->data()) != 0) {
            return Error::ComputeSigFailed;
        }
    }

    OUTCOME_TRYV(update_global_signature(sig, key));

    return std::move(sig);
}

/*!
 * \brief Verify signature of file
 *
 * \param file Input file
 * \param sig Signature to verify
 * \param key Public key to verify signature against
 *
 * \return
 *   * Nothing if successful
 *   * Error::MismatchedKey if the key ID in \p sig does not match the key ID of
 *     \p key
 *   * Error::SignatureVerifyFailed if signature verification fails
 *   * Otherwise, a specific I/O error code
 */
oc::result<void>
verify_file(File &file, const Signature &sig, const PublicKey &key) noexcept
{
    if (key.id != sig.id) {
        return Error::MismatchedKey;
    }

    {
        OUTCOME_TRY(data, read_to_memory(file));

        if (crypto_sign_ed25519_verify_detached(
                sig.sig.data(), data.data(), data.size(), key.key.data()) != 0) {
            return Error::SignatureVerifyFailed;
        }
    }

    return verify_global_signature(sig, key);
}

/*!
 * \brief Update Signature::global_sig
 *
 * This function must be called if the trusted comment is updated. The
 * \p global_sig is a signature of the file signature concatenated with the
 * trusted comment.
 *
 * \param[in,out] sig Signature object to update \p global_sig field
 * \param[in] key Secret key to use for signing
 *
 * \return
 *   * Nothing if successful
 *   * Error::ComputeSigFailed if signature computation fails
 */
oc::result<void>
update_global_signature(Signature &sig, const SecretKey &key) noexcept
{
    GlobalSigPayload payload;
    payload.sig = sig.sig;
    payload.trusted = sig.trusted;

    auto trusted_size = strlen(sig.trusted.data());

    if (crypto_sign_ed25519_detached(sig.global_sig.data(), nullptr,
                                     reinterpret_cast<unsigned char *>(&payload),
                                     sizeof(payload.sig) + trusted_size,
                                     key.key->data()) != 0) {
        return Error::ComputeSigFailed;
    }

    return oc::success();
}

/*!
 * \brief Verify Signature::global_sig
 *
 * \param sig Signature object to verify \p global_sig field
 * \param key Public key to verify signature against
 *
 * \return
 *   * Nothing if successful
 *   * Error::SignatureVerifyFailed if signature verification fails
 */
oc::result<void>
verify_global_signature(const Signature &sig, const PublicKey &key) noexcept
{
    GlobalSigPayload payload;
    payload.sig = sig.sig;
    payload.trusted = sig.trusted;

    auto trusted_size = strlen(sig.trusted.data());

    if (crypto_sign_ed25519_verify_detached(
            sig.global_sig.data(), reinterpret_cast<unsigned char *>(&payload),
            sizeof(payload.sig) + trusted_size, key.key.data()) != 0) {
        return Error::SignatureVerifyFailed;
    }

    return oc::success();
}

}
