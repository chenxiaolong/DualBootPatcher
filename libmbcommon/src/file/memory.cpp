/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include "mbcommon/file/memory_p.h"
#include "mbcommon/string.h"

/*!
 * \file mbcommon/file/memory.h
 * \brief Open file from memory
 */

namespace mb
{

/*! \cond INTERNAL */

MemoryFilePrivate::MemoryFilePrivate()
{
    clear();
}

MemoryFilePrivate::~MemoryFilePrivate() = default;

void MemoryFilePrivate::clear()
{
    data = nullptr;
    size = 0;
    data_ptr = nullptr;
    size_ptr = nullptr;
    pos = 0;
    fixed_size = false;
}

/*! \endcond */

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
    : MemoryFile(new MemoryFilePrivate())
{
}

/*!
 * \brief Open File handle from fixed size memory buffer.
 *
 * Construct the file handle and open the file. Use is_open() to check if the
 * file was successfully opened.
 *
 * \sa open(const void *, size_t)
 *
 * \param buf Data buffer
 * \param size Size of data buffer
 */
MemoryFile::MemoryFile(const void *buf, size_t size)
    : MemoryFile(new MemoryFilePrivate(), buf, size)
{
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
    : MemoryFile(new MemoryFilePrivate(), buf_ptr, size_ptr)
{
}

/*! \cond INTERNAL */

MemoryFile::MemoryFile(MemoryFilePrivate *priv)
    : File(priv)
{
}

MemoryFile::MemoryFile(MemoryFilePrivate *priv,
                       const void *buf, size_t size)
    : File(priv)
{
    (void) open(buf, size);
}

MemoryFile::MemoryFile(MemoryFilePrivate *priv,
                       void **buf_ptr, size_t *size_ptr)
    : File(priv)
{
    (void) open(buf_ptr, size_ptr);
}

/*! \endcond */

MemoryFile::~MemoryFile()
{
    (void) close();
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
oc::result<void> MemoryFile::open(const void *buf, size_t size)
{
    MB_PRIVATE(MemoryFile);
    if (priv) {
        priv->data = const_cast<void *>(buf);
        priv->size = size;
        priv->data_ptr = nullptr;
        priv->size_ptr = nullptr;
        priv->pos = 0;
        priv->fixed_size = true;
    }
    return File::open();
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
    MB_PRIVATE(MemoryFile);
    if (priv) {
        priv->data = *buf_ptr;
        priv->size = *size_ptr;
        priv->data_ptr = buf_ptr;
        priv->size_ptr = size_ptr;
        priv->pos = 0;
        priv->fixed_size = false;
    }
    return File::open();
}

oc::result<void> MemoryFile::on_close()
{
    MB_PRIVATE(MemoryFile);

    // Reset to allow opening another file
    priv->clear();

    return oc::success();
}

oc::result<size_t> MemoryFile::on_read(void *buf, size_t size)
{
    MB_PRIVATE(MemoryFile);

    size_t to_read = 0;
    if (priv->pos < priv->size) {
        to_read = std::min(priv->size - priv->pos, size);
    }

    memcpy(buf, static_cast<char *>(priv->data) + priv->pos, to_read);
    priv->pos += to_read;

    return to_read;
}

oc::result<size_t> MemoryFile::on_write(const void *buf, size_t size)
{
    MB_PRIVATE(MemoryFile);

    if (priv->pos > SIZE_MAX - size) {
        return FileError::ArgumentOutOfRange;
    }

    size_t desired_size = priv->pos + size;
    size_t to_write = size;

    if (desired_size > priv->size) {
        if (priv->fixed_size) {
            to_write = priv->pos <= priv->size ? priv->size - priv->pos : 0;
        } else {
            // Enlarge buffer
            void *new_data = realloc(priv->data, desired_size);
            if (!new_data) {
                return ec_from_errno();
            }

            // Zero-initialize new space
            memset(static_cast<char *>(new_data) + priv->size, 0,
                   desired_size - priv->size);

            priv->data = new_data;
            priv->size = desired_size;
            if (priv->data_ptr) {
                *priv->data_ptr = priv->data;
            }
            if (priv->size_ptr) {
                *priv->size_ptr = priv->size;
            }
        }
    }

    memcpy(static_cast<char *>(priv->data) + priv->pos, buf, to_write);
    priv->pos += to_write;

    return to_write;
}

oc::result<uint64_t> MemoryFile::on_seek(int64_t offset, int whence)
{
    MB_PRIVATE(MemoryFile);

    switch (whence) {
    case SEEK_SET:
        if (offset < 0 || static_cast<uint64_t>(offset) > SIZE_MAX) {
            return FileError::ArgumentOutOfRange;
        }
        return priv->pos = static_cast<size_t>(offset);
    case SEEK_CUR:
        if ((offset < 0 && static_cast<uint64_t>(-offset) > priv->pos)
                || (offset > 0 && static_cast<uint64_t>(offset)
                        > SIZE_MAX - priv->pos)) {
            return FileError::ArgumentOutOfRange;
        }
        return priv->pos += static_cast<size_t>(offset);
    case SEEK_END:
        if ((offset < 0 && static_cast<size_t>(-offset) > priv->size)
                || (offset > 0 && static_cast<uint64_t>(offset)
                        > SIZE_MAX - priv->size)) {
            return FileError::ArgumentOutOfRange;
        }
        return priv->pos = priv->size + static_cast<size_t>(offset);
    default:
        MB_UNREACHABLE("Invalid whence argument: %d", whence);
    }
}

oc::result<void> MemoryFile::on_truncate(uint64_t size)
{
    MB_PRIVATE(MemoryFile);

    if (priv->fixed_size) {
        // Cannot truncate fixed buffer
        return FileError::UnsupportedTruncate;
    } else {
        void *new_data = realloc(priv->data, static_cast<size_t>(size));
        if (!new_data) {
            return ec_from_errno();
        }

        // Zero-initialize new space
        if (size > priv->size) {
            memset(static_cast<char *>(new_data) + priv->size, 0,
                   static_cast<size_t>(size) - priv->size);
        }

        priv->data = new_data;
        priv->size = static_cast<size_t>(size);
        if (priv->data_ptr) {
            *priv->data_ptr = priv->data;
        }
        if (priv->size_ptr) {
            *priv->size_ptr = priv->size;
        }
    }

    return oc::success();
}

}
