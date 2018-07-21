/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "util/signature.h"

#include <cstdlib>
#include <cstring>

#include <getopt.h>

#ifdef __clang__
#  pragma GCC diagnostic push
#  if __has_warning("-Wold-style-cast")
#    pragma GCC diagnostic ignored "-Wold-style-cast"
#  endif
#endif

#include <openssl/err.h>
#include <openssl/x509.h>

#ifdef __clang__
#  pragma GCC diagnostic pop
#endif

#include "mblog/logging.h"
#include "mbsign/sign.h"

#include "util/validcerts.h"

#define LOG_TAG "mbtool/util/signature"

#define COMPILE_ERROR_STRINGS 0

using ScopedBIO = std::unique_ptr<BIO, decltype(BIO_free) *>;
using mb::sign::ScopedEVP_PKEY;
using ScopedX509 = std::unique_ptr<X509, decltype(X509_free) *>;

namespace mb
{

static inline bool hex2num(char c, char *out)
{
    if (c >= 'a' && c <= 'f') {
        *out = c - 'a' + 10;
    } else if (c >= 'A' && c <= 'F') {
        *out = c - 'A' + 10;
    } else if (c >= '0' && c <= '9') {
        *out = c - '0';
    } else {
        return false;
    }
    return true;
}

static inline bool hex2bin(const std::string &source, std::string &out)
{
    std::string result;
    result.reserve((source.size() + 1) / 2);

    char temp1;
    char temp2;

    if (source.size() % 2 == 1) {
        return false;
    }

    for (size_t i = 0; i < source.size(); i += 2) {
        if (!hex2num(source[i], &temp1) || !hex2num(source[i + 1], &temp2)) {
            return false;
        }
        result += static_cast<char>(temp1 << 4 | temp2);
    }

    out.swap(result);
    return true;
}

static int log_callback(const char *str, size_t len, void *userdata)
{
    (void) userdata;

    // Strip newline
    LOGE("%s", std::string(str, len > 0 ? len - 1 : len).c_str());

    return static_cast<int>(len);
}

static void openssl_log_errors()
{
    ERR_print_errors_cb(&log_callback, nullptr);
}

static SigVerifyResult verify_signature_with_key(const char *path,
                                                 const char *sig_path,
                                                 EVP_PKEY &public_key)
{
    ScopedBIO bio_data_in(BIO_new_file(path, "rb"), BIO_free);
    if (!bio_data_in) {
        LOGE("%s: Failed to open input file", path);
        openssl_log_errors();
        return SigVerifyResult::Failure;
    }

    ScopedBIO bio_sig_in(BIO_new_file(sig_path, "rb"), BIO_free);
    if (!bio_sig_in) {
        LOGE("%s: Failed to open signature file", sig_path);
        openssl_log_errors();
        return SigVerifyResult::Failure;
    }

    auto ret = sign::verify_data(*bio_data_in, *bio_sig_in, public_key);
    if (!ret) {
        if (ret.error().ec == sign::Error::BadSignature) {
            return SigVerifyResult::Invalid;
        } else {
            LOGE("%s: Failed to verify signature: %s", sig_path,
                 ret.error().ec.message().c_str());
            if (ret.error().has_openssl_error) {
                openssl_log_errors();
            }
            return SigVerifyResult::Failure;
        }
    }

    return SigVerifyResult::Valid;
}

SigVerifyResult verify_signature(const char *path, const char *sig_path)
{
    for (const std::string &hex_der : valid_certs) {
        std::string der;
        if (!hex2bin(hex_der, der)) {
            LOGE("Failed to convert hex-encoded certificate to binary: %s",
                 hex_der.c_str());
            return SigVerifyResult::Failure;
        }

        // Cast to (void *) is okay since BIO_new_mem_buf() creates a read-only
        // BIO object
        ScopedBIO bio_x509_cert(BIO_new_mem_buf(
                der.data(), static_cast<int>(der.size())), BIO_free);
        if (!bio_x509_cert) {
            LOGE("Failed to create BIO for X509 certificate: %s",
                 hex_der.c_str());
            openssl_log_errors();
            return SigVerifyResult::Failure;
        }

        // Load DER-encoded certificate
        ScopedX509 cert(d2i_X509_bio(bio_x509_cert.get(), nullptr), X509_free);
        if (!cert) {
            LOGE("Failed to load X509 certificate: %s", hex_der.c_str());
            openssl_log_errors();
            return SigVerifyResult::Failure;
        }

        // Get public key from certificate
        ScopedEVP_PKEY public_key(X509_get_pubkey(cert.get()), EVP_PKEY_free);
        if (!public_key) {
            LOGE("Failed to load public key from X509 certificate: %s",
                 hex_der.c_str());
            openssl_log_errors();
            return SigVerifyResult::Failure;
        }

        SigVerifyResult result =
                verify_signature_with_key(path, sig_path, *public_key);
        if (result == SigVerifyResult::Invalid) {
            // Keep trying ...
            continue;
        }

        return result;
    }

    return SigVerifyResult::Invalid;
}

static void sigverify_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: sigverify [option...] <input file> <signature file>\n\n"
            "Options:\n"
            "  -h, --help       Display this help message\n"
            "\n"
            "Exit codes:\n"
            "  0: Signature is valid\n"
            "  1: An error occurred while checking the signature\n"
            "  2: Signature is invalid\n");
}

//#define EXIT_SUCCESS
//#define EXIT_FAILURE
#define EXIT_INVALID 2

int sigverify_main(int argc, char *argv[])
{
    int opt;

    static struct option long_options[] = {
        {"help",      no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "h", long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            sigverify_usage(stdout);
            return EXIT_SUCCESS;

        default:
            sigverify_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind != 2) {
        sigverify_usage(stderr);
        return EXIT_FAILURE;
    }

#if COMPILE_ERROR_STRINGS
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();
#endif

    const char *path = argv[optind];
    const char *sig_path = argv[optind + 1];

    SigVerifyResult result = verify_signature(path, sig_path);

    switch (result) {
    case SigVerifyResult::Valid:
        return EXIT_SUCCESS;
    case SigVerifyResult::Invalid:
        return EXIT_INVALID;
    case SigVerifyResult::Failure:
    default:
        return EXIT_FAILURE;
    }
}

}
