/*
 * Copyright (C) 2016-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbsign/sign.h"

using ScopedBIGNUM = std::unique_ptr<BIGNUM, decltype(BN_free) *>;
using ScopedBIO = std::unique_ptr<BIO, decltype(BIO_free) *>;
using ScopedRSA = std::unique_ptr<RSA, decltype(RSA_free) *>;

using namespace mb::sign;

static void generate_keys(ScopedEVP_PKEY &private_key_out,
                          ScopedEVP_PKEY &public_key_out)
{
    ScopedEVP_PKEY private_key(EVP_PKEY_new(), EVP_PKEY_free);
    ScopedEVP_PKEY public_key(EVP_PKEY_new(), EVP_PKEY_free);
    ScopedRSA rsa(RSA_new(), RSA_free);
    ScopedBIGNUM e(BN_new(), BN_free);

    ASSERT_TRUE(!!private_key) << "Failed to allocate private key";
    ASSERT_TRUE(!!public_key) << "Failed to allocate public key";
    ASSERT_TRUE(!!rsa) << "Failed to allocate RSA";
    ASSERT_TRUE(!!e) << "Failed to allocate BIGNUM";

    BN_set_word(e.get(), RSA_F4);

    ASSERT_TRUE(RSA_generate_key_ex(rsa.get(), 2048, e.get(), nullptr))
            << "RSA_generate_key_ex() failed";

    ASSERT_TRUE(EVP_PKEY_assign_RSA(private_key.get(),
                                    RSAPrivateKey_dup(rsa.get())))
            << "EVP_PKEY_assign_RSA() failed for private key";

    ASSERT_TRUE(EVP_PKEY_assign_RSA(public_key.get(),
                                    RSAPublicKey_dup(rsa.get())))
            << "EVP_PKEY_assign_RSA() failed for public key";

    private_key_out = std::move(private_key);
    public_key_out = std::move(public_key);
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

    auto private_key = load_private_key(
            *bio_private_key, KeyFormat::Pem, nullptr);
    ASSERT_FALSE(private_key);

    auto public_key = load_public_key(
            *bio_public_key, KeyFormat::Pem, nullptr);
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

    auto private_key = load_private_key(
            *bio_private_key, KeyFormat::Pkcs12, nullptr);
    ASSERT_FALSE(private_key);

    auto public_key = load_public_key(
            *bio_public_key, KeyFormat::Pkcs12, nullptr);
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
    generate_keys(private_key, public_key);

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
    auto private_key_enc_read = load_private_key(
            *bio_private_key_enc, KeyFormat::Pem, "testing");
    ASSERT_TRUE(!!private_key_enc_read);
    auto private_key_noenc_read = load_private_key(
            *bio_private_key_noenc, KeyFormat::Pem, nullptr);
    ASSERT_TRUE(!!private_key_noenc_read);
    auto public_key_read = load_public_key(
            *bio_public_key, KeyFormat::Pem, "testing");
    ASSERT_TRUE(!!public_key_read);

    // Compare keys
    EXPECT_EQ(EVP_PKEY_cmp(private_key.get(),
                           private_key_enc_read.value().get()), 1);
    EXPECT_EQ(EVP_PKEY_cmp(private_key.get(),
                           private_key_noenc_read.value().get()), 1);
    EXPECT_EQ(EVP_PKEY_cmp(public_key.get(),
                           public_key_read.value().get()), 1);
}

TEST(SignTest, TestLoadValidPemKeysWithInvalidPassphrase)
{
    ScopedEVP_PKEY private_key(nullptr, EVP_PKEY_free);
    ScopedEVP_PKEY public_key(nullptr, EVP_PKEY_free);

    ScopedBIO bio(BIO_new(BIO_s_mem()), BIO_free);
    ASSERT_TRUE(!!bio);

    // Generate keys
    generate_keys(private_key, public_key);

    // Write key
    ASSERT_TRUE(PEM_write_bio_PrivateKey(bio.get(), private_key.get(),
                                         EVP_des_ede3_cbc(), nullptr, 0,
                                         nullptr,
                                         const_cast<char *>("testing")));

    // Read back the key using invalid password
    auto private_key_read = load_private_key(*bio, KeyFormat::Pem, "gnitset");
    ASSERT_FALSE(private_key_read);
}
