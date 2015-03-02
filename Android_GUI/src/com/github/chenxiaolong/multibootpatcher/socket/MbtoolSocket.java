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

import android.content.Context;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.net.LocalSocketAddress.Namespace;
import android.os.Build;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;
import com.github.chenxiaolong.multibootpatcher.Version;
import com.github.chenxiaolong.multibootpatcher.patcher.PatcherUtils;

import org.apache.commons.io.IOUtils;

import java.io.FileDescriptor;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class MbtoolSocket {
    private static final String TAG = MbtoolSocket.class.getSimpleName();

    private static final String SOCKET_ADDRESS = "mbtool.daemon";

    private static final String RESPONSE_ALLOW = "ALLOW";
    private static final String RESPONSE_DENY = "DENY";
    private static final String RESPONSE_OK = "OK";
    private static final String RESPONSE_SUCCESS = "SUCCESS";
    private static final String RESPONSE_FAIL = "FAIL";
    private static final String RESPONSE_UNSUPPORTED = "UNSUPPORTED";

    private static final String V1_COMMAND_VERSION = "VERSION";
    private static final String V1_COMMAND_LIST_ROMS = "LIST_ROMS";
    private static final String V1_COMMAND_CURRENT_ROM = "CURRENT_ROM";
    private static final String V1_COMMAND_CHOOSE_ROM = "CHOOSE_ROM";
    private static final String V1_COMMAND_SET_KERNEL = "SET_KERNEL";
    private static final String V1_COMMAND_REBOOT = "REBOOT";
    private static final String V1_COMMAND_OPEN = "OPEN";
    private static final String V1_COMMAND_COPY = "COPY";
    private static final String V1_COMMAND_CHMOD = "CHMOD";
    private static final String V1_COMMAND_LOKI_PATCH = "LOKI_PATCH";

    private static MbtoolSocket sInstance;

    private LocalSocket mSocket;
    private InputStream mSocketIS;
    private OutputStream mSocketOS;
    private int mInterfaceVersion;
    private String mMbtoolVersion;

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

    /**
     * Initializes the mbtool connection.
     *
     * 1. Setup input and output streams
     * 2. Check to make sure mbtool authorized our connection
     * 3. Request interface version and check if the daemon supports it
     * 4. Get mbtool version from daemon
     *
     * @throws IOException
     */
    private void initializeConnection() throws IOException {
        mSocketIS = mSocket.getInputStream();
        mSocketOS = mSocket.getOutputStream();

        // Verify credentials
        String response = SocketUtils.readString(mSocketIS);
        if (RESPONSE_DENY.equals(response)) {
            throw new IOException("mbtool explicitly denied access to the daemon. " +
                    "WARNING: This app is probably not officially signed!");
        } else if (!RESPONSE_ALLOW.equals(response)) {
            throw new IOException("Unexpected reply: " + response);
        }

        // Request an interface version
        SocketUtils.writeInt32(mSocketOS, mInterfaceVersion);
        response = SocketUtils.readString(mSocketIS);
        if (RESPONSE_UNSUPPORTED.equals(response)) {
            throw new IOException("Daemon does not support interface " + mInterfaceVersion);
        } else if (!RESPONSE_OK.equals(response)) {
            throw new IOException("Unexpected reply: " + response);
        }

        // Get mbtool version
        sendCommand(V1_COMMAND_VERSION);
        mMbtoolVersion = SocketUtils.readString(mSocketIS);
        if (mMbtoolVersion == null) {
            throw new IOException("Could not determine mbtool version");
        }

        Version v1 = new Version(mMbtoolVersion);
        Version v2 = MbtoolUtils.getMinimumRequiredVersion();

        Log.v(TAG, "mbtool version: " + v1);
        Log.v(TAG, "minimum version: " + v2);

        // Ensure that the version is newer than the minimum required version
        if (v1.compareTo(v2) < 0) {
            throw new IOException("mbtool version is: " + v1 + ", " +
                    "minimum needed is: " + v2);
        }
    }

    /**
     * Connects to the mbtool socket
     *
     * 1. Try connecting to the socket
     * 2. If that fails or if the version is too old, launch bundled mbtool and connect again
     * 3. If that fails, then return false
     *
     * @param context Application context
     * @return True if successfully connected or if already connected. False, otherwise
     */
    public boolean connect(Context context) {
        // If we're already connected, then we're good
        if (mSocket != null) {
            return true;
        }

        // Try connecting to the socket
        try {
            mSocket = new LocalSocket();
            mSocket.connect(new LocalSocketAddress(SOCKET_ADDRESS, Namespace.ABSTRACT));
            initializeConnection();
            return true;
        } catch (IOException e) {
            Log.e(TAG, "Could not connect to mbtool socket", e);
            disconnect();
        }

        Log.v(TAG, "Launching bundled mbtool");

        if (!executeMbtool(context)) {
            Log.e(TAG, "Failed to execute mbtool");
            return false;
        }

        try {
            Thread.sleep(1000);
            mSocket = new LocalSocket();
            mSocket.connect(new LocalSocketAddress(SOCKET_ADDRESS, Namespace.ABSTRACT));
            initializeConnection();
            return true;
        } catch (InterruptedException e) {
            e.printStackTrace();
        } catch (IOException e) {
            Log.e(TAG, "Could not connect to mbtool socket", e);
            disconnect();
        }

        return false;
    }

    private boolean executeMbtool(Context context) {
        PatcherUtils.extractPatcher(context);
        String abi;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            abi = Build.SUPPORTED_ABIS[0];
        } else {
            abi = Build.CPU_ABI;
        }

        String mbtool = PatcherUtils.getTargetDirectory(context)
                + "/binaries/android/" + abi + "/mbtool";

        return CommandUtils.runRootCommand("mount -o remount,rw /") == 0
                && CommandUtils.runRootCommand("mv /mbtool /mbtool.bak || :") == 0
                && CommandUtils.runRootCommand("cp " + mbtool + " /mbtool") == 0
                && CommandUtils.runRootCommand("chmod 755 /mbtool") == 0
                && CommandUtils.runRootCommand("mount -o remount,ro /") == 0
                && CommandUtils.runRootCommand("/mbtool daemon --replace --daemonize") == 0;
    }

    public void disconnect() {
        Log.i(TAG, "Disconnecting from mbtool");
        IOUtils.closeQuietly(mSocket);
        IOUtils.closeQuietly(mSocketIS);
        IOUtils.closeQuietly(mSocketOS);
        mSocket = null;
        mSocketIS = null;
        mSocketOS = null;

        mMbtoolVersion = null;
    }

    public String version(Context context) {
        if (!connect(context)) {
            return null;
        }

        return mMbtoolVersion;
    }

    public RomInformation[] getInstalledRoms(Context context) {
        if (!connect(context)) {
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
                        rom.setId(SocketUtils.readString(mSocketIS));
                    } else if (ready && "SYSTEM_PATH".equals(response)) {
                        rom.setSystemPath(SocketUtils.readString(mSocketIS));
                    } else if (ready && "CACHE_PATH".equals(response)) {
                        rom.setCachePath(SocketUtils.readString(mSocketIS));
                    } else if (ready && "DATA_PATH".equals(response)) {
                        rom.setDataPath(SocketUtils.readString(mSocketIS));
                    } else if (ready && "USE_RAW_PATHS".equals(response)) {
                        // Ignore
                        SocketUtils.readString(mSocketIS);
                    } else if (ready && "VERSION".equals(response)) {
                        rom.setVersion(SocketUtils.readString(mSocketIS));
                    } else if (ready && "BUILD".equals(response)) {
                        rom.setBuild(SocketUtils.readString(mSocketIS));
                    } else if (ready && "ROM_END".equals(response)) {
                        break;
                    } else {
                        Log.e(TAG, "getInstalledRoms(): Unknown response: " + response);
                    }
                }
            }

            // Command always returns OK at the end
            SocketUtils.readString(mSocketIS);

            return roms;
        } catch (IOException e) {
            e.printStackTrace();
            disconnect();
        }

        return null;
    }

    public String getCurrentRom(Context context) {
        if (!connect(context)) {
            return null;
        }

        try {
            sendCommand(V1_COMMAND_CURRENT_ROM);

            String response = SocketUtils.readString(mSocketIS);
            switch (response) {
            case RESPONSE_SUCCESS:
                return SocketUtils.readString(mSocketIS);
            case RESPONSE_FAIL:
                return null;
            default:
                throw new IOException("Invalid response: " + response);
            }
        } catch (IOException e) {
            e.printStackTrace();
            disconnect();
        }

        return null;
    }

    public boolean chooseRom(Context context, String id) {
        if (!connect(context)) {
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
                throw new IOException("Invalid response: " + response);
            }
        } catch (IOException e) {
            e.printStackTrace();
            disconnect();
        }

        return false;
    }

    public boolean setKernel(Context context, String id) {
        if (!connect(context)) {
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
                throw new IOException("Invalid response: " + response);
            }
        } catch (IOException e) {
            e.printStackTrace();
            disconnect();
        }

        return false;
    }

    public boolean restart(Context context, String arg) {
        if (!connect(context)) {
            return false;
        }

        try {
            sendCommand(V1_COMMAND_REBOOT);

            SocketUtils.writeString(mSocketOS, arg);

            String response = SocketUtils.readString(mSocketIS);
            if (response.equals(RESPONSE_FAIL)) {
                return false;
            } else {
                throw new IOException("Invalid response: " + response);
            }
        } catch (IOException e) {
            e.printStackTrace();
            disconnect();
        }

        return false;
    }

    public FileDescriptor open(Context context, String filename, String[] flags) {
        if (!connect(context)) {
            return null;
        }

        try {
            sendCommand(V1_COMMAND_OPEN);

            SocketUtils.writeString(mSocketOS, filename);
            SocketUtils.writeStringArray(mSocketOS, flags);

            String response = SocketUtils.readString(mSocketIS);
            if (response.equals(RESPONSE_FAIL)) {
                return null;
            } else if (!response.equals(RESPONSE_SUCCESS)) {
                throw new IOException("Invalid response: " + response);
            }

            // Read file descriptors
            //int dummy = mSocketIS.read();
            byte[] buf = new byte[1];
            SocketUtils.readFully(mSocketIS, buf, 0, 1);
            FileDescriptor[] fds = mSocket.getAncillaryFileDescriptors();

            if (fds == null || fds.length == 0) {
                throw new IOException("mbtool sent no file descriptor");
            }

            return fds[0];
        } catch (IOException e) {
            e.printStackTrace();
            disconnect();
        }

        return null;
    }

    public boolean copy(Context context, String source, String target) {
        if (!connect(context)) {
            return false;
        }

        try {
            sendCommand(V1_COMMAND_COPY);

            SocketUtils.writeString(mSocketOS, source);
            SocketUtils.writeString(mSocketOS, target);

            String response = SocketUtils.readString(mSocketIS);
            if (response.equals(RESPONSE_FAIL)) {
                return false;
            } else if (!response.equals(RESPONSE_SUCCESS)) {
                throw new IOException("Invalid response: " + response);
            }

            return true;
        } catch (IOException e) {
            e.printStackTrace();
            disconnect();
        }

        return false;
    }

    public boolean chmod(Context context, String filename, int mode) {
        if (!connect(context)) {
            return false;
        }

        try {
            sendCommand(V1_COMMAND_CHMOD);

            SocketUtils.writeString(mSocketOS, filename);
            SocketUtils.writeInt32(mSocketOS, mode);

            String response = SocketUtils.readString(mSocketIS);
            if (response.equals(RESPONSE_FAIL)) {
                return false;
            } else if (!response.equals(RESPONSE_SUCCESS)) {
                throw new IOException("Invalid response: " + response);
            }

            return true;
        } catch (IOException e) {
            e.printStackTrace();
            disconnect();
        }

        return false;
    }

    public boolean lokiPatch(Context context, String source, String target) {
        if (!connect(context)) {
            return false;
        }

        try {
            sendCommand(V1_COMMAND_LOKI_PATCH);

            SocketUtils.writeString(mSocketOS, source);
            SocketUtils.writeString(mSocketOS, target);

            String response = SocketUtils.readString(mSocketIS);
            if (response.equals(RESPONSE_FAIL)) {
                return false;
            } else if (!response.equals(RESPONSE_SUCCESS)) {
                throw new IOException("Invalid response: " + response);
            }

            return true;
        } catch (IOException e) {
            e.printStackTrace();
            disconnect();
        }

        return false;
    }

    // Private helper functions

    private void sendCommand(String command) throws IOException {
        SocketUtils.writeString(mSocketOS, command);

        String response = SocketUtils.readString(mSocketIS);
        if (RESPONSE_UNSUPPORTED.equals(response)) {
            throw new IOException("Unsupported command: " + command);
        } else if (!RESPONSE_OK.equals(response)) {
            throw new IOException("Unknown response: " + response);
        }
    }

    public static class OpenFlags {
        public static final String APPEND = "APPEND";
        public static final String CREAT = "CREAT";
        public static final String EXCL = "EXCL";
        public static final String RDWR = "RDWR";
        public static final String TRUNC = "TRUNC";
        public static final String WRONLY = "WRONLY";
    }
}
