/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file.h"
#include "mbcommon/file/memory.h"
#include "mbcommon/file/memory_p.h"

TEST(FileStaticMemoryTest, OpenFile)
{
    constexpr char *in = nullptr;
    constexpr size_t in_size = 0;

    mb::MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());
}

TEST(FileStaticMemoryTest, CloseFile)
{
    constexpr char *in = nullptr;
    constexpr size_t in_size = 0;

    mb::MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.close());
}

TEST(FileStaticMemoryTest, ReadInBounds)
{
    constexpr char in[] = "x";
    constexpr size_t in_size = 1;
    char out[1];
    size_t out_size;

    mb::MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.read(out, sizeof(out), out_size));
    ASSERT_EQ(out_size, 1u);
    ASSERT_EQ(out[0], 'x');
}

TEST(FileStaticMemoryTest, ReadOutOfBounds)
{
    constexpr char in[] = "x";
    constexpr size_t in_size = 1;
    char out[1];
    size_t out_size;

    mb::MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.seek(10, SEEK_SET, nullptr));
    ASSERT_TRUE(file.read(out, sizeof(out), out_size));
    ASSERT_EQ(out_size, 0u);
}

TEST(FileStaticMemoryTest, ReadEmpty)
{
    constexpr char *in = nullptr;
    constexpr size_t in_size = 0;
    char out[1];
    size_t out_size;

    mb::MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.read(out, sizeof(out), out_size));
    ASSERT_EQ(out_size, 0u);
}

TEST(FileStaticMemoryTest, ReadTooLarge)
{
    constexpr char in[] = "x";
    constexpr size_t in_size = 1;
    char out[2];
    size_t out_size;

    mb::MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.read(out, sizeof(out), out_size));
    ASSERT_EQ(out_size, 1u);
    ASSERT_EQ(out[0], 'x');
}

TEST(FileStaticMemoryTest, WriteInBounds)
{
    constexpr char in[] = "x";
    constexpr size_t in_size = 1;
    size_t n;

    mb::MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.write("y", 1, n));
    ASSERT_EQ(n, 1u);
    ASSERT_EQ(in[0], 'y');
}

TEST(FileStaticMemoryTest, WriteOutOfBounds)
{
    constexpr char in[] = "x";
    constexpr size_t in_size = 1;
    size_t n;

    mb::MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.seek(10, SEEK_SET, nullptr));
    ASSERT_TRUE(file.write("y", 1, n));
    ASSERT_EQ(n, 0u);
}

TEST(FileStaticMemoryTest, WriteTooLarge)
{
    constexpr char in[] = "x";
    constexpr size_t in_size = 1;
    size_t n;

    mb::MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.write("yz", 2, n));
    ASSERT_EQ(n, 1u);
    ASSERT_EQ(in[0], 'y');
}

TEST(FileStaticMemoryTest, SeekNormal)
{
    constexpr char in[] = "abcdefghijklmnopqrstuvwxyz";
    constexpr size_t in_size = 26;
    uint64_t pos;

    mb::MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    // SEEK_SET
    ASSERT_TRUE(file.seek(10, SEEK_SET, &pos));
    ASSERT_EQ(pos, 10u);

    // Positive SEEK_CUR
    ASSERT_TRUE(file.seek(10, SEEK_CUR, &pos));
    ASSERT_EQ(pos, 20u);

    // Negative SEEK_CUR
    ASSERT_TRUE(file.seek(-8, SEEK_CUR, &pos));
    ASSERT_EQ(pos, 12u);

    // Positive SEEK_END (out of bounds)
    ASSERT_TRUE(file.seek(10, SEEK_END, &pos));
    ASSERT_EQ(pos, 36u);

    // Negative SEEK_END
    ASSERT_TRUE(file.seek(-18, SEEK_END, &pos));
    ASSERT_EQ(pos, 8u);
}

