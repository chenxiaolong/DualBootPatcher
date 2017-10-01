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

#include "mbcommon/file_error.h"

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
    bool close();

    // File operations
    bool read(void *buf, size_t size, size_t &bytes_read);
    bool write(const void *buf, size_t size, size_t &bytes_written);
    bool seek(int64_t offset, int whence, uint64_t *new_offset);
    bool truncate(uint64_t size);

    // File state
    bool is_open();
    bool is_fatal();
    bool set_fatal(bool fatal);

    // Error handling functions
    std::error_code error();
    std::string error_string();
    MB_PRINTF(3, 4)
    bool set_error(std::error_code ec, const char *fmt, ...);
    bool set_error_v(std::error_code ec, const char *fmt, va_list ap);

protected:
    /*! \cond INTERNAL */
    File(FilePrivate *priv);
    /*! \endcond */

    // File open
    bool open();

    virtual bool on_open();
    virtual bool on_close();
    virtual bool on_read(void *buf, size_t size, size_t &bytes_read);
    virtual bool on_write(const void *buf, size_t size, size_t &bytes_written);
    virtual bool on_seek(int64_t offset, int whence, uint64_t &new_offset);
    virtual bool on_truncate(uint64_t size);

    std::unique_ptr<FilePrivate> _priv_ptr;
};

}
