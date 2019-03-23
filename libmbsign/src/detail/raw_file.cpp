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

#include "mbsign/detail/raw_file.h"

#include <memory>

#include <cstring>

#include "mbcommon/file/buffered.h"
#include "mbcommon/file_util.h"
#include "mbcommon/string.h"

#include "mbsign/detail/file_format.h"
#include "mbsign/error.h"

namespace mb::sign::detail
{

/*!
 * \brief Save raw minisign-formatted file (secret key, public key, or signature)
 *
 * The trusted comment will be written only if \p trusted and \p global_sig are
 * not null.
 *
 * \param file Input file
 * \param payload Data payload
 * \param untrusted Untrusted comment
 * \param trusted Trusted comment (optional)
 * \param global_sig Global signature (optional)
 *
 * \return
 *   * Nothing if successful
 *   * `std::errc::invalid_argument` if only one of \p trusted and \p global_sig
 *     is null
 *   * `std::errc::not_enough_memory` if memory allocation fails
 *   * Error::InvalidUntrustedComment if the untrusted comment contains newlines
 *   * Error::InvalidTrustedComment if the trusted comment contains newlines
 *   * Otherwise, a specific I/O error code
 */
oc::result<void>
save_raw_file(File &file, span<const unsigned char> payload,
              const UntrustedComment &untrusted,
              const TrustedComment * const trusted,
              const RawSignature * const global_sig) noexcept
{
    if (!!trusted != !!global_sig) {
        return std::errc::invalid_argument;
    }

    if (strpbrk(untrusted.data(), "\r\n")) {
        return Error::InvalidUntrustedComment;
    }

    if (trusted && strpbrk(trusted->data(), "\r\n")) {
        return Error::InvalidTrustedComment;
    }

    BufferedFile buf_file;

    OUTCOME_TRYV(buf_file.open(file));

    // Write untrusted comment
    {
        OUTCOME_TRYV(file_write_exact(buf_file, as_uchars(UNTRUSTED_PREFIX,
                                      strlen(UNTRUSTED_PREFIX))));
        OUTCOME_TRYV(file_write_exact(buf_file, as_uchars(untrusted.data(),
                                      strlen(untrusted.data()))));
        OUTCOME_TRYV(file_write_exact(buf_file, "\n"_uchars));
    }

    // Write base64-encoded payload
    {
        size_t base64_max_size = sodium_base64_ENCODED_LEN(
                payload.size(), sodium_base64_VARIANT_ORIGINAL);
        std::unique_ptr<char[]> base64_payload(
                new(std::nothrow) char[base64_max_size]);
        if (!base64_payload) {
            return std::errc::not_enough_memory;
        }

        sodium_bin2base64(base64_payload.get(), base64_max_size,
                          reinterpret_cast<const unsigned char *>(payload.data()),
                          payload.size(), sodium_base64_VARIANT_ORIGINAL);

        // sodium_base64_ENCODED_LEN() includes space for a trailing NULL byte
        // and sodium_bin2base64() fills extra bytes in the buffer with zeros.
        OUTCOME_TRYV(file_write_exact(buf_file, as_uchars(base64_payload.get(),
                                      strlen(base64_payload.get()))));
        OUTCOME_TRYV(file_write_exact(buf_file, "\n"_uchars));
    }

    // Write trusted comment
    if (trusted) {
        OUTCOME_TRYV(file_write_exact(buf_file, as_uchars(TRUSTED_PREFIX,
                                      strlen(TRUSTED_PREFIX))));
        OUTCOME_TRYV(file_write_exact(buf_file, as_uchars(trusted->data(),
                                      strlen(trusted->data()))));
        OUTCOME_TRYV(file_write_exact(buf_file, "\n"_uchars));
    }

    // Write base64-encoded signature of signature and trusted comment
    if (global_sig) {
        RawSignatureBase64 base64_raw_sig;

        sodium_bin2base64(base64_raw_sig.data(), base64_raw_sig.size(),
                          global_sig->data(), global_sig->size(),
                          sodium_base64_VARIANT_ORIGINAL);

        // sodium_base64_ENCODED_LEN() includes space for a trailing NULL byte
        // and sodium_bin2base64() fills extra bytes in the buffer with zeros.
        OUTCOME_TRYV(file_write_exact(buf_file, as_uchars(base64_raw_sig.data(),
                                      strlen(base64_raw_sig.data()))));
        OUTCOME_TRYV(file_write_exact(buf_file, "\n"_uchars));
    }

    OUTCOME_TRYV(buf_file.close());

    return oc::success();
}

/*!
 * \brief Load raw minisign-formatted file (secret key, public key, or signature)
 *
 * The trusted comment will be read only if \p trusted and \p global_sig are not
 * null.
 *
 * \param[in] file Input file
 * \param[out] payload Span to store data payload
 * \param[out] untrusted Reference to store untrusted comment
 * \param[out] trusted Pointer to store trusted comment (optional)
 * \param[out] global_sig Pointer to store global signature (optional)
 *
 * \return
 *   * Nothing if successful
 *   * `std::errc::invalid_argument` if only one of \p trusted and \p global_sig
 *     is null
 *   * `std::errc::not_enough_memory` if memory allocation fails
 *   * Error::InvalidUntrustedComment if the untrusted comment is too long
 *   * Error::InvalidTrustedComment if the trusted comment is too long
 *   * Error::Base64DecodeError if invalid base64 data is encountered
 *   * Error::InvalidPayloadSize if the payload size is incorrect
 *   * Error::InvalidGlobalSigSize if the global signature size is incorrect
 *   * Otherwise, a specific I/O error code
 */
oc::result<void>
load_raw_file(File &file, span<unsigned char> payload,
              UntrustedComment &untrusted, TrustedComment *trusted,
              RawSignature *global_sig) noexcept
{
    if (!!trusted != !!global_sig) {
        return std::errc::invalid_argument;
    }

    BufferedFile buf_file;

    OUTCOME_TRYV(buf_file.open(file));

    // Read untrusted comment
    {
        unsigned char prefix[sizeof(UNTRUSTED_PREFIX) - 1];
        OUTCOME_TRY(file_read_exact(buf_file, prefix));

        if (memcmp(prefix, UNTRUSTED_PREFIX, sizeof(prefix)) != 0) {
            return Error::InvalidUntrustedComment;
        }

        OUTCOME_TRY(n, buf_file.read_sized_line(as_writable_uchars(span(
                untrusted.data(), untrusted.size() - 1))));

        std::string_view sv(untrusted.data(), n);
        sv = trimmed_right(sv);
        untrusted[sv.size()] = '\0';
    }

    // Read base64-encoded payload
    {
        size_t base64_max_size = sodium_base64_ENCODED_LEN(
                payload.size(), sodium_base64_VARIANT_ORIGINAL);
        std::unique_ptr<char[]> base64_payload(
                new(std::nothrow) char[base64_max_size]);
        if (!base64_payload) {
            return std::errc::not_enough_memory;
        }

        // The array has enough room for a null byte, which may be used to store
        // a newline in our case.
        OUTCOME_TRY(n, buf_file.read_sized_line(as_writable_uchars(
                span(base64_payload.get(), base64_max_size))));

        size_t bin_size;
        if (sodium_base642bin(reinterpret_cast<unsigned char *>(payload.data()),
                              payload.size(), base64_payload.get(), n, " \r\n",
                              &bin_size, nullptr,
                              sodium_base64_VARIANT_ORIGINAL) != 0) {
            return Error::Base64DecodeError;
        }

        if (bin_size != payload.size()) {
            return Error::InvalidPayloadSize;
        }
    }

    // Read trusted comment
    if (trusted) {
        unsigned char prefix[sizeof(TRUSTED_PREFIX) - 1];
        OUTCOME_TRY(file_read_exact(buf_file, prefix));

        if (memcmp(prefix, TRUSTED_PREFIX, sizeof(prefix)) != 0) {
            return Error::InvalidTrustedComment;
        }

        OUTCOME_TRY(n, buf_file.read_sized_line(as_writable_uchars(span(
                trusted->data(), trusted->size() - 1))));

        std::string_view sv(trusted->data(), n);
        sv = trimmed_right(sv);
        (*trusted)[sv.size()] = '\0';
    }

    // Read base64-encoded signature of signature and trusted comment
    if (global_sig) {
        RawSignatureBase64 base64_raw_sig;

        OUTCOME_TRY(n, buf_file.read_sized_line(
                as_writable_uchars(base64_raw_sig)));

        size_t bin_size;
        if (sodium_base642bin(reinterpret_cast<unsigned char *>(global_sig),
                              sizeof(*global_sig), base64_raw_sig.data(), n,
                              " \r\n", &bin_size, nullptr,
                              sodium_base64_VARIANT_ORIGINAL) != 0) {
            return Error::Base64DecodeError;
        }

        if (bin_size != sizeof(*global_sig)) {
            return Error::InvalidGlobalSigSize;
        }
    }

    return oc::success();
}

}
