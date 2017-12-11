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

#include <gtest/gtest.h>

#include <memory>

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include "mblog/logging.h"
#include "mbsign/mbsign.h"

#define LOG_TAG "test_sign"

using ScopedBIGNUM = std::unique_ptr<BIGNUM, decltype(BN_free) *>;
using ScopedBIO = std::unique_ptr<BIO, decltype(BIO_free) *>;
using ScopedEVP_PKEY = std::unique_ptr<EVP_PKEY, decltype(EVP_PKEY_free) *>;
using ScopedRSA = std::unique_ptr<RSA, decltype(RSA_free) *>;

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

static bool generate_keys(ScopedEVP_PKEY &private_key_out,
                          ScopedEVP_PKEY &public_key_out)
{
    ScopedEVP_PKEY private_key(EVP_PKEY_new(), EVP_PKEY_free);
    ScopedEVP_PKEY public_key(EVP_PKEY_new(), EVP_PKEY_free);
    ScopedRSA rsa(RSA_new(), RSA_free);
    ScopedBIGNUM e(BN_new(), BN_free);

    if (!private_key) {
        LOGE("Failed to allocate private key");
        openssl_log_errors();
        return false;
    }

    if (!public_key) {
        LOGE("Failed to allocate public key");
        openssl_log_errors();
        return false;
    }

    if (!rsa) {
        LOGE("Failed to allocate RSA");
        openssl_log_errors();
        return false;
    }

    if (!e) {
        LOGE("Failed to allocate BIGNUM");
        openssl_log_errors();
        return false;
    }

    BN_set_word(e.get(), RSA_F4);

    if (RSA_generate_key_ex(rsa.get(), 2048, e.get(), nullptr) < 0) {
        LOGE("RSA_generate_key_ex() failed");
        openssl_log_errors();
        return false;
    }

    if (!EVP_PKEY_assign_RSA(private_key.get(), RSAPrivateKey_dup(rsa.get()))) {
        LOGE("EVP_PKEY_assign_RSA() failed for private key");
        openssl_log_errors();
        return false;
    }

    if (!EVP_PKEY_assign_RSA(public_key.get(), RSAPublicKey_dup(rsa.get()))) {
        LOGE("EVP_PKEY_assign_RSA() failed for public key");
        openssl_log_errors();
        return false;
    }

    private_key_out = std::move(private_key);
    public_key_out = std::move(public_key);
    return true;
}

TEST(SignTest, TestLoadInvalidPemKeys)
{
    ScopedBIO bio_private_key(BIO_new(BIO_s_mem()), BIO_free);
    ASSERT_TRUE(!!bio_private_key);
    ScopedBIO bio_public_key(BIO_new(BIO_s_mem()), BIO_free);
    ASSERT_TRUE(!!bio_public_key);

    // Fill with garbage
    ASSERT_EQ(BIO_write(bio_private_key.get(),
                        "abcdefghijklmnopqrstuvwxyz", 26), 26);
    ASSERT_EQ(BIO_write(bio_public_key.get(),
                        "zyxwvutsrqponmlkjihgfedcba", 26), 26);

    ScopedEVP_PKEY private_key(mb::sign::load_private_key(
            bio_private_key.get(), mb::sign::KEY_FORMAT_PEM, nullptr),
            EVP_PKEY_free);
    ASSERT_FALSE(private_key);

    ScopedEVP_PKEY public_key(mb::sign::load_public_key(
            bio_public_key.get(), mb::sign::KEY_FORMAT_PEM, nullptr),
            EVP_PKEY_free);
    ASSERT_FALSE(public_key);
}

TEST(SignTest, TestLoadInvalidPkcs12PrivateKey)
{
    ScopedBIO bio_private_key(BIO_new(BIO_s_mem()), BIO_free);
    ASSERT_TRUE(!!bio_private_key);
    ScopedBIO bio_public_key(BIO_new(BIO_s_mem()), BIO_free);
    ASSERT_TRUE(!!bio_public_key);

    // Fill with garbage
    ASSERT_EQ(BIO_write(bio_private_key.get(),
                        "abcdefghijklmnopqrstuvwxyz", 26), 26);
    ASSERT_EQ(BIO_write(bio_public_key.get(),
                        "zyxwvutsrqponmlkjihgfedcba", 26), 26);

    ScopedEVP_PKEY private_key(mb::sign::load_private_key(
            bio_private_key.get(), mb::sign::KEY_FORMAT_PKCS12, nullptr),
            EVP_PKEY_free);
    ASSERT_FALSE(private_key);

    ScopedEVP_PKEY public_key(mb::sign::load_public_key(
            bio_public_key.get(), mb::sign::KEY_FORMAT_PKCS12, nullptr),
            EVP_PKEY_free);
    ASSERT_FALSE(public_key);
}

