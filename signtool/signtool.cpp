/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <openssl/err.h>

// libmbsign
#include "mbsign/mbsign.h"

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
    const char *file_pkcs12;
    const char *file_input;
    const char *file_output;
    EVP_PKEY *private_key = nullptr;
    const char *pass;
    BIO *bio_data_in = nullptr;
    BIO *bio_sig_out = nullptr;
    bool ret;

    ERR_load_crypto_strings();
    OpenSSL_add_all_algorithms();

    if (argc != 4) {
        usage(stderr);
        return EXIT_FAILURE;
    }

    file_pkcs12 = argv[1];
    file_input = argv[2];
    file_output = argv[3];

    pass = getenv("MBSIGN_PASSPHRASE");
    if (!pass) {
        fprintf(stderr,
                "The MBSIGN_PASSPHRASE environment variable is not set\n");
        goto error;
    }

    private_key = mb::sign::load_private_key_from_file(
            file_pkcs12, mb::sign::KEY_FORMAT_PKCS12, pass);
    if (!private_key) {
        goto error;
    }

    bio_data_in = BIO_new_file(file_input, "rb");
    if (!bio_data_in) {
        fprintf(stderr, "%s: Failed to open input file\n", file_input);
        openssl_log_errors();
        goto error;
    }
    bio_sig_out = BIO_new_file(file_output, "wb");
    if (!bio_sig_out) {
        fprintf(stderr, "%s: Failed to open output file\n", file_output);
        openssl_log_errors();
        goto error;
    }

    ret = mb::sign::sign_data(bio_data_in, bio_sig_out, private_key);

    EVP_PKEY_free(private_key);

    if (!BIO_free(bio_data_in)) {
        fprintf(stderr, "%s: Failed to close input file\n", file_input);
        openssl_log_errors();
        ret = false;
    }
    if (!BIO_free(bio_sig_out)) {
        fprintf(stderr, "%s: Failed to close output file\n", file_output);
        openssl_log_errors();
        ret = false;
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;

error:
    EVP_PKEY_free(private_key);
    BIO_free(bio_data_in);
    BIO_free(bio_sig_out);
    return EXIT_FAILURE;
}
