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
#include "mbcommon/file/posix_p.h"

#include <cstdio>

namespace mb
{

class MB_EXPORT PosixFile : public File
{
public:
    PosixFile();
    PosixFile(FILE *fp, bool owned);
    PosixFile(const std::string &filename, FileOpenMode mode);
    PosixFile(const std::wstring &filename, FileOpenMode mode);
    virtual ~PosixFile();

    PosixFile(PosixFile &&other) noexcept;
    PosixFile & operator=(PosixFile &&rhs) noexcept;

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(PosixFile)

    oc::result<void> open(FILE *fp, bool owned);
    oc::result<void> open(const std::string &filename, FileOpenMode mode);
    oc::result<void> open(const std::wstring &filename, FileOpenMode mode);

protected:
    /*! \cond INTERNAL */
    PosixFile(detail::PosixFileFuncs *funcs);
    PosixFile(detail::PosixFileFuncs *funcs,
              FILE *fp, bool owned);
    PosixFile(detail::PosixFileFuncs *funcs,
              const std::string &filename, FileOpenMode mode);
    PosixFile(detail::PosixFileFuncs *funcs,
              const std::wstring &filename, FileOpenMode mode);
    /*! \endcond */

    oc::result<void> on_open() override;
    oc::result<void> on_close() override;
    oc::result<size_t> on_read(void *buf, size_t size) override;
    oc::result<size_t> on_write(const void *buf, size_t size) override;
    oc::result<uint64_t> on_seek(int64_t offset, int whence) override;
    oc::result<void> on_truncate(uint64_t size) override;

private:
    /*! \cond INTERNAL */
    void clear();

    detail::PosixFileFuncs *m_funcs;

    FILE *m_fp;
    bool m_owned;
#ifdef _WIN32
    std::wstring m_filename;
    const wchar_t *m_mode;
#else
    std::string m_filename;
    const char *m_mode;
#endif

    bool m_can_seek;
    /*! \endcond */
};

}
