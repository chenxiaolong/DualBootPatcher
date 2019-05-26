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

#include "mbcommon/file/open_mode.h"
#include "mbcommon/file/win32_p.h"

#include <windows.h>

namespace mb
{

class MB_EXPORT Win32File : public File
{
public:
    Win32File();
    Win32File(HANDLE handle, bool owned, bool append);
    Win32File(const std::string &filename, FileOpenMode mode);
    Win32File(const std::wstring &filename, FileOpenMode mode);
    virtual ~Win32File();

    Win32File(Win32File &&other) noexcept;
    Win32File & operator=(Win32File &&rhs) noexcept;

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(Win32File)

    oc::result<void> open(HANDLE handle, bool owned, bool append);
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
    Win32File(detail::Win32FileFuncs *funcs);
    Win32File(detail::Win32FileFuncs *funcs,
              HANDLE handle, bool owned, bool append);
    Win32File(detail::Win32FileFuncs *funcs,
              const std::string &filename, FileOpenMode mode);
    Win32File(detail::Win32FileFuncs *funcs,
              const std::wstring &filename, FileOpenMode mode);
    /*! \endcond */

private:
    /*! \cond INTERNAL */
    oc::result<void> open();

    void clear() noexcept;

    detail::Win32FileFuncs *m_funcs;

    HANDLE m_handle;
    bool m_owned;
    std::wstring m_filename;

    DWORD m_access;
    DWORD m_sharing;
    SECURITY_ATTRIBUTES m_sa;
    DWORD m_creation;
    DWORD m_attrib;

    bool m_append;
    /*! \endcond */
};

}
