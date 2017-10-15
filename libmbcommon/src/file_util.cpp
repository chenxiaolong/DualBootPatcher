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

#include "mbcommon/error/error.h"
#include "mbcommon/error/error_handler.h"
#include "mbcommon/error/type/ec_error.h"
#include "mbcommon/file_error.h"
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
 * fails and the error is an ECError with code std::errc::interrupted, then the
 * read operation will be automatically reattempted.
 *
 * \param[in] file File handle
 * \param[out] buf Buffer to read into
 * \param[in] size Buffer size
 *
 * \return Number of bytes read if some are successfully read or EOF is reached.
 *         Otherwise, the error.
 */
Expected<size_t> file_read_fully(File &file, void *buf, size_t size)
{
    size_t bytes_read = 0;

    while (bytes_read < size) {
        auto n = file.read(static_cast<char *>(buf) + bytes_read,
                           size - bytes_read);
        if (!n) {
            auto error = handle_errors(
                n.take_error(),
                [](std::unique_ptr<ECError> err) -> Error {
                    if (err->error_code() == std::errc::interrupted) {
                        return Error::success();
                    } else {
                        return {std::move(err)};
                    }
                }
            );
            if (error) {
                return std::move(error);
            } else {
                continue;
            }
        } else if (*n == 0) {
            break;
        }

        bytes_read += *n;
    }

    return bytes_read;
}

/*!
 * \brief Write to a File handle.
 *
 * This function differs from File::write() in that it will call File::write()
 * repeatedly until the buffer is filled or EOF is reached. If File::write()
 * fails and the error is an ECError with code std::errc::interrupted, then the
 * write operation will be automatically reattempted.
 *
 * \param file File handle
 * \param buf Buffer to write from
 * \param size Buffer size
 *
 * \return Number of bytes written if some are successfully written. Otherwise,
 *         the error.
 */
Expected<size_t> file_write_fully(File &file, const void *buf, size_t size)
{
    size_t bytes_written = 0;

    while (bytes_written < size) {
        auto n = file.write(static_cast<const char *>(buf) + bytes_written,
                            size - bytes_written);
        if (!n) {
            auto error = handle_errors(
                n.take_error(),
                [](std::unique_ptr<ECError> err) -> Error {
                    if (err->error_code() == std::errc::interrupted) {
                        return Error::success();
                    } else {
                        return {std::move(err)};
                    }
                }
            );
            if (error) {
                return std::move(error);
            } else {
                continue;
            }
        } else if (*n == 0) {
            break;
        }

        bytes_written += *n;
    }

    return bytes_written;
}

/*!
 * \brief Read from a File handle and discard the data.
 *
 * This function will repeatedly call File::read() and discard any data that
 * is read. If File::read() fails and the error is an ECError with code
 * std::errc::interrupted, then read operation will be automatically
 * reattempted.
 *
 * \param file File handle
 * \param size Number of bytes to discard
 *
 * \return Number of bytes discarded if some are successfully read and discarded
 *         or EOF is reached. Otherwise, the error.
 */
