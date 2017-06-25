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

typedef std::unique_ptr<MbFile, decltype(mb_file_free) *> ScopedFile;

TEST(FileStaticMemoryTest, OpenFile)
{
    char *in = nullptr;
    size_t in_size = 0;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), in, in_size), MB_FILE_OK);
}

TEST(FileStaticMemoryTest, CloseFile)
{
    char *in = nullptr;
    size_t in_size = 0;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), in, in_size), MB_FILE_OK);

    ASSERT_EQ(mb_file_close(file.get()), MB_FILE_OK);
}

TEST(FileStaticMemoryTest, ReadInBounds)
{
    char in[] = "x";
    size_t in_size = 1;
    char out[1];
    size_t out_size;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), in, in_size), MB_FILE_OK);

    ASSERT_EQ(mb_file_read(file.get(), out, sizeof(out), &out_size),
              MB_FILE_OK);
    ASSERT_EQ(out_size, 1);
    ASSERT_EQ(out[0], 'x');
}

TEST(FileStaticMemoryTest, ReadOutOfBounds)
{
    char in[] = "x";
    size_t in_size = 1;
    char out[1];
    size_t out_size;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), in, in_size), MB_FILE_OK);

    ASSERT_EQ(mb_file_seek(file.get(), 10, SEEK_SET, nullptr), MB_FILE_OK);

    ASSERT_EQ(mb_file_read(file.get(), out, sizeof(out), &out_size),
              MB_FILE_OK);
    ASSERT_EQ(out_size, 0);
}

TEST(FileStaticMemoryTest, ReadEmpty)
{
    char *in = nullptr;
    size_t in_size = 0;
    char out[1];
    size_t out_size;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), in, in_size), MB_FILE_OK);

    ASSERT_EQ(mb_file_read(file.get(), out, sizeof(out), &out_size),
              MB_FILE_OK);
    ASSERT_EQ(out_size, 0);
}

TEST(FileStaticMemoryTest, ReadTooLarge)
{
    char in[] = "x";
    size_t in_size = 1;
    char out[2];
    size_t out_size;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), in, in_size), MB_FILE_OK);

    ASSERT_EQ(mb_file_read(file.get(), out, sizeof(out), &out_size),
              MB_FILE_OK);
    ASSERT_EQ(out_size, 1);
    ASSERT_EQ(out[0], 'x');
}

TEST(FileStaticMemoryTest, WriteInBounds)
{
    char in[] = "x";
    size_t in_size = 1;
    size_t n;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), in, in_size), MB_FILE_OK);

    ASSERT_EQ(mb_file_write(file.get(), "y", 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(in[0], 'y');
}

TEST(FileStaticMemoryTest, WriteOutOfBounds)
{
    char in[] = "x";
    size_t in_size = 1;
    size_t n;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), in, in_size), MB_FILE_OK);

    ASSERT_EQ(mb_file_seek(file.get(), 10, SEEK_SET, nullptr), MB_FILE_OK);

    ASSERT_EQ(mb_file_write(file.get(), "y", 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 0);
}

TEST(FileStaticMemoryTest, WriteTooLarge)
{
    char in[] = "x";
    size_t in_size = 1;
    size_t n;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), in, in_size), MB_FILE_OK);

    ASSERT_EQ(mb_file_write(file.get(), "yz", 2, &n), MB_FILE_OK);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(in[0], 'y');
}

