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

#define DEFAULT_BUFFER_SIZE             (8 * 1024 * 1024)

/*!
 * \file mbcommon/file_util.h
 * \brief Useful utility functions for File API
 */

namespace mb
{

#ifdef __ANDROID__
namespace std2 = std::experimental;
#else
namespace std2 = std;
#endif

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
 * \typedef FileSearchResultCallback
 *
 * \brief Search result callback for file_search()
 *
 * \note The file position must not change after a successful return of this
 *       callback. If file operations need to be performed, save the file
 *       position beforehand with File::seek() and restore it afterwards. Note
 *       that the file position is unlikely to match \p offset.
 *
 * \sa file_search()
 *
 * \param file File handle
 * \param offset File offset of search result
 *
 * \return
 *   * #FileSearchAction::Continue to continue search
 *   * #FileSearchAction::Stop to stop search, but have file_search() report a
 *     successful result
 *   * An error code if file_search() should report a failure
 */

/*!
 * \brief Search file for binary sequence
 *
 * If \p buf_size is non-zero, a buffer of size \p buf_size will be allocated.
 * If it is less than or equal to \p pattern_size, then the function will return
 * FileError::ArgumentOutOfRange. If \p buf_size is zero, then the larger of
 * 8 MiB and 2 * \p pattern_size will be used. In the rare case that
 * 2 * \p pattern_size would exceed the maximum value of a `size_t`, `SIZE_MAX`
 * will be used.
 *
 * If \p file does not support seeking, then the file position must be set to
 * the beginning of the file before calling this function. Instead of seeking,
 * the function will read and discard any data before \p start.
 *
 * \note We do not do overlapping searches. For example, if a file's contents
 *       is "ababababab" and the search pattern is "abab", the resulting offsets
 *       will be (0 and 4), *not* (0, 2, 4, 6). In other words, the next search
 *       begins at the end of the curent search.
 *
 * \note The file position after this function returns is undefined. Be sure to
 *       seek to a known location before attempting further read or write
 *       operations.
 *
 * \param file File handle
 * \param start Start offset or nothing for beginning of file
 * \param end End offset or nothing for end of file
 * \param bsize Buffer size or 0 to automatically choose a size
 * \param pattern Pattern to search
 * \param pattern_size Size of pattern
 * \param max_matches Maximum number of matches or nothing to find all matches
 * \param result_cb Callback to invoke upon finding a match
 *
 * \return Nothing if the search completes successfully. Otherwise, the error
 *         code.
 */
oc::result<void> file_search(File &file,
                             std::optional<uint64_t> start,
                             std::optional<uint64_t> end,
                             size_t bsize, const void *pattern,
                             size_t pattern_size,
                             std::optional<uint64_t> max_matches,
                             const FileSearchResultCallback &result_cb)
{
    size_t buf_size;
    uint64_t offset;

    // Check boundaries
    if (start && end && *end < *start) {
        // End offset < start offset
        return FileError::ArgumentOutOfRange;
    }

    // Trivial case
    if ((max_matches && *max_matches == 0) || pattern_size == 0) {
        return oc::success();
    }

    // Compute buffer size
    if (bsize != 0) {
        buf_size = bsize;
    } else {
        buf_size = DEFAULT_BUFFER_SIZE;

        if (pattern_size > SIZE_MAX / 2) {
            buf_size = SIZE_MAX;
        } else {
            buf_size = std::max(buf_size, pattern_size * 2);
        }
    }

    // Ensure buffer is large enough
    if (buf_size < pattern_size) {
        // Buffer size cannot be less than pattern size
        return FileError::ArgumentOutOfRange;
    }

    std::vector<unsigned char> buf(buf_size);

    if (start) {
        offset = *start;
    } else {
        offset = 0;
    }

    // Seek to starting point
    auto seek_ret = file.seek(static_cast<int64_t>(offset), SEEK_SET);
    if (!seek_ret) {
        if (seek_ret.error() == FileErrorC::Unsupported) {
            OUTCOME_TRY(discarded, file_read_discard(file, offset));

            if (discarded != offset) {
                // Reached EOF before starting offset
                return FileError::ArgumentOutOfRange;
            }
        } else {
            return seek_ret.as_failure();
        }
    }

    // Initially read to beginning of buffer
    unsigned char *ptr = buf.data();
    size_t ptr_remain = buf.size();

    // Boyer-Moore searcher for pattern
    auto pattern_searcher = std2::boyer_moore_searcher<const unsigned char *>(
            static_cast<const unsigned char *>(pattern),
            static_cast<const unsigned char *>(pattern) + pattern_size);

    while (true) {
        OUTCOME_TRY(n, file_read_retry(file, ptr, ptr_remain));

        // Number of available bytes in buf
        n += static_cast<size_t>(ptr - buf.data());

        if (n < pattern_size) {
            // Reached EOF
            return oc::success();
        } else if (end && offset >= *end) {
            // Artificial EOF
            return oc::success();
        }

        // Ensure that offset + n (and consequently, offset + diff) cannot
        // overflow
        if (n > UINT64_MAX - offset) {
            // Read overflows offset value
            return FileError::IntegerOverflow;
        }

        // Search from beginning of buffer
        unsigned char *match = buf.data();
        size_t match_remain = n;

        while (true) {
            auto it = std2::search(match, match + match_remain,
                                   pattern_searcher);
            if (it == match + match_remain) {
                break;
            }
            match = it;

            // Stop if match falls outside of ending boundary
            if (end && offset + static_cast<size_t>(match - buf.data())
                    + pattern_size > *end) {
                return oc::success();
            }

            // Invoke callback
            auto ret = result_cb(
                    file, offset + static_cast<size_t>(match - buf.data()));
            if (!ret) {
                return ret.as_failure();
            } else if (ret.value() == FileSearchAction::Stop) {
                // Stop searching early
                return oc::success();
            }

            if (max_matches && *max_matches > 0) {
                --*max_matches;
                if (*max_matches == 0) {
                    return oc::success();
                }
            }

            // We don't do overlapping searches
            if (match_remain >= pattern_size) {
                match += pattern_size;
                match_remain = n - static_cast<size_t>(match - buf.data());
            } else {
                break;
            }
        }

        // Up to pattern_size - 1 bytes may still match, so move those to the
        // beginning. We will move fewer than pattern_size - 1 bytes if there
        // was a match close to the end.
        size_t to_move = std::min(match_remain, pattern_size - 1);
        memmove(buf.data(), buf.data() + n - to_move, to_move);
        ptr = buf.data() + to_move;
        ptr_remain = buf.size() - to_move;
        offset += n - to_move;
    }
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
    char buf[10240];

    // Check if we need to do anything
    if (src == dest || size == 0) {
        return size;
    }

    if (src > UINT64_MAX - size || dest > UINT64_MAX - size) {
        // Offset + size overflows integer
        return FileError::ArgumentOutOfRange;
    }

    uint64_t size_moved = 0;

    if (dest < src) {
        // Copy forwards
        while (size_moved < size) {
            auto to_read = std::min<uint64_t>(
                    sizeof(buf), size - size_moved);

            // Seek to source offset
            OUTCOME_TRYV(file.seek(static_cast<int64_t>(src + size_moved),
                                   SEEK_SET));

            // Read data from source
            OUTCOME_TRY(n_read, file_read_retry(
                    file, buf, static_cast<size_t>(to_read)));
            if (n_read == 0) {
                break;
            }

            // Seek to destination offset
            OUTCOME_TRYV(file.seek(static_cast<int64_t>(dest + size_moved),
                                   SEEK_SET));

            // Write data to destination
            OUTCOME_TRY(n_written, file_write_retry(file, buf, n_read));

            size_moved += n_written;

            if (n_written < n_read) {
                break;
            }
        }
    } else {
        // Copy backwards
        while (size_moved < size) {
            auto to_read = std::min<uint64_t>(sizeof(buf), size - size_moved);

            // Seek to source offset
            OUTCOME_TRYV(file.seek(static_cast<int64_t>(
                    src + size - size_moved - to_read), SEEK_SET));

            // Read data form source
            OUTCOME_TRY(n_read, file_read_retry(
                    file, buf, static_cast<size_t>(to_read)));
            if (n_read == 0) {
                break;
            }

            // Seek to destination offset
            OUTCOME_TRYV(file.seek(static_cast<int64_t>(
                    dest + size - size_moved - n_read), SEEK_SET));

            // Write data to destination
            OUTCOME_TRY(n_written, file_write_retry(file, buf, n_read));

            size_moved += n_written;

            if (n_written < n_read) {
                // Hit EOF. Subtract bytes beyond EOF that we can't copy
                size -= n_read - n_written;
            }
        }
    }

    return size_moved;
}

}
