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

#include "mbcommon/file_util.h"

#include <algorithm>
#include <functional>
#include <vector>
#ifdef __ANDROID__
#  include <experimental/algorithm>
#  include <experimental/functional>
#endif

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "mbcommon/error_code.h"
#include "mbcommon/file_error.h"

/*!
 * \file mbcommon/file_util.h
 * \brief Useful utility functions for File API
 */

namespace mb
{

using namespace detail;

/*!
 * \brief Read from a File handle.
 *
 * This function differs from File::read() in that it will call File::read()
 * repeatedly until \p size bytes are read or EOF is reached. If File::read()
 * returns std::errc::interrupted, then the read operation will be automatically
 * reattempted.
 *
 * If this function fails, the contents of \p buf are unspecified. It is also
 * unspecified how many bytes are read, though it will always be less than
 * \p size.
 *
 * \param[in] file File handle
 * \param[out] buf Buffer to read into
 * \param[in] size Buffer size
 *
 * \return Number of bytes read if some are successfully read or EOF is reached.
 *         Otherwise, the error code.
 */
oc::result<size_t> file_read_retry(File &file, void *buf, size_t size)
{
    size_t bytes_read = 0;

    while (bytes_read < size) {
        auto n = file.read(static_cast<char *>(buf) + bytes_read,
                           size - bytes_read);
        if (!n) {
            if (n.error() == std::errc::interrupted) {
                continue;
            } else {
                return n.as_failure();
            }
        } else if (n.value() == 0) {
            break;
        }

        bytes_read += n.value();
    }

    return bytes_read;
}

/*!
 * \brief Write to a File handle.
 *
 * This function differs from File::write() in that it will call File::write()
 * repeatedly until \p size bytes are written or EOF is reached. If
 * File::write() returns std::errc::interrupted, then the write operation is
 * automatically reattempted.
 *
 * If this function fails, it is unspecified how many bytes were written, though
 * it will always be less than \p size.
 *
 * \param file File handle
 * \param buf Buffer to write from
 * \param size Buffer size
 *
 * \return Number of bytes written if some are successfully written or EOF is
 *         reached. Otherwise, the error code.
 */
oc::result<size_t> file_write_retry(File &file, const void *buf, size_t size)
{
    size_t bytes_written = 0;

    while (bytes_written < size) {
        auto n = file.write(static_cast<const char *>(buf) + bytes_written,
                            size - bytes_written);
        if (!n) {
            if (n.error() == std::errc::interrupted) {
                continue;
            } else {
                return n.as_failure();
            }
        } else if (n.value() == 0) {
            break;
        }

        bytes_written += n.value();
    }

    return bytes_written;
}

/*!
 * \brief Read from a File handle.
 *
 * This function differs from File::read() in that it will call File::read()
 * repeatedly until \p size bytes are read. If File::read() returns
 * std::errc::interrupted, then the read operation will be automatically
 * reattempted. If EOF is reached before the specified number of bytes are read,
 * then FileError::UnexpectedEof will be returned.
 *
 * If this function fails, the contents of \p buf are unspecified. It is also
 * unspecified how many bytes are read, though it will always be less than
 * \p size.
 *
 * \param[in] file File handle
 * \param[out] buf Buffer to read into
 * \param[in] size Buffer size
 *
 * \return Nothing if the specified number of bytes were successfully read.
 *         Otherwise, the error code.
 */
oc::result<void> file_read_exact(File &file, void *buf, size_t size)
{
    OUTCOME_TRY(n, file_read_retry(file, buf, size));

    if (n != size) {
        return FileError::UnexpectedEof;
    }

    return oc::success();
}

/*!
 * \brief Write to a File handle.
 *
 * This function differs from File::write() in that it will call File::write()
 * repeatedly until \p size bytes are written. If File::write() returns
 * std::errc::interrupted, then the write operation will be automatically
 * reattempted. If EOF is reached before the specified number of bytes are read,
 * then FileError::UnexpectedEof will be returned.
 *
 * If this function fails, it is unspecified how many bytes were written, though
 * it will always be less than \p size.
 *
 * \param file File handle
 * \param buf Buffer to write from
 * \param size Buffer size
 *
 * \return Nothing if the specified number of bytes were successfully written.
 *         Otherwise, the error code.
 */
oc::result<void> file_write_exact(File &file, const void *buf, size_t size)
{
    OUTCOME_TRY(n, file_write_retry(file, buf, size));

    if (n != size) {
        return FileError::UnexpectedEof;
    }

    return oc::success();
}

/*!
 * \brief Read from a File handle and discard the data.
 *
 * This function will repeatedly call File::read() and discard any data that
 * is read. If File::read() returns std::errc::interrupted, then the read
 * operation will be automatically reattempted.
 *
 * \param file File handle
 * \param size Number of bytes to discard
 *
 * \return Number of bytes discarded if some are successfully read and discarded
 *         or EOF is reached. Otherwise, the error code.
 */
oc::result<uint64_t> file_read_discard(File &file, uint64_t size)
{
    char buf[10240];

    uint64_t bytes_discarded = 0;

    while (bytes_discarded < size) {
        auto to_read = std::min<uint64_t>(size - bytes_discarded, sizeof(buf));

        OUTCOME_TRY(n, file_read_retry(file, buf,
                                       static_cast<size_t>(to_read)));
        bytes_discarded += n;

        if (n < to_read) {
            break;
        }
    }

    return bytes_discarded;
}

/*!
 * \class FileSearcher
 *
 * \brief Search file for binary sequence
 */

/*!
 * \brief Construct with search pattern
 *
 * \param file File to search
 * \param pattern Pointer to search for
 * \param pattern_size Size of pattern
 */
FileSearcher::FileSearcher(File *file, const void *pattern, size_t pattern_size)
    : m_file(file)
    , m_pattern(pattern)
    , m_pattern_size(pattern_size)
    , m_searcher({static_cast<const unsigned char *>(pattern),
                  static_cast<const unsigned char *>(pattern) + pattern_size})
    , m_region_begin(0)
    , m_region_end(0)
    , m_offset(0)
{
    auto buf_size = DEFAULT_BUFFER_SIZE;

    if (pattern_size > SIZE_MAX / 2) {
        buf_size = SIZE_MAX;
    } else {
        buf_size = std::max(buf_size, pattern_size * 2);
    }

    m_buf.resize(buf_size);
}

/*!
 * \brief Move constructor
 *
 * \p other will be placed in an unusable state. It can be reused by assigning
 * it to a new FileSearcher instance.
 *
 * \param other File searcher to move from
 */
FileSearcher::FileSearcher(FileSearcher &&other) noexcept
{
    clear();

    std::swap(m_file, other.m_file);
    std::swap(m_pattern, other.m_pattern);
    std::swap(m_pattern_size, other.m_pattern_size);
    std::swap(m_searcher, other.m_searcher);
    std::swap(m_buf, other.m_buf);
    std::swap(m_region_begin, other.m_region_begin);
    std::swap(m_region_end, other.m_region_end);
    std::swap(m_offset, other.m_offset);
}

/*!
 * \brief Move assignment operator
 *
 * \p rhs will be placed in an unusable state. It can be reused by assigning it
 * to a new FileSearcher instance.
 *
 * \param rhs File searcher to move from
 */
FileSearcher & FileSearcher::operator=(FileSearcher &&rhs) noexcept
{
    if (this != &rhs) {
        clear();

        std::swap(m_file, rhs.m_file);
        std::swap(m_pattern, rhs.m_pattern);
        std::swap(m_pattern_size, rhs.m_pattern_size);
        std::swap(m_searcher, rhs.m_searcher);
        std::swap(m_buf, rhs.m_buf);
        std::swap(m_region_begin, rhs.m_region_begin);
        std::swap(m_region_end, rhs.m_region_end);
        std::swap(m_offset, rhs.m_offset);
    }

    return *this;
}

/*!
 * \brief Find next match in the file
 *
 * The offset returned is relative to the file position when the FileSearcher
 * was constructed.
 *
 * If the file happens to be seekable, the caller may interact with the file.
 * However, the file position *must* be restored to the original position before
 * the next call to this function. Note that the file position is not guaranteed
 * (and even unlikely) to equal the match offset due to in-memory buffering.
  *
 * \note We do not do overlapping searches. For example, if a file's contents
 *       is `ababababab` and the search pattern is `abab`, the resulting offsets
 *       will be (0 and 4), *not* (0, 2, 4, 6). In other words, the next search
 *       begins at the end of the curent match.
 *
 * \note The file position after this function returns is unspecified. Be sure
 *       to seek to a known location before attempting further read or write
 *       operations.
 *
 * \return
 *   * The match offset if the pattern is found
 *   * std::nullopt if there are no more matches
 *   * Otherwise, an appropriate error code
 */
oc::result<std::optional<uint64_t>> FileSearcher::next()
{
    if (!m_file) {
        return FileError::InvalidState;
    }

    if (m_pattern_size == 0) {
        return std::nullopt;
    }

    while (true) {
        // Find pattern in current buffer
        if (auto it = std2::search(m_buf.data() + m_region_begin,
                                   m_buf.data() + m_region_end, *m_searcher);
                it != m_buf.data() + m_region_end) {
            auto match_index = static_cast<size_t>(it - m_buf.data());
            auto match_offset = m_offset + match_index;

            // We don't do overlapping searches
            m_region_begin += match_index + m_pattern_size;

            return match_offset;
        }

        // If the pattern is not found, up to pattern_size - 1 bytes may still
        // match, so move those to the beginning. We will move fewer than
        // pattern_size - 1 bytes if there was a match close to the end.
        auto to_move = std::min(m_region_end - m_region_begin,
                                m_pattern_size - 1);
        m_offset += m_region_end - to_move;
        memmove(m_buf.data(), m_buf.data() + m_region_end - to_move, to_move);
        m_region_begin = 0;
        m_region_end = to_move;

        // Fill up buffer. Dereferencing m_buf_end is always okay because it is
        // guaranteed to be m_pattern_size - 1 bytes or less into the buffer.
        OUTCOME_TRY(n, file_read_retry(*m_file, m_buf.data() + m_region_end,
                                       m_buf.size() - m_region_end));

        m_region_end += n;

        if (m_region_end < m_pattern_size) {
            // Reached EOF
            return std::nullopt;
        }

        // Ensure match offset cannot overflow
        if (m_offset > UINT64_MAX - (m_region_end - m_pattern_size)) {
            return std::errc::result_out_of_range;
        }
    }
}

void FileSearcher::clear() noexcept
{
    m_file = nullptr;
    m_pattern = nullptr;
    m_pattern_size = 0;
    m_searcher = std::nullopt;
    m_buf.clear();
    m_region_begin = 0;
    m_region_end = 0;
    m_offset = 0;
}

/*!
 * \brief Move data in file
 *
 * This function is equivalent to `memmove()`, except it operates on a File
 * handle. The source and destination regions can overlap. In the degenerate
 * case where \p src == \p dest or \p size == 0, no operation will be performed,
 * but the function will return \p size accordingly.
 *
 * \note This function is very seek-heavy and may be slow if the handle cannot
 *       seek efficiently. It will perform two seeks per loop interation. Each
 *       iteration moves up to 10240 bytes.
 *
 * \note If the return value, \p r, is less than \p size, then the *first* \p r
 *       bytes have been copied from offset \p src to offset \p dest. This is
 *       true even if \p src \< \p dest, resulting in a backwards copy.
 *
 * \param file File handle
 * \param src Source offset
 * \param dest Destination offset
 * \param size Size of data to move
 *
 * \return Size of data that is moved if data is successfully moved. Otherwise,
 *         the error code.
 */
oc::result<uint64_t> file_move(File &file, uint64_t src, uint64_t dest,
                               uint64_t size)
{
    // Check if we need to do anything
    if (src == dest || size == 0) {
        return size;
    }

    if (src > UINT64_MAX - size || dest > UINT64_MAX - size) {
        // Offset + size overflows integer
        return FileError::ArgumentOutOfRange;
    }

    char buf[10240];
    uint64_t size_moved = 0;
    const bool copy_forwards = dest < src;

    while (size_moved < size) {
        auto to_read = std::min<uint64_t>(sizeof(buf), size - size_moved);

        // Seek to source offset
        OUTCOME_TRYV(file.seek(static_cast<int64_t>(
                copy_forwards ? src + size_moved
                        : src + size - size_moved - to_read), SEEK_SET));

        // Read data from source
        OUTCOME_TRY(n_read, file_read_retry(
                file, buf, static_cast<size_t>(to_read)));
        if (n_read == 0) {
            break;
        }

        // Seek to destination offset
        OUTCOME_TRYV(file.seek(static_cast<int64_t>(
                copy_forwards ? dest + size_moved
                        : dest + size - size_moved - n_read), SEEK_SET));

        // Write data to destination
        OUTCOME_TRY(n_written, file_write_retry(file, buf, n_read));

        size_moved += n_written;

        if (n_written < n_read) {
            if (copy_forwards) {
                break;
            } else {
                // Hit EOF. Subtract bytes beyond EOF that we can't copy
                size -= n_read - n_written;
            }
        }
    }

    return size_moved;
}

}
