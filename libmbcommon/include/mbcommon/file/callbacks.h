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

#pragma once

#include "mbcommon/file.h"

namespace mb
{

class CallbackFilePrivate;
class MB_EXPORT CallbackFile : public File
{
    MB_DECLARE_PRIVATE(CallbackFile)

public:
    typedef bool (*OpenCb)(File &file, void *userdata);
    typedef bool (*CloseCb)(File &file, void *userdata);
    typedef bool (*ReadCb)(File &file, void *userdata,
                           void *buf, size_t size,
                           size_t &bytes_read);
    typedef bool (*WriteCb)(File &file, void *userdata,
                            const void *buf, size_t size,
                            size_t &bytes_written);
    typedef bool (*SeekCb)(File &file, void *userdata,
                           int64_t offset, int whence, uint64_t &new_offset);
    typedef bool (*TruncateCb)(File &file, void *userdata,
                               uint64_t size);

    CallbackFile();
    CallbackFile(OpenCb open_cb,
                 CloseCb close_cb,
                 ReadCb read_cb,
                 WriteCb write_cb,
                 SeekCb seek_cb,
                 TruncateCb truncate_cb,
                 void *userdata);
    virtual ~CallbackFile();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(CallbackFile)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(CallbackFile)

    bool open(OpenCb open_cb,
              CloseCb close_cb,
              ReadCb read_cb,
              WriteCb write_cb,
              SeekCb seek_cb,
              TruncateCb truncate_cb,
              void *userdata);

protected:
    /*! \cond INTERNAL */
    CallbackFile(CallbackFilePrivate *priv);
    CallbackFile(CallbackFilePrivate *priv,
                 OpenCb open_cb,
                 CloseCb close_cb,
                 ReadCb read_cb,
                 WriteCb write_cb,
                 SeekCb seek_cb,
                 TruncateCb truncate_cb,
                 void *userdata);
    /*! \endcond */

    virtual bool on_open() override;
    virtual bool on_close() override;
    virtual bool on_read(void *buf, size_t size,
                         size_t &bytes_read) override;
    virtual bool on_write(const void *buf, size_t size,
                          size_t &bytes_written) override;
    virtual bool on_seek(int64_t offset, int whence,
                         uint64_t &new_offset) override;
    virtual bool on_truncate(uint64_t size) override;
};

}
