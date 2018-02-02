/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.socket

import java.io.EOFException
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.nio.ByteBuffer
import java.nio.ByteOrder

object SocketUtils {
    @Throws(IOException::class)
    fun readFully(`is`: InputStream, buf: ByteArray, offset: Int, length: Int) {
        var curOffset = offset
        var curLength = length
        while (curLength > 0) {
            val n = `is`.read(buf, curOffset, curLength)
            if (n < 0) {
                throw EOFException()
            }
            curOffset += n
            curLength -= n
        }
    }

    @Throws(IOException::class)
    fun readBytes(`is`: InputStream): ByteArray {
        val length = readInt32(`is`)
        if (length < 0) {
            throw IOException("Invalid byte array length")
        }

        val str = ByteArray(length)
        readFully(`is`, str, 0, length)
        return str
    }

    @Throws(IOException::class)
    fun writeBytes(os: OutputStream, bytes: ByteArray) {
        writeInt32(os, bytes.size)
        os.write(bytes)
    }

    @Throws(IOException::class)
    fun readInt16(`is`: InputStream): Short {
        val buf = ByteArray(2)
        readFully(`is`, buf, 0, 2)
        return ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).short
    }

    @Throws(IOException::class)
    fun writeInt16(os: OutputStream, num: Short) {
        os.write(ByteBuffer.allocate(2).order(ByteOrder.LITTLE_ENDIAN).putShort(num).array())
    }

    @Throws(IOException::class)
    fun readInt32(`is`: InputStream): Int {
        val buf = ByteArray(4)
        readFully(`is`, buf, 0, 4)
        return ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).int
    }

    @Throws(IOException::class)
    fun writeInt32(os: OutputStream, num: Int) {
        os.write(ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(num).array())
    }

    @Throws(IOException::class)
    fun readInt64(`is`: InputStream): Long {
        val buf = ByteArray(8)
        readFully(`is`, buf, 0, 8)
        return ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).long
    }

    @Throws(IOException::class)
    fun writeInt64(os: OutputStream, num: Long) {
        os.write(ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(num).array())
    }

    @Throws(IOException::class)
    fun readString(`is`: InputStream): String {
        val length = readInt32(`is`)
        if (length < 0) {
            throw IOException("Invalid string length")
        }

        val str = ByteArray(length)
        readFully(`is`, str, 0, length)
        return String(str)
    }

    @Throws(IOException::class)
    fun writeString(os: OutputStream, str: String) {
        writeInt32(os, str.length)
        os.write(str.toByteArray())
    }

    @Throws(IOException::class)
    fun readStringArray(`is`: InputStream): Array<String> {
        val length = readInt32(`is`)
        if (length < 0) {
            throw IOException("Invalid array length")
        }

        return (0 until length).map { readString(`is`) }.toTypedArray()
    }

    @Throws(IOException::class)
    fun writeStringArray(os: OutputStream, array: Array<String>) {
        writeInt32(os, array.size)

        for (str in array) {
            writeString(os, str)
        }
    }
}
