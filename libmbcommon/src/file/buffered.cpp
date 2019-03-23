/*
 * Copyright (C) 2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file/buffered.h"

#include <cstring>

#include "mbcommon/file_error.h"
#include "mbcommon/file_util.h"
#include "mbcommon/finally.h"

/*!
 * \file mbcommon/file/buffered.h
 * \brief Buffered wrapper around a File
 */

namespace mb
{

/*!
 * \class BufferedFile
 *
 * \brief Buffered wrapper around a File handle
 */

/*!
 * \brief Construct unbound BufferedFile
 *
 * The File handle will not be bound to any file. One of the open functions will
 * need to be called to open a file.
 *
 * \param buf_size Size of read and write buffers
 */
BufferedFile::BufferedFile(size_t buf_size)
    : File(), m_file(nullptr), m_fpos(0), m_rbuf(buf_size), m_rpos(0), m_rcap(0)
{
    m_wbuf.reserve(buf_size);
}

/*!
 * \brief Construct BufferedFile with underlying file
 *
 * Construct the file handle and open the file. Use is_open() to check if the
 * file was successfully opened.
 *
 * \sa open(File &)
 *
 * \param buf_size Size of read and write buffers
 *
 * \param file Reference to underlying file
 */
BufferedFile::BufferedFile(File &file, size_t buf_size)
    : BufferedFile(buf_size)
{
    (void) open(file);
}

BufferedFile::~BufferedFile()
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
BufferedFile::BufferedFile(BufferedFile &&other) noexcept
{
    clear();

    std::swap(m_file, other.m_file);
    std::swap(m_fpos, other.m_fpos);
    std::swap(m_rbuf, other.m_rbuf);
    std::swap(m_rpos, other.m_rpos);
    std::swap(m_rcap, other.m_rcap);
    std::swap(m_wbuf, other.m_wbuf);
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
BufferedFile & BufferedFile::operator=(BufferedFile &&rhs) noexcept
{
    if (this != &rhs) {
        (void) close();

        std::swap(m_file, rhs.m_file);
        std::swap(m_fpos, rhs.m_fpos);
        std::swap(m_rbuf, rhs.m_rbuf);
        std::swap(m_rpos, rhs.m_rpos);
        std::swap(m_rcap, rhs.m_rcap);
        std::swap(m_wbuf, rhs.m_wbuf);
    }

    return *this;
}

/*!
 * \brief Open underlying file
 *
 * \param file Reference to underlying file
 *
 * \return Nothing if the file is successfully opened. Otherwise, the error
 *         code.
 */
oc::result<void> BufferedFile::open(File &file)
{
    if (is_open()) return FileError::InvalidState;

    auto reset = finally([&] {
        (void) close();
    });

    m_file = &file;

    OUTCOME_TRYV(sync_pos());

    reset.dismiss();

    return oc::success();
}

/*!
 * \brief Close the buffered file
 *
 * \note This function does *NOT* close the underlying file. It only flushes
 *       pending writes and closes the buffered file so that a new underlying
 *       file can be opened.
 *
 * Regardless of the return value, the handle will be closed. The file handle
 * can then be reused  for opening another file.
 *
 * \return
 *   * Nothing if no error is encountered when flushing pending writes
 *   * Otherwise, the error code of flush()
 */
oc::result<void> BufferedFile::close()
{
    if (!is_open()) return FileError::InvalidState;

    // Reset to allow opening another file
    auto reset = finally([&] {
        clear();
    });

    OUTCOME_TRYV(flush());

    return oc::success();
}

/*!
 * \brief Read data from file
 *
 * \note This function *will* return fewer than \p size bytes in certain
 *       conditions. Make sure the caller does not treat short reads as EOF.
 *
 * \param[out] buf Buffer to read into
 *
 * \return
 *   * Number of bytes read if some bytes are successfully read or EOF is
 *     reached
 *   * FileError::InvalidState if called after write() without flush() in between
 *   * Otherwise, the error code of file_read_retry() from the underlying file
 */
oc::result<size_t> BufferedFile::read(span<unsigned char> buf)
{
    if (!is_open() || !m_wbuf.empty()) return FileError::InvalidState;

    // If there's no data buffered and a large read is being done, then don't
    // bother buffering the data
    if (m_rpos == m_rcap && buf.size() >= m_rbuf.size()) {
        return read_underlying(buf);
    }

    OUTCOME_TRYV(fill_rbuf());

    auto to_copy = std::min(m_rcap - m_rpos, buf.size());
    memcpy(buf.data(), m_rbuf.data() + m_rpos, to_copy);

    consume_rbuf(to_copy);

    return to_copy;
}

/*!
 * \brief Write data to file
 *
 * \note If the amount of data written exceeds the size of the write buffer,
 *       then the data will be written directly to the file
 *
 * \param buf Buffer to write from
 *
 * \return
 *   * Number of bytes written if some bytes are successfully written or EOF is
 *     reached
 *   * FileError::UnexpectedEof if EOF is reached when flushing the write buffer
 *   * Otherwise, the error code of file_write_retry() from the underlying file
 */
oc::result<size_t> BufferedFile::write(span<const unsigned char> buf)
{
    if (!is_open()) return FileError::InvalidState;

    // Need to seek if the logical file position does not match the real file
    // position
    if (m_rpos != m_rcap) {
        auto remainder = static_cast<int64_t>(m_rcap - m_rpos);
        OUTCOME_TRYV(seek_underlying(-remainder, SEEK_CUR));
    }

    // Clear read cache
    m_rpos = m_rcap;

    if (m_wbuf.size() + buf.size() > m_wbuf.capacity()) {
        OUTCOME_TRYV(flush());
    }

    if (buf.size() >= m_wbuf.capacity()) {
        return write_underlying(buf);
    } else {
        auto ptr = reinterpret_cast<const unsigned char *>(buf.data());
        m_wbuf.insert(m_wbuf.end(), ptr, ptr + buf.size());
        return buf.size();
    }
}

/*!
 * \brief Set file position
 *
 * \post The write buffer will be flushed. Also, if \p whence is not `SEEK_CUR`
 *       or if \p offset falls outside of the read buffer, then the read buffer
 *       will be cleared.
 *
 * \param offset File position offset
 * \param whence SEEK_SET, SEEK_CUR, or SEEK_END from `stdio.h`
 *
 * \return
 *   * New file offset if the file position is successfully set
 *   * Otherwise, the error code of File::seek() from the underlying file
 */
oc::result<uint64_t> BufferedFile::seek(int64_t offset, int whence)
{
    if (!is_open()) return FileError::InvalidState;

    OUTCOME_TRYV(flush());

    switch (whence) {
    case SEEK_CUR: {
        // Try to seek within the buffer if possible
        if (offset < 0) {
            if (static_cast<uint64_t>(-offset) <= m_rpos) {
                m_rpos -= static_cast<size_t>(-offset);
                return oc::success(m_fpos - (m_rcap - m_rpos));
            }
        } else {
            if (static_cast<uint64_t>(offset) <= m_rcap - m_rpos) {
                m_rpos += static_cast<size_t>(offset);
                return oc::success(m_fpos - (m_rcap - m_rpos));
            }
        }

        auto remainder = static_cast<int64_t>(m_rcap - m_rpos);

        // if offset - remainder is valid
        // if offset - remainder < INT64_MIN
        if (offset >= INT64_MIN + remainder) {
            return seek_underlying(offset - remainder, SEEK_CUR);
        } else {
            OUTCOME_TRYV(seek_underlying(-remainder, SEEK_CUR));
            return seek_underlying(offset, SEEK_CUR);
        }
    }
    case SEEK_SET:
    case SEEK_END:
        return seek_underlying(offset, whence);
    default:
        MB_UNREACHABLE("Invalid whence argument: %d", whence);
    }
}

/*!
 * \brief Truncate or extend file
 *
 * \note The file position is *not* changed after a successful call of this
 *       function. The size of the file may increase if the file position is
 *       larger than the truncated file size and File::write() is called.
 *
 * \post The read buffer will be cleared and the write buffer will be flushed.
 *
 * \param size New size of file
 *
 * \return
 *   * Nothing if the file size is successfully changed
 *   * Otherwise, the error code of flush() or File::truncate() from the
 *     underlying file
 */
oc::result<void> BufferedFile::truncate(uint64_t size)
{
    if (!is_open()) return FileError::InvalidState;

    OUTCOME_TRYV(flush());

    m_rpos = m_rcap;

    return m_file->truncate(size);
}

bool BufferedFile::is_open()
{
    return m_file;
}

/*!
 * \brief Update file position with that of the underlying file
 *
 * This function is needed to keep the file position of the BufferedFile in sync
 * if something directly called File::seek() on the underlying file.
 *
 * \post The read buffer will be cleared.
 *
 * \return Nothing if successful. Otherwise, the error returned by
 *         calling `File::seek(0, SEEK_CUR)`.
 */
oc::result<void> BufferedFile::sync_pos()
{
    if (!is_open()) return FileError::InvalidState;

    OUTCOME_TRYV(seek_underlying(0, SEEK_CUR));
    return oc::success();
}

/*!
 * \brief Flush write buffer to file
 *
 * \note This function does not do anything if no write() calls have occurred
 *       since the last flush.
 *
 * \return
 *   * Nothing if the write buffer is successfully flushed to the file
 *   * FileError::UnexpectedEof if EOF is reached
 *   * Otherwise, the error code of file_write_exact() from the underlying file
 */
oc::result<void> BufferedFile::flush()
{
    if (!is_open()) return FileError::InvalidState;

    if (!m_wbuf.empty()) {
        OUTCOME_TRYV(write_exact_underlying(m_wbuf));
        m_wbuf.clear();
    }

    return oc::success();
}

/*!
 * \brief Resize read and write buffers
 *
 * \pre The BufferedFile handle must not be open
 *
 * \return Whether the buffers' sizes are successfully set
 */
bool BufferedFile::set_buf_size(size_t size)
{
    if (is_open() || size == 0) return false;

    m_rbuf.reserve(size);
    m_rbuf.resize(size);
    m_rpos = m_rcap = 0;

    m_wbuf.clear();
    m_wbuf.shrink_to_fit();
    m_wbuf.reserve(size);

    return true;
}

/*!
 * \brief Fill the read buffer if it is empty
 *
 * \return Nothing if successful. Otherwise, the error code of
 *         file_read_retry().
 */
oc::result<void> BufferedFile::fill_rbuf()
{
    if (!is_open()) return FileError::InvalidState;

    if (m_rpos == m_rcap) {
        OUTCOME_TRY(n, read_underlying(m_rbuf));
        m_rcap = n;
        m_rpos = 0;
    }

    return oc::success();
}

/*!
 * \brief Mark a number of bytes in the read buffer as being consumed
 *
 * \param n Number of bytes to consume.
 */
void BufferedFile::consume_rbuf(size_t n)
{
    m_rpos = std::min(m_rpos + n, m_rcap);
}

/*!
 * \brief Get read buffer
 *
 * \return Span representing the current read buffer region
 */
span<unsigned char> BufferedFile::rbuf()
{
    return {m_rbuf.data() + m_rpos, m_rcap - m_rpos};
}

/*!
 * \brief Get write buffer
 *
 * \return Span representing the current write buffer region
 */
span<unsigned char> BufferedFile::wbuf()
{
    return m_wbuf;
}

/*!
 * \brief Read line from file until delimiter is found
 *
 * \param f Callback to receive line data
 * \param max_size Maximum size of line
 * \param delim Delimiter
 *
 * \return If successful, the number of bytes read, including the trailing
 *         newline if one exists. If 0 is returned, then EOF is reached.
 *         Otherwise, the error code of fill_rbuf().
 */
oc::result<size_t> BufferedFile::get_delim(const GetDelimFn &f,
                                           std::optional<size_t> max_size,
                                           unsigned char delim)
{
    size_t read = 0;

    while (!max_size || *max_size > 0) {
        OUTCOME_TRYV(fill_rbuf());
        auto read_buf = rbuf();
        bool done = false;

        // If we have a size limit, only search within the boundary
        if (max_size && read_buf.size() > *max_size) {
            read_buf = read_buf.subspan(0, *max_size);
        }

        // Search for delimiter
        if (auto ptr = reinterpret_cast<unsigned char *>(
                memchr(read_buf.data(), delim, read_buf.size()))) {
            read_buf = read_buf.subspan(
                    0, static_cast<size_t>(ptr - read_buf.data() + 1));
            done = true;
        }

        // Call user-provided append function
        f(read_buf);

        // Consume used bytes
        consume_rbuf(read_buf.size());
        read += read_buf.size();
        if (max_size) {
            *max_size -= read_buf.size();
        }

        // Stop if delimiter found or EOF is reached
        if (done || read_buf.empty()) {
            break;
        }
    }

    return read;
}

/*!
 * \fn BufferedFile::read_line(Container &, unsigned char)
 *
 * \brief Read line into dynamically sized container
 *
 * \warning Do NOT use this function with a file containing untrusted data. A
 *          long line can cause a DOS attack where all available memory is used
 *          up.
 *
 * \param[in,out] buf Container to read line into
 * \param[in] delim Delimiter
 *
 * \return If successful, the number of bytes read, including the trailing
 *         newline if one exists. If 0 is returned, then EOF is reached.
 *         Otherwise, the error code of fill_rbuf().
 */

/*!
 * \fn BufferedFile::read_sized_line(Container &, size_t, unsigned char)
 *
 * \brief Read line into dynamically sized container with maximum size
 *
 * Read line into dynamically sized container until \p delim is found or
 * \p max_size is reached, whichever comes first. This is better than calling
 *
 * \code{.cpp}
 * container.resize(max_size);
 * buf_file.read_sized_line(span(container), delim);
 * \endcode
 *
 * because it doesn't require allocating the maximum size of the line being read
 * is small.
 *
 * \param[in,out] buf Container to read line into
 * \param[in] max_size Maximum size of line
 * \param[in] delim Delimiter
 *
 * \return If successful, the number of bytes read, including the trailing
 *         newline if one exists. If 0 is returned, then EOF is reached.
 *         Otherwise, the error code of fill_rbuf().
 */

/*!
 * \brief Read line into fixed size span
 *
 * Read line into \buf until \p delim is found or `buf.size()` is reached,
 * whichever comes first.
 *
 * \param[in,out] buf Buffer to read line into
 * \param[in] delim Delimiter
 *
 * \return If successful, the number of bytes read, including the trailing
 *         newline if one exists. If 0 is returned, then EOF is reached.
 *         Otherwise, the error code of fill_rbuf().
 */
oc::result<size_t> BufferedFile::read_sized_line(span<unsigned char> buf,
                                                 unsigned char delim)
{
    return get_delim([&buf](span<const unsigned char> data) {
        memcpy(buf.data(), data.data(), data.size());
        buf = buf.subspan(data.size());
    }, buf.size(), delim);
}

/*!
 * \brief Read underlying file
 *
 * \post The \ref m_fpos field is incremented by the number of bytes read
 *
 * \return Result of file_read_retry()
 */
oc::result<size_t> BufferedFile::read_underlying(span<unsigned char> buf)
{
    OUTCOME_TRY(n, file_read_retry(*m_file, buf));
    m_fpos += n;

    return n;
}

/*!
 * \brief Write underlying file
 *
 * \post The \ref m_fpos field is incremented by the number of bytes written
 *
 * \return Result of file_write_retry()
 */
oc::result<size_t> BufferedFile::write_underlying(span<const unsigned char> buf)
{
    OUTCOME_TRY(n, file_write_retry(*m_file, buf));
    m_fpos += n;

    return n;
}

/*!
 * \brief Write underlying file (failing if EOF)
 *
 * \post The \ref m_fpos field is incremented by the number of bytes written
 *
 * \return Result of file_write_exact()
 */
oc::result<void> BufferedFile::write_exact_underlying(span<const unsigned char> buf)
{
    OUTCOME_TRYV(file_write_exact(*m_file, buf));
    m_fpos += buf.size();

    return oc::success();
}

/*!
 * \brief Seek underlying file and clear buffer
 *
 * \post The \ref m_fpos field is updated with the result of the seek operation
 *       and the read buffer is cleared.
 *
 * \return Result of File::seek()
 */
oc::result<uint64_t> BufferedFile::seek_underlying(int64_t offset, int whence)
{
    OUTCOME_TRY(pos, m_file->seek(offset, whence));
    m_fpos = pos;
    m_rpos = m_rcap;

    return pos;
}

void BufferedFile::clear() noexcept
{
    m_file = nullptr;
    m_fpos = 0;
    m_rpos = 0;
    m_rcap = 0;
    m_wbuf.clear();
}

}
