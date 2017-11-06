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

    oc::result<void> open(int fd, bool owned);
    oc::result<void> open(const std::string &filename, FileOpenMode mode);
    oc::result<void> open(const std::wstring &filename, FileOpenMode mode);

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

    oc::result<void> on_open() override;
    oc::result<void> on_close() override;
    oc::result<size_t> on_read(void *buf, size_t size) override;
    oc::result<size_t> on_write(const void *buf, size_t size) override;
    oc::result<uint64_t> on_seek(int64_t offset, int whence) override;
    oc::result<void> on_truncate(uint64_t size) override;
};

}
