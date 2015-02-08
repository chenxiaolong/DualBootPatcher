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

package com.github.chenxiaolong.multibootpatcher.socket;

import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class SocketUtils {
    public static void readFully(InputStream is, byte[] buf, int offset,
                                 int length) throws IOException {
        while (length > 0) {
            int n = is.read(buf, offset, length);
            if (n < 0) {
                throw new EOFException();
            }
            offset += n;
            length -= n;
        }
    }

    public static short readInt16(InputStream is) throws IOException {
        byte[] buf = new byte[2];
        readFully(is, buf, 0, 2);
        return ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).getShort();
    }

    public static void writeInt16(OutputStream os, short num) throws IOException {
        os.write(ByteBuffer.allocate(2).order(ByteOrder.LITTLE_ENDIAN).putShort(num).array());
    }

    public static int readInt32(InputStream is) throws IOException {
        byte[] buf = new byte[4];
        readFully(is, buf, 0, 4);
        return ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).getInt();
    }

    public static void writeInt32(OutputStream os, int num) throws IOException {
        os.write(ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(num).array());
    }

    public static long readInt64(InputStream is) throws IOException {
        byte[] buf = new byte[8];
        readFully(is, buf, 0, 8);
        return ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).getLong();
    }

    public static void writeInt64(OutputStream os, long num) throws IOException {
        os.write(ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(num).array());
    }

    public static String readString(InputStream is) throws IOException {
        int length = readInt32(is);
        if (length < 0) {
            throw new IOException("Invalid string length");
        }

        byte[] str = new byte[length];
        readFully(is, str, 0, length);
        return new String(str);
    }

    public static void writeString(OutputStream os, String str) throws IOException {
        writeInt32(os, str.length());
        os.write(str.getBytes());
    }
}
