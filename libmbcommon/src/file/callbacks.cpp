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

/*!
 * \file mbcommon/file/callbacks.h
 * \brief Open file with callbacks
 */

MB_BEGIN_C_DECLS

/*!
 * Open MbFile handle with callbacks.
 *
 * This is a wrapper function around the `mb_file_set_*_callback()` functions.
 *
 * \param file MbFile handle
 * \param open_cb File open callback
 * \param close_cb File close callback
 * \param read_cb File read callback
 * \param write_cb File write callback
 * \param seek_cb File seek callback
 * \param truncate_cb File truncate callback
 * \param userdata Data pointer to pass to callbacks
 *
 * \return The minimum return value of the `mb_file_set_*_callback()` functions,
 *         mb_file_set_callback_data(), and mb_file_open()
 */
int mb_file_open_callbacks(struct MbFile *file,
                           MbFileOpenCb open_cb,
                           MbFileCloseCb close_cb,
                           MbFileReadCb read_cb,
                           MbFileWriteCb write_cb,
                           MbFileSeekCb seek_cb,
                           MbFileTruncateCb truncate_cb,
                           void *userdata)
{
    int ret = MB_FILE_OK;
    int ret2;

    ret2 = mb_file_set_open_callback(file, open_cb);
    if (ret2 < ret) {
        ret = ret2;
    }

    ret2 = mb_file_set_close_callback(file, close_cb);
    if (ret2 < ret) {
        ret = ret2;
    }

    ret2 = mb_file_set_read_callback(file, read_cb);
    if (ret2 < ret) {
        ret = ret2;
    }

    ret2 = mb_file_set_write_callback(file, write_cb);
    if (ret2 < ret) {
        ret = ret2;
    }

    ret2 = mb_file_set_seek_callback(file, seek_cb);
    if (ret2 < ret) {
        ret = ret2;
    }

    ret2 = mb_file_set_truncate_callback(file, truncate_cb);
    if (ret2 < ret) {
        ret = ret2;
    }

    ret2 = mb_file_set_callback_data(file, userdata);
    if (ret2 < ret) {
        ret = ret2;
    }

    ret2 = mb_file_open(file);
    if (ret2 < ret) {
        ret = ret2;
    }

    return ret;
}

MB_END_C_DECLS
