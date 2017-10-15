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
    using OpenCb = Expected<void> (*)(File &file, void *userdata);
    using CloseCb = Expected<void> (*)(File &file, void *userdata);
    using ReadCb = Expected<size_t> (*)(File &file, void *userdata,
                                        void *buf, size_t size);
    using WriteCb = Expected<size_t> (*)(File &file, void *userdata,
                                         const void *buf, size_t size);
    using SeekCb = Expected<uint64_t> (*)(File &file, void *userdata,
                                          int64_t offset, int whence);
    using TruncateCb = Expected<void> (*)(File &file, void *userdata,
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

    Expected<void> open(OpenCb open_cb,
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

    Expected<void> on_open() override;
    Expected<void> on_close() override;
    Expected<size_t> on_read(void *buf, size_t size) override;
    Expected<size_t> on_write(const void *buf, size_t size) override;
    Expected<uint64_t> on_seek(int64_t offset, int whence) override;
    Expected<void> on_truncate(uint64_t size) override;
};

}
