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
 * Exception thrown when there is a communication error with mbtool.
 *
 * If this exception is thrown, the mbtool connection can no longer be used.
 */
class MbtoolException : Exception {
    val reason: Reason

    enum class Reason {
        // mbtool is not running
        DAEMON_NOT_RUNNING,
        // mbtool rejected the connection due to failing signature check
        SIGNATURE_CHECK_FAIL,
        // Protocol version not supported
        INTERFACE_NOT_SUPPORTED,
        // mbtool version too old
        VERSION_TOO_OLD,
        // Protocol error
        PROTOCOL_ERROR
    }

    constructor(reason: Reason, detailMessage: String) : super(detailMessage) {
        this.reason = reason
    }

    constructor(reason: Reason, message: String, cause: Throwable) : super(message, cause) {
        this.reason = reason
    }

    constructor(reason: Reason, cause: Throwable) : super(cause) {
        this.reason = reason
    }
}