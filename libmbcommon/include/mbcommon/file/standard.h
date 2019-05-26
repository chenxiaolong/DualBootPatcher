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

#if defined(_WIN32)
#  include "mbcommon/file/win32.h"
#  define STANDARD_FILE_IMPL Win32File
#elif defined(__ANDROID__)
#  include "mbcommon/file/fd.h"
#  define STANDARD_FILE_IMPL FdFile
#else
#  include "mbcommon/file/posix.h"
#  define STANDARD_FILE_IMPL PosixFile
#endif

namespace mb
{

class MB_EXPORT StandardFile : public File
{
public:
    StandardFile();
    StandardFile(const std::string &filename, FileOpenMode mode);
    StandardFile(const std::wstring &filename, FileOpenMode mode);
    virtual ~StandardFile();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(StandardFile)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(StandardFile)

    oc::result<void> open(const std::string &filename, FileOpenMode mode);
    oc::result<void> open(const std::wstring &filename, FileOpenMode mode);

    oc::result<void> close() override;

    oc::result<size_t> read(void *buf, size_t size) override;
    oc::result<size_t> write(const void *buf, size_t size) override;
    oc::result<uint64_t> seek(int64_t offset, int whence) override;
    oc::result<void> truncate(uint64_t size) override;

    bool is_open() override;

private:
    STANDARD_FILE_IMPL m_file;
};

}

#undef STANDARD_FILE_IMPL