Expected<uint64_t> file_read_discard(File &file, uint64_t size)
{
    char buf[10240];

    uint64_t bytes_discarded = 0;

    while (bytes_discarded < size) {
        auto to_read = std::min<uint64_t>(size, sizeof(buf));
        auto n = file.read(buf, static_cast<size_t>(to_read));
        if (!n) {
            auto error = handle_errors(
                n.take_error(),
                [](std::unique_ptr<ECError> err) -> Error {
                    if (err->error_code() == std::errc::interrupted) {
                        return Error::success();
                    } else {
                        return {std::move(err)};
                    }
                }
            );
            if (error) {
                return std::move(error);
            } else {
                continue;
            }
        } else if (*n == 0) {
            break;
        }

        bytes_discarded += *n;
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
 * \param userdata User callback data
 * \param offset File offset of search result
 *
 * \return
 *   * #FileSearchAction::Continue to continue search
 *   * #FileSearchAction::Stop to stop search, but have file_search() report a
 *     successful result
 *   * An error if file_search() should report a failure
 */

/*!
 * \brief Search file for binary sequence
 *
 * If \p buf_size is non-zero, a buffer of size \p buf_size will be allocated.
 * If it is less than or equal to \p pattern_size, then the function will return
 * a FileError with type FileErrorType::ArgumentOutOfRange. If \p buf_size is
 * zero, then the larger of 8 MiB and 2 * \p pattern_size will be used. In the
 * rare case that 2 * \p pattern_size would exceed the maximum value of a
 * `size_t`, `SIZE_MAX` will be used.
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
 * \return Nothing if the search completes successfully. Otherwise, the error.
 */
Expected<void> file_search(File &file, int64_t start, int64_t end,
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
        return make_error<FileError>(FileErrorType::ArgumentOutOfRange,
                                     "End offset < start offset");
    }

    // Trivial case
    if (max_matches == 0 || pattern_size == 0) {
        return {};
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
        return make_error<FileError>(
                FileErrorType::ArgumentOutOfRange,
                "Buffer size cannot be less than pattern size");
    }

    buf.reset(static_cast<char *>(malloc(buf_size)));
    if (!buf) {
        return make_error<ECError>(ECError::from_errno(errno));
    }

    if (start >= 0) {
        offset = static_cast<uint64_t>(start);
    } else {
        offset = 0;
    }

    // Seek to starting point
    auto seek_ret = file.seek(static_cast<int64_t>(offset), SEEK_SET);
    if (!seek_ret) {
        // If seeking is not supported, read and discard data
        auto error = handle_errors(
            seek_ret.take_error(),
            [&](std::unique_ptr<FileError> err) -> Error {
                if (err->type() == FileErrorType::UnsupportedSeek) {
                    auto discarded = file_read_discard(file, offset);
                    if (!discarded) {
                        return discarded.take_error();
                    } else if (*discarded != offset) {
                        file.set_fatal(true);
                        return make_error<FileError>(
                                FileErrorType::ArgumentOutOfRange,
                                "Reached EOF before starting offset");
                    }
                    return Error::success();
                }
                return {std::move(err)};
            }
        );
        if (error) {
            return std::move(error);
        }
    }

    // Initially read to beginning of buffer
    ptr = buf.get();
    ptr_remain = buf_size;

    while (true) {
        auto n_read = file_read_fully(file, ptr, ptr_remain);
        if (!n_read) {
            return n_read.take_error();
        }
        n = *n_read;

        // Number of available bytes in buf
        n += static_cast<size_t>(ptr - buf.get());

        if (n < pattern_size) {
            // Reached EOF
            return {};
        } else if (end >= 0 && offset >= static_cast<uint64_t>(end)) {
            // Artificial EOF
            return {};
        }

        // Ensure that offset + n (and consequently, offset + diff) cannot
        // overflow
        if (n > UINT64_MAX - offset) {
            return make_error<FileError>(FileErrorType::IntegerOverflow,
                                         "Read overflows offset value");
        }

        // Search from beginning of buffer
        match = buf.get();
        match_remain = n;

        while ((match = static_cast<char *>(
                mb_memmem(match, match_remain, pattern, pattern_size)))) {
            // Stop if match falls outside of ending boundary
            if (end >= 0 && offset + static_cast<size_t>(match - buf.get())
                    + pattern_size > static_cast<uint64_t>(end)) {
                return {};
            }

            // Invoke callback
            auto ret = result_cb(file, userdata,
                                 offset + static_cast<size_t>(match - buf.get()));
            if (!ret) {
                return ret.take_error();
            } else if (*ret == FileSearchAction::Stop) {
                // Stop searching early
                return {};
            }

            if (max_matches > 0) {
                --max_matches;
                if (max_matches == 0) {
                    return {};
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
 * \param file File handle
 * \param src Source offset
 * \param dest Destination offset
 * \param size Size of data to move
 *
 * \return Size of data that is moved if data is successfully moved. Otherwise,
 *         the error.
 */
Expected<uint64_t> file_move(File &file, uint64_t src, uint64_t dest,
                             uint64_t size)
{
    char buf[10240];
    size_t n_read;
    size_t n_written;

    // Check if we need to do anything
    if (src == dest || size == 0) {
        return size;
    }

    if (src > UINT64_MAX - size || dest > UINT64_MAX - size) {
        return make_error<FileError>(FileErrorType::ArgumentOutOfRange,
                                     "Offset + size overflows integer");
    }

    uint64_t size_moved = 0;

    if (dest < src) {
        // Copy forwards
        while (size_moved < size) {
            auto to_read = std::min<uint64_t>(
                    sizeof(buf), size - size_moved);

            // Seek to source offset
            auto seek_ret = file.seek(static_cast<int64_t>(src + size_moved),
                                      SEEK_SET);
            if (!seek_ret) {
                return seek_ret.take_error();
            }

            // Read data from source
            auto ret = file_read_fully(file, buf, static_cast<size_t>(to_read));
            if (!ret) {
                return ret.take_error();
            } else if (*ret == 0) {
                break;
            }
            n_read = *ret;

            // Seek to destination offset
            seek_ret = file.seek(static_cast<int64_t>(dest + size_moved),
                                 SEEK_SET);
            if (!seek_ret) {
                return seek_ret.take_error();
            }

            // Write data to destination
            ret = file_write_fully(file, buf, n_read);
            if (!ret) {
                return ret.take_error();
            }
            n_written = *ret;

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
            auto seek_ret = file.seek(static_cast<int64_t>(
                    src + size - size_moved - to_read), SEEK_SET);
            if (!seek_ret) {
                return seek_ret.take_error();
            }

            // Read data form source
            auto ret = file_read_fully(file, buf, static_cast<size_t>(to_read));
            if (!ret) {
                return ret.take_error();
            } else if (*ret == 0) {
                break;
            }
            n_read = *ret;

            // Seek to destination offset
            seek_ret = file.seek(static_cast<int64_t>(
                    dest + size - size_moved - n_read), SEEK_SET);
            if (!seek_ret) {
                return seek_ret.take_error();
            }

            // Write data to destination
            ret = file_write_fully(file, buf, n_read);
            if (!ret) {
                return ret.take_error();
            }
            n_written = *ret;

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
