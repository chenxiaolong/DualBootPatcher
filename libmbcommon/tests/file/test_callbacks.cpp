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

#include <cinttypes>

#include "mbcommon/file.h"
#include "mbcommon/file_p.h"
#include "mbcommon/file/callbacks.h"

struct FileCallbacksTest : testing::Test
{
    MbFile *_file;

    FileCallbacksTest() : _file(mb_file_new())
    {
    }

    virtual ~FileCallbacksTest()
    {
        mb_file_free(_file);
    }

    static int _open_cb(MbFile *file, void *userdata)
    {
        (void) file;
        (void) userdata;

        return MB_FILE_OK;
    }

    static int _close_cb(MbFile *file, void *userdata)
    {
        (void) file;
        (void) userdata;

        return MB_FILE_OK;
    }

    static int _read_cb(MbFile *file, void *userdata,
                        void *buf, size_t size,
                        size_t *bytes_read)
    {
        (void) file;
        (void) userdata;
        (void) buf;
        (void) size;
        (void) bytes_read;

        return MB_FILE_UNSUPPORTED;
    }

    static int _write_cb(MbFile *file, void *userdata,
                         const void *buf, size_t size,
                         size_t *bytes_written)
    {
        (void) file;
        (void) userdata;
        (void) buf;
        (void) size;
        (void) bytes_written;

        return MB_FILE_UNSUPPORTED;
    }

    static int _seek_cb(MbFile *file, void *userdata,
                        int64_t offset, int whence,
                        uint64_t *new_offset)
    {
        (void) file;
        (void) userdata;
        (void) offset;
        (void) whence;
        (void) new_offset;

        return MB_FILE_UNSUPPORTED;
    }

    static int _truncate_cb(MbFile *file, void *userdata,
                            uint64_t size)
    {
        (void) file;
        (void) userdata;
        (void) size;

        return MB_FILE_UNSUPPORTED;
    }
};

TEST_F(FileCallbacksTest, CheckCallbacksSet)
{
    ASSERT_EQ(mb_file_open_callbacks(_file,
                                     &_open_cb,
                                     &_close_cb,
                                     &_read_cb,
                                     &_write_cb,
                                     &_seek_cb,
                                     &_truncate_cb,
                                     this), MB_FILE_OK);

    ASSERT_EQ(_file->state, MbFileState::OPENED);
    ASSERT_EQ(_file->open_cb, &_open_cb);
    ASSERT_EQ(_file->close_cb, &_close_cb);
    ASSERT_EQ(_file->read_cb, &_read_cb);
    ASSERT_EQ(_file->write_cb, &_write_cb);
    ASSERT_EQ(_file->seek_cb, &_seek_cb);
    ASSERT_EQ(_file->truncate_cb, &_truncate_cb);
    ASSERT_EQ(_file->cb_userdata, this);
}
