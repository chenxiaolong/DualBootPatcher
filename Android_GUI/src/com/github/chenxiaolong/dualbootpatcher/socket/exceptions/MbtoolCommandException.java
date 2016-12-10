/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.github.chenxiaolong.dualbootpatcher.socket.exceptions;

import android.support.annotation.NonNull;

/**
 * Exception thrown when an RPC call fails.
 *
 * If this exception is thrown, the mbtool connection can still be used.
 */
public class MbtoolCommandException extends Exception {
    private final int mErrno;

    public MbtoolCommandException(String detailMessage) {
        super(detailMessage);
        mErrno = -1;
    }

    public MbtoolCommandException(String message, Throwable cause) {
        super(message, cause);
        mErrno = -1;
    }

    public MbtoolCommandException(int errno, String detailMessage) {
        super(getMessageWithErrno(errno, detailMessage));
        mErrno = errno;
    }

    public MbtoolCommandException(int errno, String message, Throwable cause) {
        super(getMessageWithErrno(errno, message), cause);
        mErrno = errno;
    }

    @NonNull
    private static String getMessageWithErrno(int errno, String message) {
        return message + " [errno = " + errno + "]";
    }

    public int getErrno() {
        return mErrno;
    }
}