TEST(FileStaticMemoryTest, SeekNormal)
{
    char in[] = "abcdefghijklmnopqrstuvwxyz";
    size_t in_size = 26;
    uint64_t pos;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), in, in_size), MB_FILE_OK);

    // SEEK_SET
    ASSERT_EQ(mb_file_seek(file.get(), 10, SEEK_SET, &pos), MB_FILE_OK);
    ASSERT_EQ(pos, 10);

    // Positive SEEK_CUR
    ASSERT_EQ(mb_file_seek(file.get(), 10, SEEK_CUR, &pos), MB_FILE_OK);
    ASSERT_EQ(pos, 20);

    // Negative SEEK_CUR
    ASSERT_EQ(mb_file_seek(file.get(), -8, SEEK_CUR, &pos), MB_FILE_OK);
    ASSERT_EQ(pos, 12);

    // Positive SEEK_END (out of bounds)
    ASSERT_EQ(mb_file_seek(file.get(), 10, SEEK_END, &pos), MB_FILE_OK);
    ASSERT_EQ(pos, 36);

    // Negative SEEK_END
    ASSERT_EQ(mb_file_seek(file.get(), -18, SEEK_END, &pos), MB_FILE_OK);
    ASSERT_EQ(pos, 8);
}

TEST(FileStaticMemoryTest, SeekInvalid)
{
    char in[] = "abcdefghijklmnopqrstuvwxyz";
    size_t in_size = 26;
    uint64_t pos;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), in, in_size), MB_FILE_OK);

    // Negative SEEK_SET
    ASSERT_EQ(mb_file_seek(file.get(), -10, SEEK_SET, &pos), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(file.get()), MB_FILE_ERROR_INVALID_ARGUMENT);
    ASSERT_TRUE(strstr(mb_file_error_string(file.get()), "Invalid SEEK_SET"));

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_SET
    ASSERT_EQ(mb_file_seek(file.get(), static_cast<int64_t>(SIZE_MAX) + 1,
                           SEEK_SET, &pos), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(file.get()), MB_FILE_ERROR_INVALID_ARGUMENT);
    ASSERT_TRUE(strstr(mb_file_error_string(file.get()), "Invalid SEEK_SET"));
#endif

    // Negative out of range SEEK_CUR
    ASSERT_EQ(mb_file_seek(file.get(), -100, SEEK_CUR, &pos), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(file.get()), MB_FILE_ERROR_INVALID_ARGUMENT);
    ASSERT_TRUE(strstr(mb_file_error_string(file.get()), "Invalid SEEK_CUR"));

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_CUR
    ASSERT_EQ(mb_file_seek(file.get(), static_cast<int64_t>(SIZE_MAX) + 1,
                           SEEK_CUR, &pos), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(file.get()), MB_FILE_ERROR_INVALID_ARGUMENT);
    ASSERT_TRUE(strstr(mb_file_error_string(file.get()), "Invalid SEEK_CUR"));
#endif

    // Negative out of range SEEK_END
    ASSERT_EQ(mb_file_seek(file.get(), -100, SEEK_END, &pos), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(file.get()), MB_FILE_ERROR_INVALID_ARGUMENT);
    ASSERT_TRUE(strstr(mb_file_error_string(file.get()), "Invalid SEEK_END"));

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_END
    ASSERT_EQ(mb_file_seek(file.get(), static_cast<int64_t>(SIZE_MAX) + 1,
                           SEEK_END, &pos), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(file.get()), MB_FILE_ERROR_INVALID_ARGUMENT);
    ASSERT_TRUE(strstr(mb_file_error_string(file.get()), "Invalid SEEK_END"));
#endif
}

TEST(FileStaticMemoryTest, CheckTruncateUnsupported)
{
    char in[] = "x";
    size_t in_size = 1;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_static(file.get(), in, in_size), MB_FILE_OK);

    ASSERT_EQ(mb_file_truncate(file.get(), 10), MB_FILE_UNSUPPORTED);
    ASSERT_EQ(mb_file_error(file.get()), MB_FILE_ERROR_UNSUPPORTED);
    ASSERT_TRUE(strstr(mb_file_error_string(file.get()), "truncate"));
}

TEST(FileDynamicMemoryTest, OpenFile)
{
    void *in = nullptr;
    size_t in_size = 0;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_dynamic(file.get(), &in, &in_size),
              MB_FILE_OK);

    free(in);
}

