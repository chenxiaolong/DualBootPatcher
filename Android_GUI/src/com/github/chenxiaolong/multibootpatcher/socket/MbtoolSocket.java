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

import org.apache.commons.io.IOUtils;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

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
    private static final String V1_COMMAND_REBOOT = "REBOOT";

    private static MbtoolSocket sInstance;

    private LocalSocket mSocket;
    private InputStream mSocketIS;
    private OutputStream mSocketOS;
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

    public boolean connect() {
        // Already connected
        if (mSocket != null) {
            return true;
        }

        Log.i(TAG, "Connecting to mbtool");
        try {
            mSocket = new LocalSocket();
            mSocket.connect(new LocalSocketAddress("mbtool.daemon", Namespace.ABSTRACT));

            mSocketIS = mSocket.getInputStream();
            mSocketOS = mSocket.getOutputStream();

            // Verify credentials
            String response = SocketUtils.readString(mSocketIS);
            if (RESPONSE_DENY.equals(response)) {
                throw new SocketProtocolException("mbtool explicitly denied access to the " +
                        "daemon. WARNING: This app is probably not officially signed!");
            } else if (!RESPONSE_ALLOW.equals(response)) {
                throw new SocketProtocolException("Unexpected reply: " + response);
            }

            // Request an interface version
            SocketUtils.writeInt32(mSocketOS, mInterfaceVersion);
            response = SocketUtils.readString(mSocketIS);
            if (RESPONSE_UNSUPPORTED.equals(response)) {
                throw new SocketProtocolException("Daemon does not support interface " +
                        mInterfaceVersion);
            } else if (!RESPONSE_OK.equals(response)) {
                throw new SocketProtocolException("Unexpected reply: " + response);
            }

            return true;
        } catch (IOException e) {
            e.printStackTrace();
            disconnect();
        } catch (SocketProtocolException e) {
            e.printStackTrace();
        }

        return false;
    }

    public void disconnect() {
        Log.i(TAG, "Disconnecting from mbtool");
        IOUtils.closeQuietly(mSocket);
        IOUtils.closeQuietly(mSocketIS);
        IOUtils.closeQuietly(mSocketOS);
        mSocket = null;
        mSocketIS = null;
        mSocketOS = null;
    }

    public RomInformation[] getInstalledRoms() {
        if (!connect()) {
            return null;
        }

        try {
            sendCommand(V1_COMMAND_LIST_ROMS);

            int length = SocketUtils.readInt32(mSocketIS);
            RomInformation[] roms = new RomInformation[length];
            String response;

            for (int i = 0; i < length; i++) {
                RomInformation rom = roms[i] = new RomInformation();
                boolean ready = false;

                while (true) {
                    response = SocketUtils.readString(mSocketIS);

                    if ("ROM_BEGIN".equals(response)) {
                        ready = true;
                    } else if (ready && "ID".equals(response)) {
                        rom.id = SocketUtils.readString(mSocketIS);
                    } else if (ready && "SYSTEM_PATH".equals(response)) {
                        rom.system = SocketUtils.readString(mSocketIS);
                    } else if (ready && "CACHE_PATH".equals(response)) {
                        rom.cache = SocketUtils.readString(mSocketIS);
                    } else if (ready && "DATA_PATH".equals(response)) {
                        rom.data = SocketUtils.readString(mSocketIS);
                    } else if (ready && "USE_RAW_PATHS".equals(response)) {
                        rom.useRawPaths = SocketUtils.readInt32(mSocketIS) != 0;
                    } else if (ready && "ROM_END".equals(response)) {
                        ready = false;
                        break;
                    } else {
                        Log.e(TAG, "getInstalledRoms(): Unknown response: " + response);
                    }
                }

                rom.thumbnailPath = Environment.getExternalStorageDirectory() + "/MultiBoot/" +
                        rom.id + "/thumbnail.webp";
            }

            // Command always returns OK at the end
            SocketUtils.readString(mSocketIS);

            return roms;
        } catch (IOException e) {
            e.printStackTrace();
            disconnect();
        } catch (SocketProtocolException e) {
            e.printStackTrace();
        }

        return null;
    }

    public boolean chooseRom(String id) {
        if (!connect()) {
            return false;
        }

        try {
            sendCommand(V1_COMMAND_CHOOSE_ROM);

            String bootBlockDev = SwitcherUtils.getBootPartition();
            if (bootBlockDev == null) {
                Log.e(TAG, "Failed to determine boot partition");
                return false;
            }

            SocketUtils.writeString(mSocketOS, id);
            SocketUtils.writeString(mSocketOS, bootBlockDev);

            String response = SocketUtils.readString(mSocketIS);
            switch (response) {
            case RESPONSE_SUCCESS:
                return true;
            case RESPONSE_FAIL:
                return false;
            default:
                throw new SocketProtocolException("Invalid response: " + response);
            }
        } catch (IOException e) {
            e.printStackTrace();
            disconnect();
        } catch (SocketProtocolException e) {
            e.printStackTrace();
        }

        return false;
    }

    public boolean setKernel(String id) {
        if (!connect()) {
            return false;
        }

        try {
            sendCommand(V1_COMMAND_SET_KERNEL);

            String bootBlockDev = SwitcherUtils.getBootPartition();
            if (bootBlockDev == null) {
                Log.e(TAG, "Failed to determine boot partition");
                return false;
            }

            SocketUtils.writeString(mSocketOS, id);
            SocketUtils.writeString(mSocketOS, bootBlockDev);

            String response = SocketUtils.readString(mSocketIS);
            switch (response) {
            case RESPONSE_SUCCESS:
                return true;
            case RESPONSE_FAIL:
                return false;
            default:
                throw new SocketProtocolException("Invalid response: " + response);
            }
        } catch (IOException e) {
            e.printStackTrace();
            disconnect();
        } catch (SocketProtocolException e) {
            e.printStackTrace();
        }

        return false;
    }

    public boolean restart(String arg) {
        if (!connect()) {
            return false;
        }

        try {
            sendCommand(V1_COMMAND_REBOOT);

            SocketUtils.writeString(mSocketOS, arg);

            String response = SocketUtils.readString(mSocketIS);
            if (response.equals(RESPONSE_FAIL)) {
                return false;
            } else {
                throw new SocketProtocolException("Invalid response: " + response);
            }
        } catch (IOException e) {
            e.printStackTrace();
            disconnect();
        } catch (SocketProtocolException e) {
            e.printStackTrace();
        }

        return false;
    }

    // Exceptions

    public static class SocketProtocolException extends Exception {
        public SocketProtocolException(String message) {
            super(message);
        }

        public SocketProtocolException(Throwable throwable) {
            super(throwable);
        }

        public SocketProtocolException(String message, Throwable throwable) {
            super(message, throwable);
        }
    }

    // Private helper functions

    private void sendCommand(String command) throws IOException, SocketProtocolException {
        SocketUtils.writeString(mSocketOS, command);

        String response = SocketUtils.readString(mSocketIS);
        if (RESPONSE_UNSUPPORTED.equals(response)) {
            throw new SocketProtocolException("Unsupported command: " + command);
        } else if (!RESPONSE_OK.equals(response)) {
            throw new SocketProtocolException("Unknown response: " + response);
        }
    }
}
