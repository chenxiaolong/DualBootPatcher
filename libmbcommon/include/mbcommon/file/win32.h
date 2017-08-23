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

#include "mbcommon/file/open_mode.h"

#include <windows.h>

namespace mb
{

class Win32FilePrivate;
class MB_EXPORT Win32File : public File
{
    MB_DECLARE_PRIVATE(Win32File)

public:
    Win32File();
    Win32File(HANDLE handle, bool owned, bool append);
    Win32File(const std::string &filename, FileOpenMode mode);
    Win32File(const std::wstring &filename, FileOpenMode mode);
    virtual ~Win32File();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(Win32File)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(Win32File)

    bool open(HANDLE handle, bool owned, bool append);
    bool open(const std::string &filename, FileOpenMode mode);
    bool open(const std::wstring &filename, FileOpenMode mode);

protected:
    /*! \cond INTERNAL */
    Win32File(Win32FilePrivate *priv);
    Win32File(Win32FilePrivate *priv,
              HANDLE handle, bool owned, bool append);
    Win32File(Win32FilePrivate *priv,
              const std::string &filename, FileOpenMode mode);
    Win32File(Win32FilePrivate *priv,
              const std::wstring &filename, FileOpenMode mode);
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
