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

#include <cinttypes>

#include "mbcommon/file.h"
#include "mbcommon/file/callbacks.h"

using namespace mb;

struct FileCallbacksTest : testing::Test
{
    unsigned int _n_open_cb = 0;
    unsigned int _n_close_cb = 0;
    unsigned int _n_read_cb = 0;
    unsigned int _n_write_cb = 0;
    unsigned int _n_seek_cb = 0;
    unsigned int _n_truncate_cb = 0;

    static oc::result<void> _open_cb(File &file, void *userdata)
    {
        (void) file;

        auto *ctx = static_cast<FileCallbacksTest *>(userdata);
        ++ctx->_n_open_cb;

        return oc::success();
    }

    static oc::result<void> _close_cb(File &file, void *userdata)
    {
        (void) file;

        auto *ctx = static_cast<FileCallbacksTest *>(userdata);
        ++ctx->_n_close_cb;

        return oc::success();
    }

    static oc::result<size_t> _read_cb(File &file, void *userdata,
                                       void *buf, size_t size)
    {
        (void) file;
        (void) buf;
        (void) size;

        auto *ctx = static_cast<FileCallbacksTest *>(userdata);
        ++ctx->_n_read_cb;

        return std::error_code{};
    }

    static oc::result<size_t> _write_cb(File &file, void *userdata,
                                        const void *buf, size_t size)
    {
        (void) file;
        (void) buf;
        (void) size;

        auto *ctx = static_cast<FileCallbacksTest *>(userdata);
        ++ctx->_n_write_cb;

        return std::error_code{};
    }

    static oc::result<uint64_t> _seek_cb(File &file, void *userdata,
                                         int64_t offset, int whence)
    {
        (void) file;
        (void) offset;
        (void) whence;

        auto *ctx = static_cast<FileCallbacksTest *>(userdata);
        ++ctx->_n_seek_cb;

        return std::error_code{};
    }

    static oc::result<void> _truncate_cb(File &file, void *userdata,
                                         uint64_t size)
    {
        (void) file;
        (void) size;

        auto *ctx = static_cast<FileCallbacksTest *>(userdata);
        ++ctx->_n_truncate_cb;

        return std::error_code{};
    }
};

TEST_F(FileCallbacksTest, CheckCallbackConstructorWorks)
{
    CallbackFile file(_open_cb, _close_cb, _read_cb, _write_cb, _seek_cb,
                      _truncate_cb, this);

    ASSERT_TRUE(file.is_open());
    ASSERT_FALSE(file.read(nullptr, 0));
    ASSERT_FALSE(file.write(nullptr, 0));
    ASSERT_FALSE(file.seek(0, SEEK_SET));
    ASSERT_FALSE(file.truncate(0));
    ASSERT_TRUE(file.close());

    ASSERT_EQ(_n_open_cb, 1u);
    ASSERT_EQ(_n_close_cb, 1u);
    ASSERT_EQ(_n_read_cb, 1u);
    ASSERT_EQ(_n_write_cb, 1u);
    ASSERT_EQ(_n_seek_cb, 1u);
    ASSERT_EQ(_n_truncate_cb, 1u);
}

TEST_F(FileCallbacksTest, CheckOpenFunctionWorks)
{
    CallbackFile file;

    ASSERT_TRUE(file.open(_open_cb, _close_cb, _read_cb, _write_cb, _seek_cb,
                          _truncate_cb, this));
    ASSERT_FALSE(file.read(nullptr, 0));
    ASSERT_FALSE(file.write(nullptr, 0));
    ASSERT_FALSE(file.seek(0, SEEK_SET));
    ASSERT_FALSE(file.truncate(0));
    ASSERT_TRUE(file.close());

    ASSERT_EQ(_n_open_cb, 1u);
    ASSERT_EQ(_n_close_cb, 1u);
    ASSERT_EQ(_n_read_cb, 1u);
    ASSERT_EQ(_n_write_cb, 1u);
    ASSERT_EQ(_n_seek_cb, 1u);
    ASSERT_EQ(_n_truncate_cb, 1u);
}

TEST_F(FileCallbacksTest, CheckReopenWorks)
{
    CallbackFile file;

    ASSERT_TRUE(file.open(_open_cb, _close_cb, _read_cb, _write_cb, _seek_cb,
                          _truncate_cb, this));
    ASSERT_TRUE(file.close());
    ASSERT_TRUE(file.open(_open_cb, _close_cb, _read_cb, _write_cb, _seek_cb,
                          _truncate_cb, this));
}

TEST_F(FileCallbacksTest, CheckMoveOpensAndClosesProperly)
{
    CallbackFile file1(_open_cb, _close_cb, _read_cb, _write_cb, _seek_cb,
                       _truncate_cb, this);
    CallbackFile file2(_open_cb, _close_cb, _read_cb, _write_cb, _seek_cb,
                       _truncate_cb, this);

    ASSERT_TRUE(file1.is_open());
    ASSERT_TRUE(file2.is_open());

    file2 = std::move(file1);
    ASSERT_TRUE(file2.is_open());

    ASSERT_EQ(_n_open_cb, 2u);
    ASSERT_EQ(_n_close_cb, 1u);
}

TEST_F(FileCallbacksTest, CheckMoveConstructor)
{
    CallbackFile file1(_open_cb, _close_cb, _read_cb, _write_cb, _seek_cb,
                       _truncate_cb, this);
    ASSERT_TRUE(file1.is_open());
    ASSERT_EQ(_n_open_cb, 1u);

    CallbackFile file2(std::move(file1));
    ASSERT_TRUE(file2.is_open());
    ASSERT_FALSE(file1.is_open());
    ASSERT_EQ(_n_open_cb, 1u);
    ASSERT_EQ(_n_close_cb, 0u);

    auto ret = file1.read(nullptr, 0);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), FileError::InvalidState);

    file1 = CallbackFile(_open_cb, _close_cb, _read_cb, _write_cb, _seek_cb,
                         _truncate_cb, this);
    ASSERT_TRUE(file1.is_open());
    ASSERT_EQ(_n_open_cb, 2u);
    ASSERT_EQ(_n_close_cb, 0u);
}

TEST_F(FileCallbacksTest, CheckMoveAssignment)
{
    CallbackFile file1(_open_cb, _close_cb, _read_cb, _write_cb, _seek_cb,
                       _truncate_cb, this);
    CallbackFile file2(_open_cb, _close_cb, _read_cb, _write_cb, _seek_cb,
                       _truncate_cb, this);
    ASSERT_TRUE(file1.is_open());
    ASSERT_TRUE(file2.is_open());
    ASSERT_EQ(_n_open_cb, 2u);

    file2 = std::move(file1);
    ASSERT_EQ(_n_open_cb, 2u);
    ASSERT_EQ(_n_close_cb, 1u);

    ASSERT_TRUE(file2.is_open());
    ASSERT_FALSE(file1.is_open());

    auto ret = file1.read(nullptr, 0);
    ASSERT_FALSE(ret);
    ASSERT_EQ(ret.error(), FileError::InvalidState);

    // Bring File1 back to life
    file1 = CallbackFile(_open_cb, _close_cb, _read_cb, _write_cb, _seek_cb,
                         _truncate_cb, this);
    ASSERT_EQ(_n_open_cb, 3u);
    ASSERT_EQ(_n_close_cb, 1u);
}