TEST(FileDynamicMemoryTest, CloseFile)
{
    void *in = nullptr;
    size_t in_size = 0;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_dynamic(file.get(), &in, &in_size),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_close(file.get()), MB_FILE_OK);

    free(in);
}

TEST(FileDynamicMemoryTest, ReadInBounds)
{
    void *in = strdup("x");
    size_t in_size = 1;
    char out[1];
    size_t out_size;

    ASSERT_NE(in, nullptr);

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_dynamic(file.get(), &in, &in_size),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_read(file.get(), out, sizeof(out), &out_size),
              MB_FILE_OK);
    ASSERT_EQ(out_size, 1);
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

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_dynamic(file.get(), &in, &in_size),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_seek(file.get(), 10, SEEK_SET, nullptr), MB_FILE_OK);

    ASSERT_EQ(mb_file_read(file.get(), out, sizeof(out), &out_size),
              MB_FILE_OK);
    ASSERT_EQ(out_size, 0);

    free(in);
}

TEST(FileDynamicMemoryTest, ReadEmpty)
{
    void *in = nullptr;
    size_t in_size = 0;
    char out[1];
    size_t out_size;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_dynamic(file.get(), &in, &in_size),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_read(file.get(), out, sizeof(out), &out_size),
              MB_FILE_OK);
    ASSERT_EQ(out_size, 0);

    free(in);
}

TEST(FileDynamicMemoryTest, ReadTooLarge)
{
    void *in = strdup("x");
    size_t in_size = 1;
    char out[1];
    size_t out_size;

    ASSERT_NE(in, nullptr);

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_dynamic(file.get(), &in, &in_size),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_read(file.get(), out, sizeof(out), &out_size),
              MB_FILE_OK);
    ASSERT_EQ(out_size, 1);
    ASSERT_EQ(out[0], 'x');

    free(in);
}

TEST(FileDynamicMemoryTest, WriteInBounds)
{
    void *in = strdup("x");
    size_t in_size = 1;
    size_t n;

    ASSERT_NE(in, nullptr);

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_dynamic(file.get(), &in, &in_size),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_write(file.get(), "y", 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(static_cast<char *>(in)[0], 'y');

    free(in);
}

TEST(FileDynamicMemoryTest, WriteOutOfBounds)
{
    void *in = strdup("x");
    size_t in_size = 1;
    size_t n;

    ASSERT_NE(in, nullptr);

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_dynamic(file.get(), &in, &in_size),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_seek(file.get(), 10, SEEK_SET, nullptr), MB_FILE_OK);

    ASSERT_EQ(mb_file_write(file.get(), "y", 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(in_size, 11);
    ASSERT_NE(in, nullptr);
    ASSERT_EQ(static_cast<char *>(in)[10], 'y');

    free(in);
}

TEST(FileDynamicMemoryTest, WriteEmpty)
{
    void *in = nullptr;
    size_t in_size = 0;
    size_t n;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_dynamic(file.get(), &in, &in_size),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_write(file.get(), "x", 1, &n), MB_FILE_OK);
    ASSERT_EQ(n, 1);
    ASSERT_EQ(in_size, 1);
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

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_dynamic(file.get(), &in, &in_size),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_write(file.get(), "yz", 2, &n), MB_FILE_OK);
    ASSERT_EQ(n, 2);
    ASSERT_EQ(in_size, 2);
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

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_dynamic(file.get(), &in, &in_size),
              MB_FILE_OK);

    // SEEK_SET
    ASSERT_EQ(mb_file_seek(file.get(), 10, SEEK_SET, &pos), MB_FILE_OK);
    ASSERT_EQ(pos, 10);

    // Positive SEEK_CUR
    ASSERT_EQ(mb_file_seek(file.get(), 10, SEEK_CUR, &pos), MB_FILE_OK);
    ASSERT_EQ(pos, 20);

    // Negative SEEK_CUR
    ASSERT_EQ(mb_file_seek(file.get(), -8, SEEK_CUR, &pos), MB_FILE_OK);
    ASSERT_EQ(pos, 12);

    // Positive SEEK_END (out of bounds)
    ASSERT_EQ(mb_file_seek(file.get(), 10, SEEK_END, &pos), MB_FILE_OK);
    ASSERT_EQ(pos, 36);

    // Negative SEEK_END
    ASSERT_EQ(mb_file_seek(file.get(), -18, SEEK_END, &pos), MB_FILE_OK);
    ASSERT_EQ(pos, 8);

    free(in);
}

