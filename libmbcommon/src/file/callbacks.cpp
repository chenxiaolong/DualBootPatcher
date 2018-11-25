/*
 * Copyright (C) 2016-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file/callbacks.h"

#include "mbcommon/finally.h"

/*!
 * \file mbcommon/file/callbacks.h
 * \brief Open file with callbacks
 */

// Typedefs documentation

namespace mb
{

using namespace detail;

/*!
 * \typedef CallbackFile::OpenCb
 *
 * \brief File open callback
 *
 * \note If an error code is returned, the #CallbackFile::CloseCb callback, if
 *       registered, will be called to clean up the resources.
 *
 * \param file File handle
 *
 * \return Return nothing if the file was successfully opened. Otherwise, return
 *         an error code.
 */

/*!
 * \typedef CallbackFile::CloseCb
 *
 * \brief File close callback
 *
 * This callback, if registered, will be called once and only once once to clean
 * up the resources, regardless of the current state. If any memory, file
 * handles, or other resources need to be freed, this callback is the place to
 * do so.
 *
 * It is guaranteed that no further callbacks will be invoked after this
 * callback executes.
 *
 * \param file File handle
 *
 * \return Return nothing if the file was successfully closed. Otherwise, return
 *         an error code.
 */

/*!
 * \typedef CallbackFile::ReadCb
 *
 * \brief File read callback
 *
 * \param[in] file File handle
 * \param[out] buf Buffer to read into
 * \param[in] size Buffer size
 *
 * \return Return the number of bytes read if some were successfully read or EOF
 *         was reached. Otherwise, return an error.
 */

/*!
 * \typedef CallbackFile::WriteCb
 *
 * \brief File write callback
 *
 * \param file File handle
 * \param buf Buffer to write from
 * \param size Buffer size
 *
 * \return Return the number of bytes written if some were successfully written
 *         or EOF was reached. Otherwise, return an error code.
 */

/*!
 * \typedef CallbackFile::SeekCb
 *
 * \brief File seek callback
 *
 * \param file File handle
 * \param offset File position offset
 * \param whence SEEK_SET, SEEK_CUR, or SEEK_END from `stdio.h`
 *
 * \return Return the new file offset if the file position was successfully set.
 *         Otherwise, return an error code.
 */

/*!
 * \typedef CallbackFile::TruncateCb
 *
 * \brief File truncate callback
 *
 * \note This callback must *not* change the file position.
 *
 * \param file File handle
 * \param size New size of file
 *
 * \return Return nothing if the file size was successfully changed. Otherwise,
 *         return an error code.
 */

/*!
 * \class CallbackFile
 *
 * \brief Open file with C-style callbacks.
 */

/*!
 * \brief Construct unbound CallbackFile.
 *
 * The File handle will not be bound to any file. The open(OpenCb, CloseCb,
 * ReadCb, WriteCb, SeekCb, TruncateCb, void *) function will need to be called
 * to open a file.
 */
CallbackFile::CallbackFile()
    : File()
{
    clear();
}

/*!
 * \brief Open File handle with callbacks.
 *
 * Construct the file handle and open the file. Use is_open() to check if the
 * file was successfully opened.
 *
 * \sa open(OpenCb, CloseCb, ReadCb, WriteCb, SeekCb, TruncateCb, void *)
 *
 * \param open_cb File open callback
 * \param close_cb File close callback
 * \param read_cb File read callback
 * \param write_cb File write callback
 * \param seek_cb File seek callback
 * \param truncate_cb File truncate callback
 */
CallbackFile::CallbackFile(OpenCb open_cb,
                           CloseCb close_cb,
                           ReadCb read_cb,
                           WriteCb write_cb,
                           SeekCb seek_cb,
                           TruncateCb truncate_cb)
    : CallbackFile()
{
    (void) open(std::move(open_cb), std::move(close_cb), std::move(read_cb),
                std::move(write_cb), std::move(seek_cb), std::move(truncate_cb));
}

CallbackFile::~CallbackFile()
{
    (void) close();
}

CallbackFile::CallbackFile(CallbackFile &&other) noexcept
    : File(std::move(other))
    , m_open_cb(std::move(other.m_open_cb))
    , m_close_cb(std::move(other.m_close_cb))
    , m_read_cb(std::move(other.m_read_cb))
    , m_write_cb(std::move(other.m_write_cb))
    , m_seek_cb(std::move(other.m_seek_cb))
    , m_truncate_cb(std::move(other.m_truncate_cb))
{
    other.clear();
}

CallbackFile & CallbackFile::operator=(CallbackFile &&rhs) noexcept
{
    File::operator=(std::move(rhs));

    m_open_cb = std::move(rhs.m_open_cb);
    m_close_cb = std::move(rhs.m_close_cb);
    m_read_cb = std::move(rhs.m_read_cb);
    m_write_cb = std::move(rhs.m_write_cb);
    m_seek_cb = std::move(rhs.m_seek_cb);
    m_truncate_cb = std::move(rhs.m_truncate_cb);

    rhs.clear();

    return *this;
}

/*!
 * \brief Open file with callbacks.
 *
 * If \p open_cb is provided and returns a failure code, then \p close_cb, if
 * provided, will be called to clean up any resources.
 *
 * \param open_cb File open callback
 * \param close_cb File close callback
 * \param read_cb File read callback
 * \param write_cb File write callback
 * \param seek_cb File seek callback
 * \param truncate_cb File truncate callback
 */
oc::result<void> CallbackFile::open(OpenCb open_cb,
                                    CloseCb close_cb,
                                    ReadCb read_cb,
                                    WriteCb write_cb,
                                    SeekCb seek_cb,
                                    TruncateCb truncate_cb)
{
    if (state() == FileState::New) {
        m_open_cb = std::move(open_cb);
        m_close_cb = std::move(close_cb);
        m_read_cb = std::move(read_cb);
        m_write_cb = std::move(write_cb);
        m_seek_cb = std::move(seek_cb);
        m_truncate_cb = std::move(truncate_cb);
    }

    return File::open();
}

oc::result<void> CallbackFile::on_open()
{
    if (m_open_cb) {
        return m_open_cb(*this);
    } else {
        return File::on_open();
    }
}

oc::result<void> CallbackFile::on_close()
{
    // Reset to allow opening another file
    auto reset = finally([&] {
        clear();
    });

    if (m_close_cb) {
        return m_close_cb(*this);
    } else {
        return File::on_close();
    }
}

oc::result<size_t> CallbackFile::on_read(void *buf, size_t size)
{
    if (m_read_cb) {
        return m_read_cb(*this, buf, size);
    } else {
        return File::on_read(buf, size);
    }
}

oc::result<size_t> CallbackFile::on_write(const void *buf, size_t size)
{
    if (m_write_cb) {
        return m_write_cb(*this, buf, size);
    } else {
        return File::on_write(buf, size);
    }
}

oc::result<uint64_t> CallbackFile::on_seek(int64_t offset, int whence)
{
    if (m_seek_cb) {
        return m_seek_cb(*this, offset, whence);
    } else {
        return File::on_seek(offset, whence);
    }
}

oc::result<void> CallbackFile::on_truncate(uint64_t size)
{
    if (m_truncate_cb) {
        return m_truncate_cb(*this, size);
    } else {
        return File::on_truncate(size);
    }
}

void CallbackFile::clear()
{
    m_open_cb = nullptr;
    m_close_cb = nullptr;
    m_read_cb = nullptr;
    m_write_cb = nullptr;
    m_seek_cb = nullptr;
    m_truncate_cb = nullptr;
}

}
