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

#include <gtest/gtest.h>

#include <string_view>

#include <cinttypes>

#include "mbcommon/file/memory.h"
#include "mbcommon/span.h"
#include "mbcommon/string.h"

#include "mbsign/detail/file_format.h"
#include "mbsign/detail/raw_file.h"
#include "mbsign/error.h"
#include "mbsign/sign.h"

using namespace mb;
using namespace mb::sign;
using namespace mb::sign::detail;
using namespace testing;

// Make ASSERT_EQ happy
namespace mb::sign
{

static std::ostream & operator<<(std::ostream &out, const SecretKey &)
{
    return out;
}

static std::ostream & operator<<(std::ostream &out, const PublicKey &)
{
    return out;
}

static std::ostream & operator<<(std::ostream &out, const Signature &)
{
    return out;
}

}

struct SignTest : Test
{
    KeyPair _kp;
    MemoryFile _file;
    void *_file_data = nullptr;
    size_t _file_size = 0;
    MemoryFile _data_file;
    void *_data_file_data = nullptr;
    size_t _data_file_size = 0;

    ~SignTest()
    {
        free(_file_data);
        free(_data_file_data);
    }

    void SetUp() override
    {
        auto kp = generate_keypair();
        ASSERT_TRUE(kp);
        ASSERT_TRUE(kp.value().skey.key);
        _kp = std::move(kp.value());
    }

    void open_file()
    {
        ASSERT_TRUE(_file.open(&_file_data, &_file_size));
    }

    void rewind_file()
    {
        ASSERT_TRUE(_file.seek(0, SEEK_SET));
    }

    void clear_file()
    {
        rewind_file();
        ASSERT_TRUE(_file.truncate(0));
    }

    void open_data_file()
    {
        ASSERT_TRUE(_data_file.open(&_data_file_data, &_data_file_size));
    }

    void rewind_data_file()
    {
        ASSERT_TRUE(_data_file.seek(0, SEEK_SET));
    }
};

TEST_F(SignTest, GenerateKeypair)
{
    ASSERT_EQ(_kp.skey.id, _kp.pkey.id);

    auto id_str = format("%" PRIX64, _kp.skey.id);
    ASSERT_NE(std::string_view(_kp.pkey.untrusted.data()).find(id_str),
              std::string_view::npos);
    ASSERT_NE(std::string_view(_kp.skey.untrusted.data()).find(id_str),
              std::string_view::npos);
}

TEST_F(SignTest, RoundTripSecretKey)
{
    open_file();

    ASSERT_TRUE(save_secret_key(_file, _kp.skey, "test",
                                KdfSecurityLevel::Interactive));
    rewind_file();

    auto key = load_secret_key(_file, "test");
    ASSERT_TRUE(key);

    ASSERT_EQ(_kp.skey.id, key.value().id);
    ASSERT_EQ(*_kp.skey.key, *key.value().key);
    ASSERT_STREQ(_kp.skey.untrusted.data(), key.value().untrusted.data());
}

TEST_F(SignTest, LoadSecretKeyFailureUnsupportedAlgorithms)
{
    open_file();

    SKPayload payload = {};
    payload.sig_alg = SIG_ALG;
    payload.kdf_alg = KDF_ALG;
    payload.chk_alg = CHK_ALG;
    payload.kdf_salt = {};
    set_kdf_limits(payload, KdfSecurityLevel::Interactive);
    payload.enc.id = to_le64(_kp.skey.id);
    payload.enc.key = *_kp.skey.key;
    payload.enc.chk = compute_checksum(payload);

    ASSERT_TRUE(apply_xor_cipher(payload, "test"));

    // Invalid signature algorithm
    payload.sig_alg = {};
    ASSERT_TRUE(save_raw_file(_file, as_uchars(payload), {}, nullptr, nullptr));
    rewind_file();
    ASSERT_EQ(load_secret_key(_file, "test"),
              oc::failure(Error::UnsupportedSigAlg));
    clear_file();
    payload.sig_alg = SIG_ALG;

    // Invalid KDF algorithm
    payload.kdf_alg = {};
    ASSERT_TRUE(save_raw_file(_file, as_uchars(payload), {}, nullptr, nullptr));
    rewind_file();
    ASSERT_EQ(load_secret_key(_file, "test"),
              oc::failure(Error::UnsupportedKdfAlg));
    clear_file();
    payload.kdf_alg = KDF_ALG;

    // Invalid checksum algorithm
    payload.chk_alg = {};
    ASSERT_TRUE(save_raw_file(_file, as_uchars(payload), {}, nullptr, nullptr));
    rewind_file();
    ASSERT_EQ(load_secret_key(_file, "test"),
              oc::failure(Error::UnsupportedChkAlg));
    clear_file();
    payload.chk_alg = CHK_ALG;
}

TEST_F(SignTest, LoadSecretKeyFailureIncorrectChecksum)
{
    open_file();

    SKPayload payload = {};
    payload.sig_alg = SIG_ALG;
    payload.kdf_alg = KDF_ALG;
    payload.chk_alg = CHK_ALG;
    payload.kdf_salt = {};
    set_kdf_limits(payload, KdfSecurityLevel::Interactive);
    payload.enc.id = to_le64(_kp.skey.id);
    payload.enc.key = *_kp.skey.key;
    payload.enc.chk = compute_checksum(payload);
    ++payload.enc.chk[0];

    ASSERT_TRUE(apply_xor_cipher(payload, "test"));

    ASSERT_TRUE(save_raw_file(_file, as_uchars(payload), {}, nullptr, nullptr));
    rewind_file();
    ASSERT_EQ(load_secret_key(_file, "test"),
              oc::failure(Error::IncorrectChecksum));
}

