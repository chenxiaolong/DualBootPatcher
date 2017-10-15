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

#include "mbcommon/common.h"

#include <memory>
#include <string>

#include <cstdarg>
#include <cstddef>
#include <cstdint>

#include "mbcommon/error/expected.h"

namespace mb
{

class FilePrivate;
class MB_EXPORT File
{
    MB_DECLARE_PRIVATE(File)

public:
    File();
    virtual ~File();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(File)

    File(File &&other) noexcept;
    File & operator=(File &&rhs) noexcept;

    // File close
    Expected<void> close();

    // File operations
    Expected<size_t> read(void *buf, size_t size);
    Expected<size_t> write(const void *buf, size_t size);
    Expected<uint64_t> seek(int64_t offset, int whence);
    Expected<void> truncate(uint64_t size);

    // File state
    bool is_open();
    bool is_fatal();
    bool set_fatal(bool fatal);

protected:
    /*! \cond INTERNAL */
    File(FilePrivate *priv);
    /*! \endcond */

    // File open
    Expected<void> open();

    virtual Expected<void> on_open();
    virtual Expected<void> on_close();
    virtual Expected<size_t> on_read(void *buf, size_t size);
    virtual Expected<size_t> on_write(const void *buf, size_t size);
    virtual Expected<uint64_t> on_seek(int64_t offset, int whence);
    virtual Expected<void> on_truncate(uint64_t size);

    std::unique_ptr<FilePrivate> _priv_ptr;
};

}
