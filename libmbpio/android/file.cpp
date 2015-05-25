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

#include "libmbpio/android/file.h"

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>

// TODO: Switch back to buffered API when bionic gets LFS support

namespace io
{
namespace android
{

class FileAndroid::Impl
{
public:
    int fd = -1;
    int error;
    int errnoCode;
    std::string errnoString;
};

FileAndroid::FileAndroid() : m_impl(new Impl())
{
}

FileAndroid::~FileAndroid()
{
    if (isOpen()) {
        close();
    }
}

bool FileAndroid::open(const char *filename, int mode)
{
    if (!filename) {
        m_impl->error = ErrorInvalidFilename;
        return false;
    }

    int openMode;
    switch (mode) {
    case OpenRead:
        openMode = O_LARGEFILE | O_RDONLY;
        break;
    case OpenWrite:
        openMode = O_LARGEFILE | O_WRONLY | O_CREAT | O_TRUNC;
        break;
    case OpenAppend:
        openMode = O_LARGEFILE | O_APPEND | O_CREAT;
        break;
    default:
        m_impl->error = ErrorInvalidOpenMode;
        return false;
    }

    m_impl->fd = open64(filename, openMode, 0666);
    if (m_impl->fd < 0) {
        m_impl->error = ErrorPlatformError;
        m_impl->errnoCode = errno;
        m_impl->errnoString = strerror(errno);
        return false;
    }

    return true;
}

bool FileAndroid::open(const std::string &filename, int mode)
{
    return open(filename.c_str(), mode);
}

bool FileAndroid::close()
{
    if (m_impl->fd < 0) {
        m_impl->error = ErrorFileIsNotOpen;
        return false;
    }

    if (::close(m_impl->fd) < 0) {
        m_impl->error = ErrorPlatformError;
        m_impl->errnoCode = errno;
        m_impl->errnoString = strerror(errno);
        return false;
    }

    return true;
}

bool FileAndroid::isOpen()
{
    return m_impl->fd >= 0;
}

bool FileAndroid::read(void *buf, uint64_t size, uint64_t *bytesRead)
{
    ssize_t n = ::read(m_impl->fd, buf, size);
    if (n < 0) {
        m_impl->error = ErrorPlatformError;
        m_impl->errnoCode = errno;
        m_impl->errnoString = strerror(errno);
        return false;
    }

    *bytesRead = n;

    if (n == 0) {
        m_impl->error = ErrorEndOfFile;
        return false;
    }

    return true;
}

bool FileAndroid::write(const void *buf, uint64_t size, uint64_t *bytesWritten)
{
    ssize_t n = ::write(m_impl->fd, buf, size);
    if (n < 0) {
        m_impl->error = ErrorPlatformError;
        m_impl->errnoCode = errno;
        m_impl->errnoString = strerror(errno);
        return false;
    }

    *bytesWritten = n;

    return true;
}

bool FileAndroid::tell(uint64_t *pos)
{
    off64_t offset = lseek64(m_impl->fd, 0, SEEK_CUR);
    if (offset < 0) {
        m_impl->error = ErrorPlatformError;
        m_impl->errnoCode = errno;
        m_impl->errnoString = strerror(errno);
        return false;
    }

    *pos = offset;
    return true;
}

bool FileAndroid::seek(int64_t offset, int origin)
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

    int ret = lseek64(m_impl->fd, offset, whence);
    if (ret < 0) {
        m_impl->error = ErrorPlatformError;
        m_impl->errnoCode = errno;
        m_impl->errnoString = strerror(errno);
        return false;
    }

    return true;
}

int FileAndroid::error()
{
    return m_impl->error;
}

std::string FileAndroid::platformErrorString()
{
    return m_impl->errnoString;
}

}
}
