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

#include "mbcommon/file_util.h"

#include <algorithm>

#include <cerrno>
#include <cstdio>
#include <cstring>

#include "mbcommon/libc/string.h"

#define DEFAULT_BUFFER_SIZE             (8 * 1024 * 1024)

/*!
 * \file mbcommon/file_util.h
 * \brief Useful utility functions for File API
 */

namespace mb
{

/*!
 * \brief Read from a File handle.
 *
 * This function differs from File::read() in that it will call File::read()
 * repeatedly until the buffer is filled or EOF is reached. If File::read()
 * fails and the error is std::errc::interrupted, the read operation will be
 * automatically reattempted.
 *
 * \note \p bytes_read is updated with the number of bytes successfully read
 *       even when this function fails. Take this into account if reattempting
 *       the read operation.
 *
 * \param[in] file File handle
 * \param[out] buf Buffer to read into
 * \param[in] size Buffer size
 * \param[out] bytes_read Output number of bytes that were read. A short read
 *                        indicates end of file.
 *
 * \return Whether some bytes are read or EOF is reached
 */
bool file_read_fully(File &file, void *buf, size_t size, size_t &bytes_read)
{
    size_t n;

    bytes_read = 0;

    while (bytes_read < size) {
        if (!file.read(static_cast<char *>(buf) + bytes_read,
                       size - bytes_read, n)) {
            if (file.error() == std::errc::interrupted) {
                continue;
            } else {
                return false;
            }
        } else if (n == 0) {
            break;
        }

        bytes_read += n;
    }

    return true;
}

/*!
 * \brief Write to a File handle.
 *
 * This function differs from File::write() in that it will call
 * File::write() repeatedly until the buffer is filled or EOF is reached. If
 * File::write() fails and the error is std::errc::interrupted, the write
 * operation will be automatically reattempted.
 *
 * \note \p bytes_written is updated with the number of bytes successfully
 *       written even when this function fails. Take this into account if
 *       reattempting the write operation.
 *
 * \param[in] file File handle
 * \param[in] buf Buffer to write from
 * \param[in] size Buffer size
 * \param[out] bytes_written Output number of bytes that were written.
 *
 * \return Whether some bytes are written
 */
bool file_write_fully(File &file, const void *buf, size_t size,
                      size_t &bytes_written)
{
    size_t n;

    bytes_written = 0;

    while (bytes_written < size) {
        if (!file.write(static_cast<const char *>(buf) + bytes_written,
                        size - bytes_written, n)) {
            if (file.error() == std::errc::interrupted) {
                continue;
            } else {
                return false;
            }
        } else if (n == 0) {
            break;
        }

        bytes_written += n;
    }

    return true;
}

/*!
 * \brief Read from a File handle and discard the data.
 *
 * This function will repeatedly call File::read() and discard any data that
 * is read. If File::read() fails and the error is std::errc::interrupted, the
 * read operation will be automatically reattempted.
 *
 * \note \p bytes_discarded is updated with the number of bytes successfully
 *       read even when this function fails. Take this into account if
 *       reattempting the read operation.
 *
 * \param[in] file File handle
 * \param[in] size Number of bytes to discard
 * \param[out] bytes_discarded Output number of bytes that were read. A short
 *                             read indicates end of file.
 *
 * \return Whether some bytes are discarded or EOF is reached
 */
bool file_read_discard(File &file, uint64_t size, uint64_t &bytes_discarded)
{
    char buf[10240];
    size_t n;

    bytes_discarded = 0;

    while (bytes_discarded < size) {
        if (!file.read(buf, std::min<uint64_t>(size, sizeof(buf)), n)) {
            if (file.error() == std::errc::interrupted) {
                continue;
            } else {
                return false;
            }
        } else if (n == 0) {
            break;
        }

        bytes_discarded += n;
    }

    return true;
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
 * \param userdata User callback data
 * \param offset File offset of search result
 *
 * \return
 *   * #FileSearchAction::Continue to continue search
 *   * #FileSearchAction::Stop to stop search, but have file_search() report a
 *     successful result
 *   * #FileSearchAction::Fail if an error occurs
 */

/*!
 * \brief Search file for binary sequence
 *
 * If \p buf_size is non-zero, a buffer of size \p buf_size will be allocated.
 * If it is less than or equal to \p pattern_size, then the function will fail
 * and set `errno` to `EINVAL`. If \p buf_size is zero, then the larger of 8 MiB
 * and 2 * \p pattern_size will be used. In the rare case that
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
 * \param start Start offset or negative number for beginning of file
 * \param end End offset or negative number for end of file
 * \param bsize Buffer size or 0 to automatically choose a size
 * \param pattern Pattern to search
 * \param pattern_size Size of pattern
 * \param max_matches Maximum number of matches or -1 to find all matches
 * \param result_cb Callback to invoke upon finding a match
 * \param userdata User callback data
 *
 * \return Whether the search completes successfully
 */
bool file_search(File &file, int64_t start, int64_t end,
                 size_t bsize, const void *pattern,
                 size_t pattern_size, int64_t max_matches,
                 FileSearchResultCallback result_cb,
                 void *userdata)
{
    std::unique_ptr<char, decltype(free) *> buf(nullptr, &free);
    size_t buf_size;
    char *ptr;
    size_t ptr_remain;
    char *match;
    size_t match_remain;
    uint64_t offset;
    size_t n;

    // Check boundaries
    if (start >= 0 && end >= 0 && end < start) {
        file.set_error(make_error_code(FileError::ArgumentOutOfRange),
                       "End offset < start offset");
        return false;
    }

    // Trivial case
    if (max_matches == 0 || pattern_size == 0) {
        return true;
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
        file.set_error(make_error_code(FileError::ArgumentOutOfRange),
                       "Buffer size cannot be less than pattern size");
        return false;
    }

    buf.reset(static_cast<char *>(malloc(buf_size)));
    if (!buf) {
        file.set_error(std::error_code(errno, std::generic_category()),
                       "Failed to allocate buffer");
        return false;
    }

    if (start >= 0) {
        offset = static_cast<uint64_t>(start);
    } else {
        offset = 0;
    }

    // Seek to starting point
    if (!file.seek(static_cast<int64_t>(offset), SEEK_SET, nullptr)) {
        if (file.error() == FileErrorC::Unsupported) {
            uint64_t discarded;
            if (!file_read_discard(file, offset, discarded)) {
                return false;
            } else if (discarded != offset) {
                file.set_error(make_error_code(FileError::ArgumentOutOfRange),
                               "Reached EOF before starting offset");
                file.set_fatal(true);
                return false;
            }
        } else {
            return false;
        }
    }

    // Initially read to beginning of buffer
    ptr = buf.get();
    ptr_remain = buf_size;

    while (true) {
        if (!file_read_fully(file, ptr, ptr_remain, n)) {
            return false;
        }

        // Number of available bytes in buf
        n += static_cast<size_t>(ptr - buf.get());

        if (n < pattern_size) {
            // Reached EOF
            return true;
        } else if (end >= 0 && offset >= static_cast<uint64_t>(end)) {
            // Artificial EOF
            return true;
        }

        // Ensure that offset + n (and consequently, offset + diff) cannot
        // overflow
        if (n > UINT64_MAX - offset) {
            file.set_error(make_error_code(FileError::IntegerOverflow),
                           "Read overflows offset value");
            return false;
        }

        // Search from beginning of buffer
        match = buf.get();
        match_remain = n;

        while ((match = static_cast<char *>(
                mb_memmem(match, match_remain, pattern, pattern_size)))) {
            // Stop if match falls outside of ending boundary
            if (end >= 0 && offset + static_cast<size_t>(match - buf.get())
                    + pattern_size > static_cast<uint64_t>(end)) {
                return true;
            }

            // Invoke callback
            auto ret = result_cb(file, userdata,
                                 offset + static_cast<size_t>(match - buf.get()));
            if (ret == FileSearchAction::Stop) {
                // Stop searching early
                return true;
            } else if (ret != FileSearchAction::Continue) {
                return false;
            }

            if (max_matches > 0) {
                --max_matches;
                if (max_matches == 0) {
                    return true;
                }
            }

            // We don't do overlapping searches
            if (match_remain >= pattern_size) {
                match += pattern_size;
                match_remain = n - static_cast<size_t>(match - buf.get());
            } else {
                break;
            }
        }

        // Up to pattern_size - 1 bytes may still match, so move those to the
        // beginning. We will move fewer than pattern_size - 1 bytes if there
        // was a match close to the end.
        size_t to_move = std::min(match_remain, pattern_size - 1);
        memmove(buf.get(), buf.get() + n - to_move, to_move);
        ptr = buf.get() + to_move;
        ptr_remain = buf_size - to_move;
        offset += n - to_move;
    }

    return true;
}

/*!
 * \brief Move data in file
 *
 * This function is equivalent to `memmove()`, except it operates on a File
 * handle. The source and destination regions can overlap. In the degenerate
 * case where \p src == \p dest or \p size == 0, no operation will be performed,
 * but the function will return true and set \p size_moved accordingly.
 *
 * \note This function is very seek-heavy and may be slow if the handle cannot
 *       seek efficiently. It will perform two seeks per loop interation. Each
 *       iteration moves up to 10240 bytes.
 *
 * \note If \p *size_moved is less than \p size, then the *first* \p *size_moved
 *       bytes have been copied from offset \p src to offset \p dest. This is
 *       true even if \p src \< \p dest, resulting in a backwards copy.
 *
 * \param[in] file File handle
 * \param[in] src Source offset
 * \param[in] dest Destination offset
 * \param[in] size Size of data to move
 * \param[out] size_moved Pointer to store size of data that is moved
 *
 * \return Whether data is successfully moved
 */
bool file_move(File &file, uint64_t src, uint64_t dest, uint64_t size,
               uint64_t &size_moved)
{
    char buf[10240];
    size_t n_read;
    size_t n_written;

    // Check if we need to do anything
    if (src == dest || size == 0) {
        size_moved = size;
        return true;
    }

    if (src > UINT64_MAX - size || dest > UINT64_MAX - size) {
        file.set_error(make_error_code(FileError::ArgumentOutOfRange),
                       "Offset + size overflows integer");
        return false;
    }

    size_moved = 0;

    if (dest < src) {
        // Copy forwards
        while (size_moved < size) {
            size_t to_read = std::min<uint64_t>(
                    sizeof(buf), size - size_moved);

            // Seek to source offset
            if (!file.seek(static_cast<int64_t>(src + size_moved),
                           SEEK_SET, nullptr)) {
                return false;
            }

            // Read data from source
            if (!file_read_fully(file, buf, to_read, n_read)) {
                return false;
            } else if (n_read == 0) {
                break;
            }

            // Seek to destination offset
            if (!file.seek(static_cast<int64_t>(dest + size_moved),
                           SEEK_SET, nullptr)) {
                return false;
            }

            // Write data to destination
            if (!file_write_fully(file, buf, n_read, n_written)) {
                return false;
            }

            size_moved += n_written;

            if (n_written < n_read) {
                break;
            }
        }
    } else {
        // Copy backwards
        while (size_moved < size) {
            size_t to_read = std::min<uint64_t>(
                    sizeof(buf), size - size_moved);

            // Seek to source offset
            if (!file.seek(static_cast<int64_t>(src + size - size_moved - to_read),
                           SEEK_SET, nullptr)) {
                return false;
            }

            // Read data form source
            if (!file_read_fully(file, buf, to_read, n_read)) {
                return false;
            } else if (n_read == 0) {
                break;
            }

            // Seek to destination offset
            if (!file.seek(static_cast<int64_t>(dest + size - size_moved - n_read),
                           SEEK_SET, nullptr)) {
                return false;
            }

            // Write data to destination
            if (!file_write_fully(file, buf, n_read, n_written)) {
                return false;
            }

            size_moved += n_written;

            if (n_written < n_read) {
                // Hit EOF. Subtract bytes beyond EOF that we can't copy
                size -= n_read - n_written;
            }
        }
    }

    return true;
}

}
