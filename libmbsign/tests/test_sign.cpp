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

#include <gtest/gtest.h>

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include "mblog/logging.h"
#include "mbsign/mbsign.h"

static int log_callback(const char *str, size_t len, void *userdata)
{
    (void) userdata;
    char *copy = strdup(str);
    if (copy) {
        // Strip newline
        copy[len - 1] = '\0';
        LOGE("%s", copy);
        free(copy);
    }
    return len;
}

static void openssl_log_errors()
{
    ERR_print_errors_cb(&log_callback, nullptr);
}

static bool generate_keys(EVP_PKEY **private_key_out, EVP_PKEY **public_key_out)
{
    assert(private_key_out && public_key_out);

    EVP_PKEY *private_key = nullptr;
    EVP_PKEY *public_key = nullptr;
    RSA *rsa = nullptr;

    private_key = EVP_PKEY_new();
    if (!private_key) {
        LOGE("EVP_PKEY_new() failed for private key");
        openssl_log_errors();
        goto error;
    }

    public_key = EVP_PKEY_new();
    if (!public_key) {
        LOGE("EVP_PKEY_new() failed for public key");
        openssl_log_errors();
        goto error;
    }

    rsa = RSA_generate_key(2048, RSA_F4, nullptr, nullptr);
    if (!rsa) {
        LOGE("RSA_generate_key() failed");
        openssl_log_errors();
        goto error;
    }

    if (!EVP_PKEY_assign_RSA(private_key, RSAPrivateKey_dup(rsa))) {
        LOGE("EVP_PKEY_assign_RSA() failed for private key");
        openssl_log_errors();
        goto error;
    }

    if (!EVP_PKEY_assign_RSA(public_key, RSAPublicKey_dup(rsa))) {
        LOGE("EVP_PKEY_assign_RSA() failed for public key");
        openssl_log_errors();
        goto error;
    }

    *private_key_out = private_key;
    *public_key_out = public_key;
    RSA_free(rsa);
    return true;

error:
    EVP_PKEY_free(private_key);
    EVP_PKEY_free(public_key);
    RSA_free(rsa);
    return false;
}

TEST(SignTest, TestLoadInvalidPemKeys)
{
    BIO *bio_private_key;
    BIO *bio_public_key;
    EVP_PKEY *pkey;

    bio_private_key = BIO_new(BIO_s_mem());
    ASSERT_NE(bio_private_key, nullptr);
    bio_public_key = BIO_new(BIO_s_mem());
    ASSERT_NE(bio_public_key, nullptr);

    // Fill with garbage
    ASSERT_EQ(BIO_write(bio_private_key, "abcdefghijklmnopqrstuvwxyz", 26), 26);
    ASSERT_EQ(BIO_write(bio_public_key, "zyxwvutsrqponmlkjihgfedcba", 26), 26);

    pkey = mb::sign::load_private_key(
            bio_private_key, mb::sign::KEY_FORMAT_PEM, nullptr);
    ASSERT_EQ(pkey, nullptr);
    pkey = mb::sign::load_public_key(
            bio_public_key, mb::sign::KEY_FORMAT_PEM, nullptr);
    ASSERT_EQ(pkey, nullptr);

    BIO_free(bio_private_key);
    BIO_free(bio_public_key);
}

TEST(SignTest, TestLoadInvalidPkcs12PrivateKey)
{
    BIO *bio_private_key;
    BIO *bio_public_key;
    EVP_PKEY *pkey;

    bio_private_key = BIO_new(BIO_s_mem());
    ASSERT_NE(bio_private_key, nullptr);
    bio_public_key = BIO_new(BIO_s_mem());
    ASSERT_NE(bio_public_key, nullptr);

    // Fill with garbage
    ASSERT_EQ(BIO_write(bio_private_key, "abcdefghijklmnopqrstuvwxyz", 26), 26);
    ASSERT_EQ(BIO_write(bio_public_key, "zyxwvutsrqponmlkjihgfedcba", 26), 26);

    pkey = mb::sign::load_private_key(
            bio_private_key, mb::sign::KEY_FORMAT_PKCS12, nullptr);
    ASSERT_EQ(pkey, nullptr);
    pkey = mb::sign::load_public_key(
            bio_public_key, mb::sign::KEY_FORMAT_PKCS12, nullptr);
    ASSERT_EQ(pkey, nullptr);

    BIO_free(bio_private_key);
    BIO_free(bio_public_key);
}

