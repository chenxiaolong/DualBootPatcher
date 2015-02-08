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

import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.net.LocalSocketAddress.Namespace;
import android.os.Environment;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;
import com.stericson.RootTools.execution.Command;

import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class MbtoolSocket {
    private static final String TAG = MbtoolSocket.class.getSimpleName();

    private static final String RESPONSE_ALLOW = "ALLOW";
    private static final String RESPONSE_DENY = "DENY";
    private static final String RESPONSE_OK = "OK";
    private static final String RESPONSE_SUCCESS = "SUCCESS";
    private static final String RESPONSE_FAIL = "FAIL";
    private static final String RESPONSE_UNSUPPORTED = "UNSUPPORTED";

    private static final String V1_COMMAND_LIST_ROMS = "LIST_ROMS";
    private static final String V1_COMMAND_CHOOSE_ROM = "CHOOSE_ROM";
    private static final String V1_COMMAND_SET_KERNEL = "SET_KERNEL";

    private static MbtoolSocket sInstance;

    private LocalSocket mSocket;
    private int mInterfaceVersion;

    // Keep this as a singleton class for now
    private MbtoolSocket() {
        // TODO: Handle other interface versions in the future
        mInterfaceVersion = 1;
    }

    public static MbtoolSocket getInstance() {
        if (sInstance == null) {
            sInstance = new MbtoolSocket();
        }
        return sInstance;
    }

    public void connect() throws SocketCommunicationException, SocketCredentialsDeniedException {
        mSocket = new LocalSocket();
        try {
            mSocket.connect(new LocalSocketAddress("mbtool.daemon", Namespace.ABSTRACT));

            OutputStream os = mSocket.getOutputStream();
            InputStream is = mSocket.getInputStream();

            // Verify credentials
            String response = readString(is);
            if (RESPONSE_DENY.equals(response)) {
                throw new SocketCredentialsDeniedException("mbtool explicitly denied access to " +
                        "the daemon. WARNING: This app is probably not officially signed!");
            } else if (!RESPONSE_ALLOW.equals(response)) {
                throw new SocketCommunicationException("Unexpected reply: " + response);
            }

            // Request an interface version
            writeInt32(os, mInterfaceVersion);
            response = readString(is);
            if (RESPONSE_UNSUPPORTED.equals(response)) {
                throw new SocketCommunicationException(
                        "Daemon does not support interface " + mInterfaceVersion);
            } else if (!RESPONSE_OK.equals(response)) {
                throw new SocketCommunicationException("Unexpected reply: " + response);
            }
        } catch (IOException e) {
            throw new SocketCommunicationException(e);
        }
    }

    public RomInformation[] getInstalledRoms() {
        ensureConnected();

        try {
            OutputStream os = mSocket.getOutputStream();
            InputStream is = mSocket.getInputStream();

            sendCommand(is, os, V1_COMMAND_LIST_ROMS);

            int length = readInt32(is);
            RomInformation[] roms = new RomInformation[length];
            String response;

            for (int i = 0; i < length; i++) {
                RomInformation rom = roms[i] = new RomInformation();
                boolean ready = false;

                while (true) {
                    response = readString(is);

                    if ("ROM_BEGIN".equals(response)) {
                        ready = true;
                    } else if (ready && "ID".equals(response)) {
                        rom.id = readString(is);
                    } else if (ready && "SYSTEM_PATH".equals(response)) {
                        rom.system = readString(is);
                    } else if (ready && "CACHE_PATH".equals(response)) {
                        rom.cache = readString(is);
                    } else if (ready && "DATA_PATH".equals(response)) {
                        rom.data = readString(is);
                    } else if (ready && "USE_RAW_PATHS".equals(response)) {
                        rom.useRawPaths = readInt32(is) != 0;
                    } else if (ready && "ROM_END".equals(response)) {
                        ready = false;
                        break;
                    } else {
                        Log.e(TAG, "getInstalledRoms(): Unknown response: " + response);
                    }
                }

                rom.thumbnailPath = Environment.getExternalStorageDirectory()
                        + "/MultiBoot/" + rom.id + "/thumbnail.webp";
            }

            // Command always returns OK at the end
            readString(is);

            return roms;
        } catch (IOException e) {
            e.printStackTrace();
        } catch (SocketCommunicationException e) {
            e.printStackTrace();
        }

        return null;
    }

    public boolean chooseRom(String id) {
        ensureConnected();

        try {
            OutputStream os = mSocket.getOutputStream();
            InputStream is = mSocket.getInputStream();

            sendCommand(is, os, V1_COMMAND_CHOOSE_ROM);

            String bootBlockDev = SwitcherUtils.getBootPartition();
            if (bootBlockDev == null) {
                Log.e(TAG, "Failed to determine boot partition");
                return false;
            }

            writeString(os, id);
            writeString(os, bootBlockDev);

            String response = readString(is);
            if (response.equals(RESPONSE_SUCCESS)) {
                return true;
            } else if (response.equals(RESPONSE_FAIL)) {
                return false;
            } else {
                throw new SocketCommunicationException("Invalid response: " + response);
            }
        } catch (IOException e) {
            e.printStackTrace();
        } catch (SocketCommunicationException e) {
            e.printStackTrace();
        }

        return false;
    }

    public boolean setKernel(String id) {
        ensureConnected();

        try {
            OutputStream os = mSocket.getOutputStream();
            InputStream is = mSocket.getInputStream();

            sendCommand(is, os, V1_COMMAND_SET_KERNEL);

            String bootBlockDev = SwitcherUtils.getBootPartition();
            if (bootBlockDev == null) {
                Log.e(TAG, "Failed to determine boot partition");
                return false;
            }

            writeString(os, id);
            writeString(os, bootBlockDev);

            String response = readString(is);
            if (response.equals(RESPONSE_SUCCESS)) {
                return true;
            } else if (response.equals(RESPONSE_FAIL)) {
                return false;
            } else {
                throw new SocketCommunicationException("Invalid response: " + response);
            }
        } catch (IOException e) {
            e.printStackTrace();
        } catch (SocketCommunicationException e) {
            e.printStackTrace();
        }

        return false;
    }

    // Exceptions

    public static class SocketCredentialsDeniedException extends Exception {
        public SocketCredentialsDeniedException(String message) {
            super(message);
        }

        public SocketCredentialsDeniedException(Throwable throwable) {
            super(throwable);
        }

        public SocketCredentialsDeniedException(String message, Throwable throwable) {
            super(message, throwable);
        }
    }

    public static class SocketCommunicationException extends Exception {
        public SocketCommunicationException(String message) {
            super(message);
        }

        public SocketCommunicationException(Throwable throwable) {
            super(throwable);
        }

        public SocketCommunicationException(String message, Throwable throwable) {
            super(message, throwable);
        }
    }

    private void ensureConnected() {
        if (mSocket == null || !mSocket.isConnected()) {
            throw new IllegalStateException("Not connected to mbtool daemon");
        }
    }

    // Private helper functions

    private static void sendCommand(InputStream is, OutputStream os, String command) throws
            IOException, SocketCommunicationException {
        writeString(os, command);

        String response = readString(is);
        if (RESPONSE_UNSUPPORTED.equals(response)) {
            throw new SocketCommunicationException("Unsupported command: " + command);
        } else if (!RESPONSE_OK.equals(response)) {
            throw new SocketCommunicationException("Unknown response: " + response);
        }
    }

    private static void readFully(InputStream is, byte[] buf, int offset, int length) throws
            IOException {
        while (length > 0) {
            int n = is.read(buf, offset, length);
            if (n < 0) {
                throw new EOFException();
            }
            offset += n;
            length -= n;
        }
    }

    private static short readInt16(InputStream is) throws IOException {
        byte[] buf = new byte[2];
        readFully(is, buf, 0, 2);
        return ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).getShort();
    }

    private static void writeInt16(OutputStream os, short num) throws IOException {
        os.write(ByteBuffer.allocate(2).order(ByteOrder.LITTLE_ENDIAN).putShort(num).array());
    }

    private static int readInt32(InputStream is) throws IOException {
        byte[] buf = new byte[4];
        readFully(is, buf, 0, 4);
        return ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).getInt();
    }

    private static void writeInt32(OutputStream os, int num) throws IOException {
        os.write(ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(num).array());
    }

    private static long readInt64(InputStream is) throws IOException {
        byte[] buf = new byte[8];
        readFully(is, buf, 0, 8);
        return ByteBuffer.wrap(buf).order(ByteOrder.LITTLE_ENDIAN).getLong();
    }

    private static void writeInt64(OutputStream os, long num) throws IOException {
        os.write(ByteBuffer.allocate(8).order(ByteOrder.LITTLE_ENDIAN).putLong(num).array());
    }

    private static String readString(InputStream is) throws IOException {
        int length = readInt32(is);
        if (length < 0) {
            throw new IOException("Invalid string length");
        }

        byte[] str = new byte[length];
        readFully(is, str, 0, length);
        return new String(str);
    }

    private static void writeString(OutputStream os, String str) throws IOException {
        writeInt32(os, str.length());
        os.write(str.getBytes());
    }
}
