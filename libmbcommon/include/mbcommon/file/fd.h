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

#include "mbcommon/file.h"

#include "mbcommon/file/fd_p.h"
#include "mbcommon/file/open_mode.h"

namespace mb
{

class MB_EXPORT FdFile : public File
{
public:
    FdFile();
    FdFile(int fd, bool owned);
    FdFile(const std::string &filename, FileOpenMode mode);
    FdFile(const std::wstring &filename, FileOpenMode mode);
    virtual ~FdFile();

    FdFile(FdFile &&other) noexcept;
    FdFile & operator=(FdFile &&rhs) noexcept;

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(FdFile)

    oc::result<void> open(int fd, bool owned);
    oc::result<void> open(const std::string &filename, FileOpenMode mode);
    oc::result<void> open(const std::wstring &filename, FileOpenMode mode);

    oc::result<void> close() override;

    oc::result<size_t> read(void *buf, size_t size) override;
    oc::result<size_t> write(const void *buf, size_t size) override;
    oc::result<uint64_t> seek(int64_t offset, int whence) override;
    oc::result<void> truncate(uint64_t size) override;

    bool is_open() override;

protected:
    /*! \cond INTERNAL */
    FdFile(detail::FdFileFuncs *funcs);
    FdFile(detail::FdFileFuncs *funcs,
           int fd, bool owned);
    FdFile(detail::FdFileFuncs *funcs,
           const std::string &filename, FileOpenMode mode);
    FdFile(detail::FdFileFuncs *funcs,
           const std::wstring &filename, FileOpenMode mode);
    /*! \endcond */

private:
    /*! \cond INTERNAL */
    oc::result<void> open();

    void clear() noexcept;

    detail::FdFileFuncs *m_funcs;

    int m_fd;
    bool m_owned;
#ifdef _WIN32
    std::wstring m_filename;
#else
    std::string m_filename;
#endif
    int m_flags;
    /*! \endcond */
};

}
