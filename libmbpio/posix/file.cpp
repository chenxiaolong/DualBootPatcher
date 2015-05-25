/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "libmbpio/posix/file.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

namespace io
{
namespace posix
{

class FilePosix::Impl
{
public:
    FILE *fp = nullptr;
    int error;
    int errnoCode;
    std::string errnoString;
};

FilePosix::FilePosix() : m_impl(new Impl())
{
}

FilePosix::~FilePosix()
{
    if (isOpen()) {
        close();
    }
}

bool FilePosix::open(const char *filename, int mode)
{
    if (!filename) {
        m_impl->error = ErrorInvalidFilename;
        return false;
    }

    const char *fopenMode;
    switch (mode) {
    case OpenRead:
        fopenMode = "rb";
        break;
    case OpenWrite:
        fopenMode = "wb";
        break;
    case OpenAppend:
        fopenMode = "ab";
        break;
    default:
        m_impl->error = ErrorInvalidOpenMode;
        return false;
    }

    m_impl->fp = fopen64(filename, fopenMode);
    if (!m_impl->fp) {
        m_impl->error = ErrorPlatformError;
        m_impl->errnoCode = errno;
        m_impl->errnoString = strerror(errno);
        return false;
    }

    return true;
}

bool FilePosix::open(const std::string &filename, int mode)
{
    return open(filename.c_str(), mode);
}

bool FilePosix::close()
{
    if (!m_impl->fp) {
        m_impl->error = ErrorFileIsNotOpen;
        return false;
    }

    if (fclose(m_impl->fp) == EOF) {
        m_impl->error = ErrorPlatformError;
        m_impl->errnoCode = errno;
        m_impl->errnoString = strerror(errno);
        return false;
    }

    return true;
}

bool FilePosix::isOpen()
{
    return m_impl->fp != nullptr;
}

bool FilePosix::read(void *buf, uint64_t size, uint64_t *bytesRead)
{
    std::size_t n = fread(buf, 1, size, m_impl->fp);
    *bytesRead = n;

    // We want to return true on a short read count so the caller can use the
    // last bit of data. Then return false when fread returns 0.
    if (n < size) {
        if (feof(m_impl->fp)) {
            m_impl->error = ErrorEndOfFile;
        } else if (ferror(m_impl->fp)) {
            m_impl->error = ErrorPlatformError;
            m_impl->errnoCode = errno;
            m_impl->errnoString = strerror(errno);
        }
        if (n == 0) {
            return false;
        }
    }

    return true;
}

bool FilePosix::write(const void *buf, uint64_t size, uint64_t *bytesWritten)
{
    std::size_t n = fwrite(buf, 1, size, m_impl->fp);
    *bytesWritten = n;

    if (n < size) {
        m_impl->error = ErrorPlatformError;
        m_impl->errnoCode = errno;
        m_impl->errnoString = strerror(errno);
        return false;
    }

    return true;
}

bool FilePosix::tell(uint64_t *pos)
{
    off64_t offset = ftello64(m_impl->fp);
    if (offset < 0) {
        m_impl->error = ErrorPlatformError;
        m_impl->errnoCode = errno;
        m_impl->errnoString = strerror(errno);
        return false;
    }

    *pos = offset;
    return true;
}

bool FilePosix::seek(int64_t offset, int origin)
{
    int whence;
    switch (origin) {
    case SeekCurrent:
        whence = SEEK_CUR;
        break;
    case SeekBegin:
        whence = SEEK_SET;
        break;
    case SeekEnd:
        whence = SEEK_END;
        break;
    default:
        m_impl->error = ErrorInvalidSeekOrigin;
        return false;
    }

    int ret = fseeko64(m_impl->fp, offset, whence);
    if (ret < 0) {
        m_impl->error = ErrorPlatformError;
        m_impl->errnoCode = errno;
        m_impl->errnoString = strerror(errno);
        return false;
    }

    return true;
}

int FilePosix::error()
{
    return m_impl->error;
}

std::string FilePosix::platformErrorString()
{
    return m_impl->errnoString;
}

}
}
