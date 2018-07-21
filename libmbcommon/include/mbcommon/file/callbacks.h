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

#pragma once

#include <functional>

#include "mbcommon/file.h"

namespace mb
{

class MB_EXPORT CallbackFile : public File
{
public:
    using OpenCb = std::function<oc::result<void>(File &file)>;
    using CloseCb = std::function<oc::result<void>(File &file)>;
    using ReadCb = std::function<oc::result<size_t>(
            File &file, void *buf, size_t size)>;
    using WriteCb = std::function<oc::result<size_t>(
            File &file, const void *buf, size_t size)>;
    using SeekCb = std::function<oc::result<uint64_t>(
            File &file, int64_t offset, int whence)>;
    using TruncateCb = std::function<oc::result<void>(
            File &file, uint64_t size)>;

    CallbackFile();
    CallbackFile(OpenCb open_cb,
                 CloseCb close_cb,
                 ReadCb read_cb,
                 WriteCb write_cb,
                 SeekCb seek_cb,
                 TruncateCb truncate_cb);
    virtual ~CallbackFile();

    CallbackFile(CallbackFile &&other) noexcept;
    CallbackFile & operator=(CallbackFile &&rhs) noexcept;

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(CallbackFile)

    oc::result<void> open(OpenCb open_cb,
                          CloseCb close_cb,
                          ReadCb read_cb,
                          WriteCb write_cb,
                          SeekCb seek_cb,
                          TruncateCb truncate_cb);

protected:
    oc::result<void> on_open() override;
    oc::result<void> on_close() override;
    oc::result<size_t> on_read(void *buf, size_t size) override;
    oc::result<size_t> on_write(const void *buf, size_t size) override;
    oc::result<uint64_t> on_seek(int64_t offset, int whence) override;
    oc::result<void> on_truncate(uint64_t size) override;

private:
    /*! \cond INTERNAL */
    void clear();

    OpenCb m_open_cb;
    CloseCb m_close_cb;
    ReadCb m_read_cb;
    WriteCb m_write_cb;
    SeekCb m_seek_cb;
    TruncateCb m_truncate_cb;
    /*! \endcond */
};

}