TEST(FileStaticMemoryTest, SeekInvalid)
{
    constexpr char in[] = "abcdefghijklmnopqrstuvwxyz";
    constexpr size_t in_size = 26;
    uint64_t pos;

    mb::MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    // Negative SEEK_SET
    ASSERT_FALSE(file.seek(-10, SEEK_SET, &pos));
    ASSERT_EQ(file.error(), mb::FileError::ArgumentOutOfRange);
    ASSERT_NE(file.error_string().find("Invalid SEEK_SET"), std::string::npos);

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_SET
    ASSERT_FALSE(file.seek(static_cast<int64_t>(SIZE_MAX) + 1, SEEK_SET, &pos));
    ASSERT_EQ(file.error(), mb::FileError::ArgumentOutOfRange);
    ASSERT_NE(file.error_string().find("Invalid SEEK_SET"), std::string::npos);
#endif

    // Negative out of range SEEK_CUR
    ASSERT_FALSE(file.seek(-100, SEEK_CUR, &pos));
    ASSERT_EQ(file.error(), mb::FileError::ArgumentOutOfRange);
    ASSERT_NE(file.error_string().find("Invalid SEEK_CUR"), std::string::npos);

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_CUR
    ASSERT_FALSE(file.seek(static_cast<int64_t>(SIZE_MAX) + 1, SEEK_CUR, &pos));
    ASSERT_EQ(file.error(), mb::FileError::ArgumentOutOfRange);
    ASSERT_NE(file.error_string().find("Invalid SEEK_CUR"), std::string::npos);
#endif

    // Negative out of range SEEK_END
    ASSERT_FALSE(file.seek(-100, SEEK_END, &pos));
    ASSERT_EQ(file.error(), mb::FileError::ArgumentOutOfRange);
    ASSERT_NE(file.error_string().find("Invalid SEEK_END"), std::string::npos);

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_END
    ASSERT_FALSE(file.seek(static_cast<int64_t>(SIZE_MAX) + 1, SEEK_END, &pos));
    ASSERT_EQ(file.error(), mb::FileError::ArgumentOutOfRange);
    ASSERT_NE(file.error_string().find("Invalid SEEK_END"), std::string::npos);
#endif
}

TEST(FileStaticMemoryTest, CheckTruncateUnsupported)
{
    constexpr char in[] = "x";
    constexpr size_t in_size = 1;

    mb::MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_FALSE(file.truncate(10));
    ASSERT_EQ(file.error(), mb::FileError::UnsupportedTruncate);
    ASSERT_NE(file.error_string().find("truncate"), std::string::npos);
}

TEST(FileDynamicMemoryTest, OpenFile)
{
    void *in = nullptr;
    size_t in_size = 0;

    mb::MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    free(in);
}

TEST(FileDynamicMemoryTest, CloseFile)
{
    void *in = nullptr;
    size_t in_size = 0;

    mb::MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.close());

    free(in);
}

TEST(FileDynamicMemoryTest, ReadInBounds)
{
    void *in = strdup("x");
    size_t in_size = 1;
    char out[1];
    size_t out_size;

    ASSERT_NE(in, nullptr);

    mb::MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.read(out, sizeof(out), out_size));
    ASSERT_EQ(out_size, 1u);
    ASSERT_EQ(out[0], 'x');

    free(in);
}

TEST(FileDynamicMemoryTest, ReadOutOfBounds)
{
    void *in = strdup("x");
    size_t in_size = 1;
    char out[1];
    size_t out_size;

    ASSERT_NE(in, nullptr);

    mb::MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.seek(10, SEEK_SET, nullptr));

    ASSERT_TRUE(file.read(out, sizeof(out), out_size));
    ASSERT_EQ(out_size, 0u);

    free(in);
}

TEST(FileDynamicMemoryTest, ReadEmpty)
{
    void *in = nullptr;
    size_t in_size = 0;
    char out[1];
    size_t out_size;

    mb::MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.read(out, sizeof(out), out_size));
    ASSERT_EQ(out_size, 0u);

    free(in);
}

TEST(FileDynamicMemoryTest, ReadTooLarge)
{
    void *in = strdup("x");
    size_t in_size = 1;
    char out[1];
    size_t out_size;

    ASSERT_NE(in, nullptr);

    mb::MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.read(out, sizeof(out), out_size));
    ASSERT_EQ(out_size, 1u);
    ASSERT_EQ(out[0], 'x');

    free(in);
}

TEST(FileDynamicMemoryTest, WriteInBounds)
{
    void *in = strdup("x");
    size_t in_size = 1;
    size_t n;

    ASSERT_NE(in, nullptr);

    mb::MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.write("y", 1, n));
    ASSERT_EQ(n, 1u);
    ASSERT_EQ(static_cast<char *>(in)[0], 'y');

    free(in);
}

TEST(FileDynamicMemoryTest, WriteOutOfBounds)
{
    void *in = strdup("x");
    size_t in_size = 1;
    size_t n;

    ASSERT_NE(in, nullptr);

    mb::MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.seek(10, SEEK_SET, nullptr));

    ASSERT_TRUE(file.write("y", 1, n));
    ASSERT_EQ(n, 1u);
    ASSERT_EQ(in_size, 11u);
    ASSERT_NE(in, nullptr);
    ASSERT_EQ(static_cast<char *>(in)[10], 'y');

    free(in);
}

