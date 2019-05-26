/*
 * Copyright (C) 2017-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file/memory.h"

#include <algorithm>

#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "mbcommon/error_code.h"
#include "mbcommon/file_error.h"
#include "mbcommon/string.h"

/*!
 * \file mbcommon/file/memory.h
 * \brief Open file from memory
 */

namespace mb
{

/*!
 * \class MemoryFile
 *
 * \brief Open file from statically or dynamically sized memory buffers.
 */

/*!
 * \brief Construct unbound MemoryFile.
 *
 * The File handle will not be bound to any file. One of the open functions will
 * need to be called to open a file.
 */
MemoryFile::MemoryFile()
    : File()
{
    clear();
}

/*!
 * \brief Open File handle from fixed size memory buffer.
 *
 * Construct the file handle and open the file. Use is_open() to check if the
 * file was successfully opened.
 *
 * \sa open(void *, size_t)
 *
 * \param buf Data buffer
 * \param size Size of data buffer
 */
MemoryFile::MemoryFile(void *buf, size_t size)
    : MemoryFile()
{
    (void) open(buf, size);
}

/*!
 * \brief Open File handle from dynamically sized memory buffer.
 *
 * Construct the file handle and open the file. Use is_open() to check if the
 * file was successfully opened.
 *
 * \sa open(void **, size_t *)
 *
 * \param[in,out] buf_ptr Pointer to data buffer
 * \param[in,out] size_ptr Pointer to size of data buffer
 */
MemoryFile::MemoryFile(void **buf_ptr, size_t *size_ptr)
    : MemoryFile()
{
    (void) open(buf_ptr, size_ptr);
}

MemoryFile::~MemoryFile()
{
    (void) close();
}

/*!
 * \brief Move construct new File handle.
 *
 * \p other will be left in a state as if it was newly constructed with the
 * default constructor.
 *
 * \param other File handle to move from
 */
MemoryFile::MemoryFile(MemoryFile &&other) noexcept
{
    clear();

    std::swap(m_is_open, other.m_is_open);
    std::swap(m_data, other.m_data);
    std::swap(m_size, other.m_size);
    std::swap(m_data_ptr, other.m_data_ptr);
    std::swap(m_size_ptr, other.m_size_ptr);
    std::swap(m_pos, other.m_pos);
    std::swap(m_fixed_size, other.m_fixed_size);
}

/*!
 * \brief Move assign a File handle
 *
 * This file handle will be closed and then \p rhs will be moved into this
 * object. \p rhs will be left in a state as if it was newly constructed with
 * the default constructor.
 *
 * \param rhs File handle to move from
 */
MemoryFile & MemoryFile::operator=(MemoryFile &&rhs) noexcept
{
    if (this != &rhs) {
        clear();

        std::swap(m_is_open, rhs.m_is_open);
        std::swap(m_data, rhs.m_data);
        std::swap(m_size, rhs.m_size);
        std::swap(m_data_ptr, rhs.m_data_ptr);
        std::swap(m_size_ptr, rhs.m_size_ptr);
        std::swap(m_pos, rhs.m_pos);
        std::swap(m_fixed_size, rhs.m_fixed_size);
    }

    return *this;
}

/*!
 * \brief Open from fixed size memory buffer.
 *
 * \param buf Data buffer
 * \param size Size of data buffer
 *
 * \return Nothing if the file is successfully opened. Otherwise, the error
 *         code.
 */
oc::result<void> MemoryFile::open(void *buf, size_t size)
{
    if (is_open()) return FileError::InvalidState;

    m_is_open = true;
    m_data = buf;
    m_size = size;
    m_data_ptr = nullptr;
    m_size_ptr = nullptr;
    m_pos = 0;
    m_fixed_size = true;

    return oc::success();
}

/*!
 * \brief Open from dynamically sized memory buffer.
 *
 * \param[in,out] buf_ptr Pointer to data buffer
 * \param[in,out] size_ptr Pointer to size of data buffer
 *
 * \return Nothing if the file is successfully opened. Otherwise, the error
 *         code.
 */
oc::result<void> MemoryFile::open(void **buf_ptr, size_t *size_ptr)
{
    if (is_open()) return FileError::InvalidState;

    m_is_open = true;
    m_data = *buf_ptr;
    m_size = *size_ptr;
    m_data_ptr = buf_ptr;
    m_size_ptr = size_ptr;
    m_pos = 0;
    m_fixed_size = false;

    return oc::success();
}

oc::result<void> MemoryFile::close()
{
    if (!is_open()) return FileError::InvalidState;

    // Reset to allow opening another file
    clear();

    return oc::success();
}

oc::result<size_t> MemoryFile::read(void *buf, size_t size)
{
    if (!is_open()) return FileError::InvalidState;

    size_t to_read = 0;
    if (m_pos < m_size) {
        to_read = std::min(m_size - m_pos, size);
    }

    memcpy(buf, static_cast<char *>(m_data) + m_pos, to_read);
    m_pos += to_read;

    return to_read;
}

oc::result<size_t> MemoryFile::write(const void *buf, size_t size)
{
    if (!is_open()) return FileError::InvalidState;

    if (m_pos > SIZE_MAX - size) {
        return FileError::ArgumentOutOfRange;
    }

    size_t desired_size = m_pos + size;
    size_t to_write = size;

    if (desired_size > m_size) {
        if (m_fixed_size) {
            to_write = m_pos <= m_size ? m_size - m_pos : 0;
        } else {
            // Enlarge buffer
            void *new_data = realloc(m_data, desired_size);
            if (!new_data) {
                return ec_from_errno();
            }

            // Zero-initialize new space
            std::fill_n(static_cast<char *>(new_data) + m_size,
                        desired_size - m_size, 0);

            m_data = new_data;
            m_size = desired_size;
            if (m_data_ptr) {
                *m_data_ptr = m_data;
            }
            if (m_size_ptr) {
                *m_size_ptr = m_size;
            }
        }
    }

    memcpy(static_cast<char *>(m_data) + m_pos, buf, to_write);
    m_pos += to_write;

    return to_write;
}

oc::result<uint64_t> MemoryFile::seek(int64_t offset, int whence)
{
    if (!is_open()) return FileError::InvalidState;

    switch (whence) {
    case SEEK_SET:
        if (offset < 0 || static_cast<uint64_t>(offset) > SIZE_MAX) {
            return FileError::ArgumentOutOfRange;
        }
        return m_pos = static_cast<size_t>(offset);
    case SEEK_CUR:
        if (offset < 0) {
            if (static_cast<uint64_t>(-offset) > m_pos) {
                return FileError::ArgumentOutOfRange;
            }
            return m_pos -= static_cast<size_t>(-offset);
        } else {
            if (static_cast<uint64_t>(offset) > SIZE_MAX - m_pos) {
                return FileError::ArgumentOutOfRange;
            }
            return m_pos += static_cast<size_t>(offset);
        }
    case SEEK_END:
        if (offset < 0) {
            if (static_cast<uint64_t>(-offset) > m_size) {
                return FileError::ArgumentOutOfRange;
            }
            return m_pos = m_size - static_cast<size_t>(-offset);
        } else {
            if (static_cast<uint64_t>(offset) > SIZE_MAX - m_size) {
                return FileError::ArgumentOutOfRange;
            }
            return m_pos = m_size + static_cast<size_t>(offset);
        }
    default:
        MB_UNREACHABLE("Invalid whence argument: %d", whence);
    }
}

oc::result<void> MemoryFile::truncate(uint64_t size)
{
    if (!is_open()) return FileError::InvalidState;

    if (m_fixed_size) {
        // Cannot truncate fixed buffer
        return FileError::UnsupportedTruncate;
    } else {
        void *new_data = realloc(m_data, static_cast<size_t>(size));
        if (!new_data) {
            return ec_from_errno();
        }

        // Zero-initialize new space
        if (size > m_size) {
            std::fill_n(static_cast<char *>(new_data) + m_size,
                        static_cast<size_t>(size) - m_size, 0);
        }

        m_data = new_data;
        m_size = static_cast<size_t>(size);
        if (m_data_ptr) {
            *m_data_ptr = m_data;
        }
        if (m_size_ptr) {
            *m_size_ptr = m_size;
        }
    }

    return oc::success();
}

bool MemoryFile::is_open()
{
    return m_is_open;
}

void MemoryFile::clear() noexcept
{
    m_is_open = false;
    m_data = nullptr;
    m_size = 0;
    m_data_ptr = nullptr;
    m_size_ptr = nullptr;
    m_pos = 0;
    m_fixed_size = false;
}

}