TEST(SignTest, TestLoadValidPemKeys)
{
    EVP_PKEY *private_key;
    EVP_PKEY *public_key;
    EVP_PKEY *private_key_enc_read;
    EVP_PKEY *private_key_noenc_read;
    EVP_PKEY *public_key_read;
    BIO *bio_private_key_enc;
    BIO *bio_private_key_noenc;
    BIO *bio_public_key;

    // Generate keys
    ASSERT_TRUE(generate_keys(&private_key, &public_key));

    // Create memory buffers
    bio_private_key_enc = BIO_new(BIO_s_mem());
    ASSERT_NE(bio_private_key_enc, nullptr);
    bio_private_key_noenc = BIO_new(BIO_s_mem());
    ASSERT_NE(bio_private_key_noenc, nullptr);
    bio_public_key = BIO_new(BIO_s_mem());
    ASSERT_NE(bio_public_key, nullptr);

    // Write keys
    ASSERT_TRUE(PEM_write_bio_PrivateKey(bio_private_key_enc, private_key,
                                         EVP_des_ede3_cbc(), nullptr, 0,
                                         nullptr, (void *) "testing"));
    ASSERT_TRUE(PEM_write_bio_PrivateKey(bio_private_key_noenc, private_key,
                                         nullptr, nullptr, 0, nullptr,
                                         nullptr));
    ASSERT_TRUE(PEM_write_bio_PUBKEY(bio_public_key, public_key));

    // Read back the keys
    private_key_enc_read = mb::sign::load_private_key(
            bio_private_key_enc, mb::sign::KEY_FORMAT_PEM, "testing");
    ASSERT_NE(private_key_enc_read, nullptr);
    private_key_noenc_read = mb::sign::load_private_key(
            bio_private_key_noenc, mb::sign::KEY_FORMAT_PEM, nullptr);
    ASSERT_NE(private_key_noenc_read, nullptr);
    public_key_read = mb::sign::load_public_key(
            bio_public_key, mb::sign::KEY_FORMAT_PEM, "testing");
    ASSERT_NE(public_key_read, nullptr);

    // Compare keys
    ASSERT_EQ(EVP_PKEY_cmp(private_key, private_key_enc_read), 1);
    ASSERT_EQ(EVP_PKEY_cmp(private_key, private_key_noenc_read), 1);
    ASSERT_EQ(EVP_PKEY_cmp(public_key, public_key_read), 1);

    EVP_PKEY_free(private_key);
    EVP_PKEY_free(public_key);
    EVP_PKEY_free(private_key_enc_read);
    EVP_PKEY_free(private_key_noenc_read);
    EVP_PKEY_free(public_key_read);
    BIO_free(bio_private_key_enc);
    BIO_free(bio_private_key_noenc);
    BIO_free(bio_public_key);
}

TEST(SignTest, TestLoadValidPemKeysWithInvalidPassphrase)
{
    EVP_PKEY *private_key;
    EVP_PKEY *public_key;
    EVP_PKEY *private_key_read;
    BIO *bio;

    // Generate keys
    ASSERT_TRUE(generate_keys(&private_key, &public_key));

    // Create memory buffers
    bio = BIO_new(BIO_s_mem());
    ASSERT_NE(bio, nullptr);

    // Write key
    ASSERT_TRUE(PEM_write_bio_PrivateKey(bio, private_key, EVP_des_ede3_cbc(),
                                         nullptr, 0, nullptr,
                                         (void *) "testing"));

    // Read back the key using invalid password
    private_key_read = mb::sign::load_private_key(
            bio, mb::sign::KEY_FORMAT_PEM, "gnitset");
    ASSERT_EQ(private_key_read, nullptr);

    EVP_PKEY_free(private_key);
    EVP_PKEY_free(public_key);
    EVP_PKEY_free(private_key_read);
    BIO_free(bio);
}