TEST(FileDynamicMemoryTest, WriteEmpty)
{
    void *in = nullptr;
    size_t in_size = 0;
    size_t n;

    mb::MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.write("x", 1, n));
    ASSERT_EQ(n, 1u);
    ASSERT_EQ(in_size, 1u);
    ASSERT_NE(in, nullptr);
    ASSERT_EQ(static_cast<char *>(in)[0], 'x');

    free(in);
}

TEST(FileDynamicMemoryTest, WriteTooLarge)
{
    void *in = strdup("x");
    size_t in_size = 1;
    size_t n;

    ASSERT_NE(in, nullptr);

    mb::MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.write("yz", 2, n));
    ASSERT_EQ(n, 2u);
    ASSERT_EQ(in_size, 2u);
    ASSERT_NE(in, nullptr);
    ASSERT_EQ(memcmp(in, "yz", 2), 0);

    free(in);
}

TEST(FileDynamicMemoryTest, SeekNormal)
{
    void *in = strdup("abcdefghijklmnopqrstuvwxyz");
    size_t in_size = 26;
    uint64_t pos;

    ASSERT_NE(in, nullptr);

    mb::MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    // SEEK_SET
    ASSERT_TRUE(file.seek(10, SEEK_SET, &pos));
    ASSERT_EQ(pos, 10u);

    // Positive SEEK_CUR
    ASSERT_TRUE(file.seek(10, SEEK_CUR, &pos));
    ASSERT_EQ(pos, 20u);

    // Negative SEEK_CUR
    ASSERT_TRUE(file.seek(-8, SEEK_CUR, &pos));
    ASSERT_EQ(pos, 12u);

    // Positive SEEK_END (out of bounds)
    ASSERT_TRUE(file.seek(10, SEEK_END, &pos));
    ASSERT_EQ(pos, 36u);

    // Negative SEEK_END
    ASSERT_TRUE(file.seek(-18, SEEK_END, &pos));
    ASSERT_EQ(pos, 8u);

    free(in);
}

TEST(FileDynamicMemoryTest, SeekInvalid)
{
    void *in = strdup("abcdefghijklmnopqrstuvwxyz");
    size_t in_size = 26;
    uint64_t pos;

    ASSERT_NE(in, nullptr);

    mb::MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    // Negative SEEK_SET
    ASSERT_FALSE(file.seek(-10, SEEK_SET, &pos));
    ASSERT_EQ(file.error(), mb::FileError::ArgumentOutOfRange);
    ASSERT_NE(file.error_string().find("Invalid SEEK_SET"), std::string::npos);

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_SET
    ASSERT_FALSE(file.seek(static_cast<int64_t>(SIZE_MAX) + 1, SEEK_SET, &pos));
    ASSERT_EQ(file.error(), mb::FileError::ArgumentOutOfRange);
    ASSERT_NE(file.error_string().find("Invalid SEEK_SET"), std::string::npos);
#endif

    // Negative out of range SEEK_CUR
    ASSERT_FALSE(file.seek(-100, SEEK_CUR, &pos));
    ASSERT_EQ(file.error(), mb::FileError::ArgumentOutOfRange);
    ASSERT_NE(file.error_string().find("Invalid SEEK_CUR"), std::string::npos);

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_CUR
    ASSERT_FALSE(file.seek(static_cast<int64_t>(SIZE_MAX) + 1, SEEK_CUR, &pos));
    ASSERT_EQ(file.error(), mb::FileError::ArgumentOutOfRange);
    ASSERT_NE(file.error_string().find("Invalid SEEK_CUR"), std::string::npos);
#endif

    // Negative out of range SEEK_END
    ASSERT_FALSE(file.seek(-100, SEEK_END, &pos));
    ASSERT_EQ(file.error(), mb::FileError::ArgumentOutOfRange);
    ASSERT_NE(file.error_string().find("Invalid SEEK_END"), std::string::npos);

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_END
    ASSERT_FALSE(file.seek(static_cast<int64_t>(SIZE_MAX) + 1, SEEK_END, &pos));
    ASSERT_EQ(file.error(), mb::FileError::ArgumentOutOfRange);
    ASSERT_NE(file.error_string().find("Invalid SEEK_END"), std::string::npos);
#endif

    free(in);
}

TEST(FileDynamicMemoryTest, TruncateFile)
{
    void *in = strdup("x");
    size_t in_size = 1;

    mb::MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.truncate(10));
    ASSERT_EQ(in_size, 10u);
    // Dumb check to ensure that the new memory is initialized
    for (int i = 1; i < 10; ++i) {
        ASSERT_EQ(static_cast<char *>(in)[i], '\0');
    }

    ASSERT_TRUE(file.truncate(5));
    ASSERT_EQ(in_size, 5u);

    free(in);
}
