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

#include "mbcommon/guard_p.h"

#include "mbcommon/file/fd.h"
#include "mbcommon/file_p.h"

/*! \cond INTERNAL */
namespace mb
{

struct FdFileFuncs
{
    virtual ~FdFileFuncs();

    // fcntl.h
#ifdef _WIN32
    virtual int fn_wopen(const wchar_t *path, int flags, mode_t mode) = 0;
#else
    virtual int fn_open(const char *path, int flags, mode_t mode) = 0;
#endif

    // sys/stat.h
    virtual int fn_fstat(int fildes, struct stat *buf) = 0;

    // unistd.h
    virtual int fn_close(int fd) = 0;
    virtual int fn_ftruncate64(int fd, off_t length) = 0;
    virtual off64_t fn_lseek64(int fd, off64_t offset, int whence) = 0;
    virtual ssize_t fn_read(int fd, void *buf, size_t count) = 0;
    virtual ssize_t fn_write(int fd, const void *buf, size_t count) = 0;
};

class FdFilePrivate : public FilePrivate
{
public:
    FdFilePrivate();
    virtual ~FdFilePrivate();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(FdFilePrivate)

    void clear();

    static int convert_mode(FileOpenMode mode);

    FdFileFuncs *funcs;

    int fd;
    bool owned;
#ifdef _WIN32
    std::wstring filename;
#else
    std::string filename;
#endif
    int flags;

protected:
    FdFilePrivate(FdFileFuncs *funcs);
};

}
/*! \endcond */
