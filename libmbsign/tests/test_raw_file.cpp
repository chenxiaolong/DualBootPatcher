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

#include <gtest/gtest.h>

#include <string_view>

#include "mbcommon/file/memory.h"

#include "mbsign/detail/raw_file.h"
#include "mbsign/error.h"

using namespace mb;
using namespace mb::sign;
using namespace mb::sign::detail;
using namespace testing;

struct RawFileTest : Test
{
    MemoryFile _file;
    void *_file_data = nullptr;
    size_t _file_size = 0;

    ~RawFileTest()
    {
        free(_file_data);
    }

    void open_file()
    {
        ASSERT_TRUE(_file.open(&_file_data, &_file_size));
    }

    void open_file(std::string_view data)
    {
        open_file();

        ASSERT_TRUE(_file.write(as_uchars(data.data(), data.size())));
        ASSERT_TRUE(_file.seek(0, SEEK_SET));
    }
};

TEST_F(RawFileTest, SaveFileSuccessWithTrusted)
{
    static constexpr std::string_view expected =
R"(untrusted comment: foo
aGVsbG8=
trusted comment: bar
AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+Pw==
)";

    open_file();

    char payload[5] = {'h', 'e', 'l', 'l', 'o'};
    UntrustedComment untrusted;
    strcpy(untrusted.data(), "foo");
    TrustedComment trusted;
    strcpy(trusted.data(), "bar");
    RawSignature global_sig;

    for (size_t i = 0; i < global_sig.size(); ++i) {
        global_sig[i] = static_cast<unsigned char>(i % 256);
    }

    ASSERT_TRUE(save_raw_file(_file, as_uchars(payload), untrusted, &trusted,
                              &global_sig));

    ASSERT_EQ(std::string_view(static_cast<char *>(_file_data), _file_size),
              expected);
}

TEST_F(RawFileTest, SaveFileSuccessWithoutTrusted)
{
    static constexpr std::string_view expected =
R"(untrusted comment: foo
aGVsbG8=
)";

    open_file();

    char payload[5] = {'h', 'e', 'l', 'l', 'o'};
    UntrustedComment untrusted;
    strcpy(untrusted.data(), "foo");

    ASSERT_TRUE(save_raw_file(_file, as_uchars(payload), untrusted, nullptr,
                              nullptr));

    ASSERT_EQ(std::string_view(static_cast<char *>(_file_data), _file_size),
              expected);
}

TEST_F(RawFileTest, SaveFileSuccessWithEmptyComments)
{
    // Single line to prevent editors from stripping out trailing whitespace
    static constexpr std::string_view expected = "untrusted comment: \naGVsbG8=\ntrusted comment: \nAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==\n";

    open_file();

    char payload[5] = {'h', 'e', 'l', 'l', 'o'};
    UntrustedComment untrusted = {};
    TrustedComment trusted = {};
    RawSignature global_sig = {};

    ASSERT_TRUE(save_raw_file(_file, as_uchars(payload), untrusted, &trusted,
                              &global_sig));

    ASSERT_EQ(std::string_view(static_cast<char *>(_file_data), _file_size),
              expected);
}

TEST_F(RawFileTest, SaveFileFailureInvalidArgument)
{
    open_file();

    char payload[5] = {'h', 'e', 'l', 'l', 'o'};
    UntrustedComment untrusted = {};
    TrustedComment trusted = {};
    RawSignature global_sig = {};

    ASSERT_EQ(save_raw_file(_file, as_uchars(payload), untrusted, nullptr,
                            &global_sig),
              oc::failure(std::errc::invalid_argument));
    ASSERT_EQ(save_raw_file(_file, as_uchars(payload), untrusted, &trusted,
                            nullptr),
              oc::failure(std::errc::invalid_argument));
}

TEST_F(RawFileTest, SaveFileFailureInvalidUntrustedComment)
{
    open_file();

    char payload[5] = {'h', 'e', 'l', 'l', 'o'};
    UntrustedComment untrusted;
    strcpy(untrusted.data(), "foo\nbar");

    ASSERT_EQ(save_raw_file(_file, as_uchars(payload), untrusted, nullptr,
                            nullptr),
              oc::failure(Error::InvalidUntrustedComment));
}

TEST_F(RawFileTest, SaveFileFailureInvalidTrustedComment)
{
    open_file();

    char payload[5] = {'h', 'e', 'l', 'l', 'o'};
    UntrustedComment untrusted;
    strcpy(untrusted.data(), "foo");
    TrustedComment trusted;
    strcpy(trusted.data(), "bar\r");
    RawSignature global_sig = {};

    ASSERT_EQ(save_raw_file(_file, as_uchars(payload), untrusted, &trusted,
                            &global_sig),
              oc::failure(Error::InvalidTrustedComment));
}

