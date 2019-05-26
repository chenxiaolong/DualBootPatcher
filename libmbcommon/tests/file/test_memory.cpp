/*
 * Copyright (C) 2017-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include "mbcommon/file_error.h"

using namespace mb;

TEST(FileMemoryTest, CheckInvalidStates)
{
    MemoryFile file;

    auto error = oc::failure(FileError::InvalidState);

    ASSERT_EQ(file.close(), error);
    ASSERT_EQ(file.read(nullptr, 0), error);
    ASSERT_EQ(file.write(nullptr, 0), error);
    ASSERT_EQ(file.seek(0, SEEK_SET), error);
    ASSERT_EQ(file.truncate(1024), error);

    void *in = nullptr;
    size_t in_size = 0;

    ASSERT_TRUE(file.open(in, in_size));
    ASSERT_EQ(file.open(in, in_size), error);
    ASSERT_EQ(file.open(&in, &in_size), error);
}

TEST(FileStaticMemoryTest, OpenFile)
{
    constexpr char *in = nullptr;
    constexpr size_t in_size = 0;

    MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());
}

TEST(FileStaticMemoryTest, CloseFile)
{
    constexpr char *in = nullptr;
    constexpr size_t in_size = 0;

    MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.close());
}

TEST(FileStaticMemoryTest, ReadInBounds)
{
    char in[] = "x";
    constexpr size_t in_size = 1;
    char out[1];

    MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.read(out, sizeof(out)), oc::success(1u));
    ASSERT_EQ(out[0], 'x');
}

TEST(FileStaticMemoryTest, ReadOutOfBounds)
{
    char in[] = "x";
    constexpr size_t in_size = 1;
    char out[1];

    MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.seek(10, SEEK_SET));
    ASSERT_EQ(file.read(out, sizeof(out)), oc::success(0u));
}

TEST(FileStaticMemoryTest, ReadEmpty)
{
    constexpr char *in = nullptr;
    constexpr size_t in_size = 0;
    char out[1];

    MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.read(out, sizeof(out)), oc::success(0u));
}

TEST(FileStaticMemoryTest, ReadTooLarge)
{
    char in[] = "x";
    constexpr size_t in_size = 1;
    char out[2];

    MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.read(out, sizeof(out)), oc::success(1u));
    ASSERT_EQ(out[0], 'x');
}

TEST(FileStaticMemoryTest, WriteInBounds)
{
    char in[] = "x";
    constexpr size_t in_size = 1;

    MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.write("y", 1), oc::success(1u));
    ASSERT_EQ(in[0], 'y');
}

TEST(FileStaticMemoryTest, WriteOutOfBounds)
{
    char in[] = "x";
    constexpr size_t in_size = 1;

    MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.seek(10, SEEK_SET));
    ASSERT_EQ(file.write("y", 1), oc::success(0u));
}

TEST(FileStaticMemoryTest, WriteTooLarge)
{
    char in[] = "x";
    constexpr size_t in_size = 1;

    MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.write("yz", 2), oc::success(1u));
    ASSERT_EQ(in[0], 'y');
}

TEST(FileStaticMemoryTest, SeekNormal)
{
    char in[] = "abcdefghijklmnopqrstuvwxyz";
    constexpr size_t in_size = 26;

    MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    // SEEK_SET
    ASSERT_EQ(file.seek(10, SEEK_SET), oc::success(10u));

    // Positive SEEK_CUR
    ASSERT_EQ(file.seek(10, SEEK_CUR), oc::success(20u));

    // Negative SEEK_CUR
    ASSERT_EQ(file.seek(-8, SEEK_CUR), oc::success(12u));

    // Positive SEEK_END (out of bounds)
    ASSERT_EQ(file.seek(10, SEEK_END), oc::success(36u));

    // Negative SEEK_END
    ASSERT_EQ(file.seek(-18, SEEK_END), oc::success(8u));
}

TEST(FileStaticMemoryTest, SeekInvalid)
{
    char in[] = "abcdefghijklmnopqrstuvwxyz";
    constexpr size_t in_size = 26;

    MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    // Negative SEEK_SET
    ASSERT_EQ(file.seek(-10, SEEK_SET),
              oc::failure(FileError::ArgumentOutOfRange));

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_SET
    ASSERT_EQ(file.seek(static_cast<int64_t>(SIZE_MAX) + 1, SEEK_SET),
              oc::failure(FileError::ArgumentOutOfRange));
#endif

    // Negative out of range SEEK_CUR
    ASSERT_EQ(file.seek(-100, SEEK_CUR),
              oc::failure(FileError::ArgumentOutOfRange));

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_CUR
    ASSERT_EQ(file.seek(static_cast<int64_t>(SIZE_MAX) + 1, SEEK_CUR),
              oc::failure(FileError::ArgumentOutOfRange));
#endif

    // Negative out of range SEEK_END
    ASSERT_EQ(file.seek(-100, SEEK_END),
              oc::failure(FileError::ArgumentOutOfRange));

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_END
    ASSERT_EQ(file.seek(static_cast<int64_t>(SIZE_MAX) + 1, SEEK_END),
              oc::failure(FileError::ArgumentOutOfRange));
#endif
}

TEST(FileStaticMemoryTest, CheckTruncateUnsupported)
{
    char in[] = "x";
    constexpr size_t in_size = 1;

    MemoryFile file(in, in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.truncate(10), oc::failure(FileError::UnsupportedTruncate));
}

TEST(FileDynamicMemoryTest, OpenFile)
{
    void *in = nullptr;
    size_t in_size = 0;

    MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    free(in);
}

TEST(FileDynamicMemoryTest, CloseFile)
{
    void *in = nullptr;
    size_t in_size = 0;

    MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.close());

    free(in);
}

TEST(FileDynamicMemoryTest, ReadInBounds)
{
    void *in = strdup("x");
    size_t in_size = 1;
    char out[1];

    ASSERT_NE(in, nullptr);

    MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.read(out, sizeof(out)), oc::success(1u));
    ASSERT_EQ(out[0], 'x');

    free(in);
}

TEST(FileDynamicMemoryTest, ReadOutOfBounds)
{
    void *in = strdup("x");
    size_t in_size = 1;
    char out[1];

    ASSERT_NE(in, nullptr);

    MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.seek(10, SEEK_SET));

    ASSERT_EQ(file.read(out, sizeof(out)), oc::success(0u));

    free(in);
}

TEST(FileDynamicMemoryTest, ReadEmpty)
{
    void *in = nullptr;
    size_t in_size = 0;
    char out[1];

    MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.read(out, sizeof(out)), oc::success(0u));

    free(in);
}

TEST(FileDynamicMemoryTest, ReadTooLarge)
{
    void *in = strdup("x");
    size_t in_size = 1;
    char out[1];

    ASSERT_NE(in, nullptr);

    MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.read(out, sizeof(out)), oc::success(1u));
    ASSERT_EQ(out[0], 'x');

    free(in);
}

TEST(FileDynamicMemoryTest, WriteInBounds)
{
    void *in = strdup("x");
    size_t in_size = 1;

    ASSERT_NE(in, nullptr);

    MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.write("y", 1), oc::success(1u));
    ASSERT_EQ(static_cast<char *>(in)[0], 'y');

    free(in);
}

TEST(FileDynamicMemoryTest, WriteOutOfBounds)
{
    void *in = strdup("x");
    size_t in_size = 1;

    ASSERT_NE(in, nullptr);

    MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_TRUE(file.seek(10, SEEK_SET));

    ASSERT_EQ(file.write("y", 1), oc::success(1u));
    ASSERT_EQ(in_size, 11u);
    ASSERT_NE(in, nullptr);
    ASSERT_EQ(static_cast<char *>(in)[10], 'y');

    free(in);
}

TEST(FileDynamicMemoryTest, WriteEmpty)
{
    void *in = nullptr;
    size_t in_size = 0;

    MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.write("x", 1), oc::success(1u));
    ASSERT_EQ(in_size, 1u);
    ASSERT_NE(in, nullptr);
    ASSERT_EQ(static_cast<char *>(in)[0], 'x');

    free(in);
}

TEST(FileDynamicMemoryTest, WriteTooLarge)
{
    void *in = strdup("x");
    size_t in_size = 1;

    ASSERT_NE(in, nullptr);

    MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    ASSERT_EQ(file.write("yz", 2), oc::success(2u));
    ASSERT_EQ(in_size, 2u);
    ASSERT_NE(in, nullptr);
    ASSERT_EQ(memcmp(in, "yz", 2), 0);

    free(in);
}

TEST(FileDynamicMemoryTest, SeekNormal)
{
    void *in = strdup("abcdefghijklmnopqrstuvwxyz");
    size_t in_size = 26;

    ASSERT_NE(in, nullptr);

    MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    // SEEK_SET
    ASSERT_EQ(file.seek(10, SEEK_SET), oc::success(10u));

    // Positive SEEK_CUR
    ASSERT_EQ(file.seek(10, SEEK_CUR), oc::success(20u));

    // Negative SEEK_CUR
    ASSERT_EQ(file.seek(-8, SEEK_CUR), oc::success(12u));

    // Positive SEEK_END (out of bounds)
    ASSERT_EQ(file.seek(10, SEEK_END), oc::success(36u));

    // Negative SEEK_END
    ASSERT_EQ(file.seek(-18, SEEK_END), oc::success(8u));

    free(in);
}

TEST(FileDynamicMemoryTest, SeekInvalid)
{
    void *in = strdup("abcdefghijklmnopqrstuvwxyz");
    size_t in_size = 26;

    ASSERT_NE(in, nullptr);

    MemoryFile file(&in, &in_size);
    ASSERT_TRUE(file.is_open());

    // Negative SEEK_SET
    ASSERT_EQ(file.seek(-10, SEEK_SET),
              oc::failure(FileError::ArgumentOutOfRange));

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_SET
    ASSERT_EQ(file.seek(static_cast<int64_t>(SIZE_MAX) + 1, SEEK_SET),
              oc::failure(FileError::ArgumentOutOfRange));
#endif

    // Negative out of range SEEK_CUR
    ASSERT_EQ(file.seek(-100, SEEK_CUR),
              oc::failure(FileError::ArgumentOutOfRange));

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_CUR
    ASSERT_EQ(file.seek(static_cast<int64_t>(SIZE_MAX) + 1, SEEK_CUR),
              oc::failure(FileError::ArgumentOutOfRange));
#endif

    // Negative out of range SEEK_END
    ASSERT_EQ(file.seek(-100, SEEK_END),
              oc::failure(FileError::ArgumentOutOfRange));

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_END
    ASSERT_EQ(file.seek(static_cast<int64_t>(SIZE_MAX) + 1, SEEK_END),
              oc::failure(FileError::ArgumentOutOfRange));
#endif

    free(in);
}

TEST(FileDynamicMemoryTest, TruncateFile)
{
    void *in = strdup("x");
    size_t in_size = 1;

    MemoryFile file(&in, &in_size);
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