TEST_F(SignTest, RoundTripPublicKey)
{
    open_file();

    ASSERT_TRUE(save_public_key(_file, _kp.pkey));
    rewind_file();

    auto key = load_public_key(_file);
    ASSERT_TRUE(key);

    ASSERT_EQ(_kp.pkey.id, key.value().id);
    ASSERT_EQ(_kp.pkey.key, key.value().key);
    ASSERT_STREQ(_kp.pkey.untrusted.data(), key.value().untrusted.data());
}

TEST_F(SignTest, LoadPublicKeyFailureUnsupportedAlgorithms)
{
    open_file();

    PKPayload payload = {};
    payload.sig_alg = {};
    payload.id = to_le64(_kp.pkey.id);
    payload.key = _kp.pkey.key;

    ASSERT_TRUE(save_raw_file(_file, as_uchars(payload), {}, nullptr, nullptr));
    rewind_file();
    ASSERT_EQ(load_public_key(_file), oc::failure(Error::UnsupportedSigAlg));
}

TEST_F(SignTest, RoundTripSignature)
{
    open_file();

    Signature sig;
    sig.id = _kp.pkey.id;
    strcpy(sig.untrusted.data(), "untrusted");
    strcpy(sig.trusted.data(), "trusted");

    for (size_t i = 0; i < sig.sig.size(); ++i) {
        sig.sig[i] = static_cast<unsigned char>(i % 256);
        sig.global_sig[i] = static_cast<unsigned char>(255 - i % 256);
    }

    ASSERT_TRUE(save_signature(_file, sig));
    rewind_file();

    auto sig2 = load_signature(_file);
    ASSERT_TRUE(sig2) << sig2.error().message();

    ASSERT_EQ(sig.id, sig2.value().id);
    ASSERT_EQ(sig.sig, sig2.value().sig);
    ASSERT_STREQ(sig.untrusted.data(), sig2.value().untrusted.data());
    ASSERT_STREQ(sig.trusted.data(), sig2.value().trusted.data());
    ASSERT_EQ(sig.global_sig, sig2.value().global_sig);
}

TEST_F(SignTest, LoadSignatureFailureUnsupportedAlgorithms)
{
    open_file();

    SigPayload payload = {};
    payload.sig_alg = {};
    payload.id = to_le64(_kp.pkey.id);
    payload.sig = {};

    TrustedComment trusted = {};
    RawSignature global_sig = {};

    ASSERT_TRUE(save_raw_file(_file, as_uchars(payload), {}, &trusted,
                              &global_sig));
    rewind_file();
    ASSERT_EQ(load_signature(_file), oc::failure(Error::UnsupportedSigAlg));
}

TEST_F(SignTest, SignVerifySuccess)
{
    open_file();
    open_data_file();

    ASSERT_TRUE(_data_file.write(as_uchars("hello", 5)));
    rewind_data_file();

    auto sig = sign_file(_data_file, _kp.skey);
    ASSERT_TRUE(sig);

    auto id_str = format("%" PRIX64, _kp.skey.id);
    ASSERT_NE(std::string_view(sig.value().untrusted.data()).find(id_str),
              std::string_view::npos);
    ASSERT_NE(std::string_view(sig.value().trusted.data()).find("timestamp:"),
              std::string_view::npos);

    rewind_data_file();
    ASSERT_TRUE(verify_file(_data_file, sig.value(), _kp.pkey));
}

TEST_F(SignTest, VerifyFailureMismatchedKey)
{
    open_file();
    open_data_file();

    ASSERT_TRUE(_data_file.write(as_uchars("hello", 5)));
    rewind_data_file();

    auto sig = sign_file(_data_file, _kp.skey);
    ASSERT_TRUE(sig);

    ++sig.value().id;

    rewind_data_file();
    ASSERT_EQ(verify_file(_data_file, sig.value(), _kp.pkey),
              oc::failure(Error::MismatchedKey));
}

TEST_F(SignTest, VerifyFailureBadSignature)
{
    open_file();
    open_data_file();

    ASSERT_TRUE(_data_file.write(as_uchars("hello", 5)));
    rewind_data_file();

    auto sig = sign_file(_data_file, _kp.skey);
    ASSERT_TRUE(sig);

    ASSERT_TRUE(_data_file.write(as_uchars("world", 5)));

    rewind_data_file();
    ASSERT_EQ(verify_file(_data_file, sig.value(), _kp.pkey),
              oc::failure(Error::SignatureVerifyFailed));
}

TEST_F(SignTest, VerifyFailureBadGlobalSignature)
{
    open_file();
    open_data_file();

    ASSERT_TRUE(_data_file.write(as_uchars("hello", 5)));
    rewind_data_file();

    auto sig = sign_file(_data_file, _kp.skey);
    ASSERT_TRUE(sig);

    strcpy(sig.value().trusted.data(), "foo");

    rewind_data_file();
    ASSERT_EQ(verify_file(_data_file, sig.value(), _kp.pkey),
              oc::failure(Error::SignatureVerifyFailed));
}
