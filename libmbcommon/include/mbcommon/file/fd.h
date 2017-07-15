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

namespace mb
{

class FdFilePrivate;
class MB_EXPORT FdFile : public File
{
    MB_DECLARE_PRIVATE(FdFile)

public:
    FdFile();
    FdFile(int fd, bool owned);
    FdFile(const std::string &filename, FileOpenMode mode);
    FdFile(const std::wstring &filename, FileOpenMode mode);
    virtual ~FdFile();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(FdFile)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(FdFile)

    FileStatus open(int fd, bool owned);
    FileStatus open(const std::string &filename, FileOpenMode mode);
    FileStatus open(const std::wstring &filename, FileOpenMode mode);

protected:
    /*! \cond INTERNAL */
    FdFile(FdFilePrivate *priv);
    FdFile(FdFilePrivate *priv,
           int fd, bool owned);
    FdFile(FdFilePrivate *priv,
           const std::string &filename, FileOpenMode mode);
    FdFile(FdFilePrivate *priv,
           const std::wstring &filename, FileOpenMode mode);
    /*! \endcond */

    virtual FileStatus on_open() override;
    virtual FileStatus on_close() override;
    virtual FileStatus on_read(void *buf, size_t size,
                               size_t &bytes_read) override;
    virtual FileStatus on_write(const void *buf, size_t size,
                                size_t &bytes_written) override;
    virtual FileStatus on_seek(int64_t offset, int whence,
                               uint64_t &new_offset) override;
    virtual FileStatus on_truncate(uint64_t size) override;
};

}
