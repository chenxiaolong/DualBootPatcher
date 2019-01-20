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

package com.github.chenxiaolong.dualbootpatcher.socket.exceptions

/**
 * Exception thrown when an RPC call fails.
 *
 * If this exception is thrown, the mbtool connection can still be used.
 */
class MbtoolCommandException : Exception {
    val errno: Int

    constructor(detailMessage: String) : this(-1, detailMessage)

    constructor(message: String, cause: Throwable) : this(-1, message, cause)

    constructor(errno: Int, detailMessage: String)
            : super(getMessageWithErrno(errno, detailMessage)) {
        this.errno = errno
    }

    constructor(errno: Int, message: String, cause: Throwable)
            : super(getMessageWithErrno(errno, message), cause) {
        this.errno = errno
    }

    companion object {
        private fun getMessageWithErrno(errno: Int, message: String): String {
            return "$message [errno = $errno]"
        }
    }
}