TEST(SignTest, TestLoadValidPemKeys)
{
    ScopedEVP_PKEY private_key(nullptr, EVP_PKEY_free);
    ScopedEVP_PKEY public_key(nullptr, EVP_PKEY_free);

    ScopedBIO bio_private_key_enc(BIO_new(BIO_s_mem()), BIO_free);
    ASSERT_TRUE(!!bio_private_key_enc);
    ScopedBIO bio_private_key_noenc(BIO_new(BIO_s_mem()), BIO_free);
    ASSERT_TRUE(!!bio_private_key_noenc);
    ScopedBIO bio_public_key(BIO_new(BIO_s_mem()), BIO_free);
    ASSERT_TRUE(!!bio_public_key);

    // Generate keys
    ASSERT_TRUE(generate_keys(private_key, public_key));

    // Write keys
    ASSERT_TRUE(PEM_write_bio_PrivateKey(bio_private_key_enc.get(),
                                         private_key.get(), EVP_des_ede3_cbc(),
                                         nullptr, 0, nullptr,
                                         const_cast<char *>("testing")));
    ASSERT_TRUE(PEM_write_bio_PrivateKey(bio_private_key_noenc.get(),
                                         private_key.get(), nullptr, nullptr, 0,
                                         nullptr, nullptr));
    ASSERT_TRUE(PEM_write_bio_PUBKEY(bio_public_key.get(), public_key.get()));

    // Read back the keys
    ScopedEVP_PKEY private_key_enc_read(mb::sign::load_private_key(
            bio_private_key_enc.get(), mb::sign::KEY_FORMAT_PEM, "testing"),
            EVP_PKEY_free);
    ASSERT_TRUE(!!private_key_enc_read);
    ScopedEVP_PKEY private_key_noenc_read(mb::sign::load_private_key(
            bio_private_key_noenc.get(), mb::sign::KEY_FORMAT_PEM, nullptr),
            EVP_PKEY_free);
    ASSERT_TRUE(!!private_key_noenc_read);
    ScopedEVP_PKEY public_key_read(mb::sign::load_public_key(
            bio_public_key.get(), mb::sign::KEY_FORMAT_PEM, "testing"),
            EVP_PKEY_free);
    ASSERT_TRUE(!!public_key_read);

    // Compare keys
    ASSERT_EQ(EVP_PKEY_cmp(private_key.get(), private_key_enc_read.get()), 1);
    ASSERT_EQ(EVP_PKEY_cmp(private_key.get(), private_key_noenc_read.get()), 1);
    ASSERT_EQ(EVP_PKEY_cmp(public_key.get(), public_key_read.get()), 1);
}

TEST(SignTest, TestLoadValidPemKeysWithInvalidPassphrase)
{
    ScopedEVP_PKEY private_key(nullptr, EVP_PKEY_free);
    ScopedEVP_PKEY public_key(nullptr, EVP_PKEY_free);

    ScopedBIO bio(BIO_new(BIO_s_mem()), BIO_free);
    ASSERT_TRUE(!!bio);

    // Generate keys
    ASSERT_TRUE(generate_keys(private_key, public_key));

    // Write key
    ASSERT_TRUE(PEM_write_bio_PrivateKey(bio.get(), private_key.get(),
                                         EVP_des_ede3_cbc(), nullptr, 0,
                                         nullptr,
                                         const_cast<char *>("testing")));

    // Read back the key using invalid password
    ScopedEVP_PKEY private_key_read(mb::sign::load_private_key(
            bio.get(), mb::sign::KEY_FORMAT_PEM, "gnitset"),
            EVP_PKEY_free);
    ASSERT_FALSE(private_key_read);
}
