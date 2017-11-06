/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file/callbacks_p.h"
#include "mbcommon/finally.h"

/*!
 * \file mbcommon/file/callbacks.h
 * \brief Open file with callbacks
 */

// Typedefs documentation

namespace mb
{

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
 * up the resources, regardless of the current state. In other words, this
 * callback will be called even if is_fatal() returns true. If any memory, file
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

/*! \cond INTERNAL */

CallbackFilePrivate::CallbackFilePrivate()
{
    clear();
}

CallbackFilePrivate::~CallbackFilePrivate() = default;

void CallbackFilePrivate::clear()
{
    open_cb = nullptr;
    close_cb = nullptr;
    read_cb = nullptr;
    write_cb = nullptr;
    seek_cb = nullptr;
    truncate_cb = nullptr;
    userdata = nullptr;
}

/*! \endcond */

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
    : CallbackFile(new CallbackFilePrivate())
{
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
 * \param userdata Data pointer to pass to callbacks
 */
CallbackFile::CallbackFile(OpenCb open_cb,
                           CloseCb close_cb,
                           ReadCb read_cb,
                           WriteCb write_cb,
                           SeekCb seek_cb,
                           TruncateCb truncate_cb,
                           void *userdata)
    : CallbackFile(new CallbackFilePrivate(), open_cb, close_cb, read_cb,
                   write_cb, seek_cb, truncate_cb, userdata)
{
}

/*! \cond INTERNAL */

CallbackFile::CallbackFile(CallbackFilePrivate *priv)
    : File(priv)
{
}

CallbackFile::CallbackFile(CallbackFilePrivate *priv,
                           OpenCb open_cb,
                           CloseCb close_cb,
                           ReadCb read_cb,
                           WriteCb write_cb,
                           SeekCb seek_cb,
                           TruncateCb truncate_cb,
                           void *userdata)
    : File(priv)
{
    (void) open(open_cb, close_cb, read_cb, write_cb, seek_cb, truncate_cb,
                userdata);
}

/*! \endcond */

CallbackFile::~CallbackFile()
{
    (void) close();
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
 * \param userdata Data pointer to pass to callbacks
 */
oc::result<void> CallbackFile::open(OpenCb open_cb,
                                    CloseCb close_cb,
                                    ReadCb read_cb,
                                    WriteCb write_cb,
                                    SeekCb seek_cb,
                                    TruncateCb truncate_cb,
                                    void *userdata)
{
    MB_PRIVATE(CallbackFile);

    // pimpl can be NULL if the handle was moved. File::open() will fail
    // appropriately in that case.
    if (priv) {
        priv->open_cb = open_cb;
        priv->close_cb = close_cb;
        priv->read_cb = read_cb;
        priv->write_cb = write_cb;
        priv->seek_cb = seek_cb;
        priv->truncate_cb = truncate_cb;
        priv->userdata = userdata;
    }

    return File::open();
}

oc::result<void> CallbackFile::on_open()
{
    MB_PRIVATE(CallbackFile);

    if (priv->open_cb) {
        return priv->open_cb(*this, priv->userdata);
    } else {
        return File::on_open();
    }
}

oc::result<void> CallbackFile::on_close()
{
    MB_PRIVATE(CallbackFile);

    // Reset to allow opening another file
    auto reset = finally([&] {
        priv->clear();
    });

    if (priv->close_cb) {
        return priv->close_cb(*this, priv->userdata);
    } else {
        return File::on_close();
    }
}

oc::result<size_t> CallbackFile::on_read(void *buf, size_t size)
{
    MB_PRIVATE(CallbackFile);

    if (priv->read_cb) {
        return priv->read_cb(*this, priv->userdata, buf, size);
    } else {
        return File::on_read(buf, size);
    }
}

oc::result<size_t> CallbackFile::on_write(const void *buf, size_t size)
{
    MB_PRIVATE(CallbackFile);

    if (priv->write_cb) {
        return priv->write_cb(*this, priv->userdata, buf, size);
    } else {
        return File::on_write(buf, size);
    }
}

oc::result<uint64_t> CallbackFile::on_seek(int64_t offset, int whence)
{
    MB_PRIVATE(CallbackFile);

    if (priv->seek_cb) {
        return priv->seek_cb(*this, priv->userdata, offset, whence);
    } else {
        return File::on_seek(offset, whence);
    }
}

oc::result<void> CallbackFile::on_truncate(uint64_t size)
{
    MB_PRIVATE(CallbackFile);

    if (priv->truncate_cb) {
        return priv->truncate_cb(*this, priv->userdata, size);
    } else {
        return File::on_truncate(size);
    }
}

}