TEST(FileDynamicMemoryTest, SeekInvalid)
{
    void *in = strdup("abcdefghijklmnopqrstuvwxyz");
    size_t in_size = 26;
    uint64_t pos;

    ASSERT_NE(in, nullptr);

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_dynamic(file.get(), &in, &in_size),
              MB_FILE_OK);

    // Negative SEEK_SET
    ASSERT_EQ(mb_file_seek(file.get(), -10, SEEK_SET, &pos), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(file.get()), MB_FILE_ERROR_INVALID_ARGUMENT);
    ASSERT_TRUE(strstr(mb_file_error_string(file.get()), "Invalid SEEK_SET"));

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_SET
    ASSERT_EQ(mb_file_seek(file.get(), static_cast<int64_t>(SIZE_MAX) + 1,
                           SEEK_SET, &pos), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(file.get()), MB_FILE_ERROR_INVALID_ARGUMENT);
    ASSERT_TRUE(strstr(mb_file_error_string(file.get()), "Invalid SEEK_SET"));
#endif

    // Negative out of range SEEK_CUR
    ASSERT_EQ(mb_file_seek(file.get(), -100, SEEK_CUR, &pos), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(file.get()), MB_FILE_ERROR_INVALID_ARGUMENT);
    ASSERT_TRUE(strstr(mb_file_error_string(file.get()), "Invalid SEEK_CUR"));

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_CUR
    ASSERT_EQ(mb_file_seek(file.get(), static_cast<int64_t>(SIZE_MAX) + 1,
                           SEEK_CUR, &pos), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(file.get()), MB_FILE_ERROR_INVALID_ARGUMENT);
    ASSERT_TRUE(strstr(mb_file_error_string(file.get()), "Invalid SEEK_CUR"));
#endif

    // Negative out of range SEEK_END
    ASSERT_EQ(mb_file_seek(file.get(), -100, SEEK_END, &pos), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(file.get()), MB_FILE_ERROR_INVALID_ARGUMENT);
    ASSERT_TRUE(strstr(mb_file_error_string(file.get()), "Invalid SEEK_END"));

#if INT64_MAX > SIZE_MAX
    // Positive out of range SEEK_END
    ASSERT_EQ(mb_file_seek(file.get(), static_cast<int64_t>(SIZE_MAX) + 1,
                           SEEK_END, &pos), MB_FILE_FAILED);
    ASSERT_EQ(mb_file_error(file.get()), MB_FILE_ERROR_INVALID_ARGUMENT);
    ASSERT_TRUE(strstr(mb_file_error_string(file.get()), "Invalid SEEK_END"));
#endif

    free(in);
}

TEST(FileDynamicMemoryTest, TruncateFile)
{
    void *in = strdup("x");
    size_t in_size = 1;

    ScopedFile file(mb_file_new(), mb_file_free);
    ASSERT_TRUE(!!file);
    ASSERT_EQ(mb_file_open_memory_dynamic(file.get(), &in, &in_size),
              MB_FILE_OK);

    ASSERT_EQ(mb_file_truncate(file.get(), 10), MB_FILE_OK);
    ASSERT_EQ(in_size, 10);
    // Dumb check to ensure that the new memory is initialized
    for (int i = 1; i < 10; ++i) {
        ASSERT_EQ(static_cast<char *>(in)[i], '\0');
    }

    ASSERT_EQ(mb_file_truncate(file.get(), 5), MB_FILE_OK);
    ASSERT_EQ(in_size, 5);

    free(in);
}