TEST_F(RawFileTest, LoadFileSuccessWithTrusted)
{
    static constexpr std::string_view data =
R"(untrusted comment: foo
aGVsbG8=
trusted comment: bar
AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+Pw==
)";

    open_file(data);

    unsigned char payload[5];
    UntrustedComment untrusted;
    TrustedComment trusted;
    RawSignature global_sig;

    ASSERT_TRUE(load_raw_file(_file, payload, untrusted, &trusted,
                              &global_sig));

    RawSignature expected_global_sig;

    for (size_t i = 0; i < expected_global_sig.size(); ++i) {
        expected_global_sig[i] = static_cast<unsigned char>(i % 256);
    }

    ASSERT_EQ(memcmp(payload, "hello", 5), 0);
    ASSERT_STREQ(untrusted.data(), "foo");
    ASSERT_STREQ(trusted.data(), "bar");
    ASSERT_EQ(global_sig, expected_global_sig);
}

TEST_F(RawFileTest, LoadFileSuccessWithoutTrusted)
{
    static constexpr std::string_view data =
R"(untrusted comment: foo
aGVsbG8=
)";

    open_file(data);

    unsigned char payload[5];
    UntrustedComment untrusted;

    ASSERT_TRUE(load_raw_file(_file, payload, untrusted, nullptr, nullptr));

    ASSERT_EQ(memcmp(payload, "hello", 5), 0);
    ASSERT_STREQ(untrusted.data(), "foo");
}

TEST_F(RawFileTest, LoadFileSuccessWithEmptyComments)
{
    // Single line to prevent editors from stripping out trailing whitespace
    static constexpr std::string_view data = "untrusted comment: \naGVsbG8=\ntrusted comment: \nAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==\n";

    open_file(data);

    unsigned char payload[5];
    UntrustedComment untrusted;
    TrustedComment trusted;
    RawSignature global_sig;

    ASSERT_TRUE(load_raw_file(_file, payload, untrusted, &trusted,
                              &global_sig));

    RawSignature expected_global_sig = {};

    ASSERT_EQ(memcmp(payload, "hello", 5), 0);
    ASSERT_STREQ(untrusted.data(), "");
    ASSERT_STREQ(trusted.data(), "");
    ASSERT_EQ(global_sig, expected_global_sig);
}

TEST_F(RawFileTest, LoadFileFailureInvalidArgument)
{
    open_file();

    unsigned char payload[5];
    UntrustedComment untrusted;
    TrustedComment trusted;
    RawSignature global_sig;

    ASSERT_EQ(load_raw_file(_file, payload, untrusted, nullptr, &global_sig),
              oc::failure(std::errc::invalid_argument));
    ASSERT_EQ(load_raw_file(_file, payload, untrusted, &trusted, nullptr),
              oc::failure(std::errc::invalid_argument));
}

TEST_F(RawFileTest, LoadFileFailureInvalidUntrustedComment)
{
    static constexpr std::string_view data =
R"(untrusted bananas: foo
aGVsbG8=
)";

    open_file(data);

    unsigned char payload[5];
    UntrustedComment untrusted;

    ASSERT_EQ(load_raw_file(_file, payload, untrusted, nullptr, nullptr),
              oc::failure(Error::InvalidUntrustedComment));
}

TEST_F(RawFileTest, LoadFileFailureInvalidTrustedComment)
{
    static constexpr std::string_view data =
R"(untrusted comment: foo
aGVsbG8=
trusted bananas: bar
AAECAwQFBgcICQoLDA0ODxAREhMUFRYXGBkaGxwdHh8gISIjJCUmJygpKissLS4vMDEyMzQ1Njc4OTo7PD0+Pw==
)";

    open_file(data);

    unsigned char payload[5];
    UntrustedComment untrusted;
    TrustedComment trusted;
    RawSignature global_sig;

    ASSERT_EQ(load_raw_file(_file, payload, untrusted, &trusted, &global_sig),
              oc::failure(Error::InvalidTrustedComment));
}

TEST_F(RawFileTest, LoadFileFailureInvalidPayloadSize)
{
    static constexpr std::string_view data =
R"(untrusted comment: foo
aGVsbG8=
)";

    open_file(data);

    unsigned char payload[6];
    UntrustedComment untrusted;

    ASSERT_EQ(load_raw_file(_file, payload, untrusted, nullptr, nullptr),
              oc::failure(Error::InvalidPayloadSize));
}

TEST_F(RawFileTest, LoadFileFailureInvalidGlobalSigSize)
{
    static constexpr std::string_view data =
R"(untrusted comment: foo
aGVsbG8=
trusted comment: bar
aGVsbG8=
)";

    open_file(data);

    unsigned char payload[5];
    UntrustedComment untrusted;
    TrustedComment trusted;
    RawSignature global_sig;

    ASSERT_EQ(load_raw_file(_file, payload, untrusted, &trusted, &global_sig),
              oc::failure(Error::InvalidGlobalSigSize));
}

TEST_F(RawFileTest, LoadFileFailureBase64DecodeError)
{
    static constexpr std::string_view data =
R"(untrusted comment: foo
abcdefghijklmnopqrstuvwxyz
)";

    open_file(data);

    unsigned char payload[6];
    UntrustedComment untrusted;

    ASSERT_EQ(load_raw_file(_file, payload, untrusted, nullptr, nullptr),
              oc::failure(Error::Base64DecodeError));
}
