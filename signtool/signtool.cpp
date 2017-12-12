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

#include <memory>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <openssl/err.h>

// libmbsign
#include "mbsign/mbsign.h"

using ScopedBIO = std::unique_ptr<BIO, decltype(BIO_free) *>;
using ScopedEVP_PKEY = std::unique_ptr<EVP_PKEY, decltype(EVP_PKEY_free) *>;

static void openssl_log_errors()
{
    ERR_print_errors_fp(stderr);
}

static void usage(FILE *stream)
{
    fprintf(stream,
            "Usage: signtool <PKCS12 file> <input file> <output signature file>\n\n"
            "NOTE: This is not a general purpose tool for signing files!\n"
            "It is only meant for use with mbtool.\n");
}

int main(int argc, char *argv[])
{
    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();

    if (argc != 4) {
        usage(stderr);
        return EXIT_FAILURE;
    }

    const char *file_pkcs12 = argv[1];
    const char *file_input = argv[2];
    const char *file_output = argv[3];

    const char *pass = getenv("MBSIGN_PASSPHRASE");
    if (!pass) {
        fprintf(stderr,
                "The MBSIGN_PASSPHRASE environment variable is not set\n");
        return EXIT_FAILURE;
    }

    ScopedEVP_PKEY private_key(mb::sign::load_private_key_from_file(
            file_pkcs12, mb::sign::KEY_FORMAT_PKCS12, pass), EVP_PKEY_free);
    if (!private_key) {
        return EXIT_FAILURE;
    }

    ScopedBIO bio_data_in(BIO_new_file(file_input, "rb"), BIO_free);
    if (!bio_data_in) {
        fprintf(stderr, "%s: Failed to open input file\n", file_input);
        openssl_log_errors();
        return EXIT_FAILURE;
    }
    ScopedBIO bio_sig_out(BIO_new_file(file_output, "wb"), BIO_free);
    if (!bio_sig_out) {
        fprintf(stderr, "%s: Failed to open output file\n", file_output);
        openssl_log_errors();
        return EXIT_FAILURE;
    }

    bool ret = mb::sign::sign_data(bio_data_in.get(), bio_sig_out.get(),
                                   private_key.get());

    if (!BIO_free(bio_data_in.release())) {
        fprintf(stderr, "%s: Failed to close input file\n", file_input);
        openssl_log_errors();
        ret = false;
    }

    if (!BIO_free(bio_sig_out.release())) {
        fprintf(stderr, "%s: Failed to close output file\n", file_output);
        openssl_log_errors();
        ret = false;
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
