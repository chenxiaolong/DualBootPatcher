/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.socket;

import android.content.Context;
import android.net.LocalSocket;
import android.net.LocalSocketAddress;
import android.net.LocalSocketAddress.Namespace;
import android.os.Build;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.ThreadUtils;
import com.github.chenxiaolong.dualbootpatcher.Version;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils.Feature;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterfaceV3;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SignedExecCompletion;

import java.io.Closeable;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import mbtool.daemon.v3.SignedExecResult;

public class MbtoolConnection implements Closeable {
    private static final String TAG = MbtoolConnection.class.getSimpleName();

    /** mbtool daemon's abstract socket address */
    private static final String SOCKET_ADDRESS = "mbtool.daemon";

    /** Protocol version to use */
    private static final int PROTOCOL_VERSION = 3;

    /** Minimum protocol version for signed exec */
    private static final int SIGNED_EXEC_MIN_PROTOCOL_VERSION = 3;
    /** Maximum protocol version for signed exec */
    private static final int SIGNED_EXEC_MAX_PROTOCOL_VERSION = 3;

    // Handshake responses

    /**
     * Handshake response indicating that the app signature has been verified and the connection
     * is allowed.
     */
    private static final String HANDSHAKE_RESPONSE_ALLOW = "ALLOW";
    /**
     * Handshake response indicating the the app signature is invalid and the negotiation has been
     * denied. The daemon will terminate the connection after sending this response.
     */
    private static final String HANDSHAKE_RESPONSE_DENY = "DENY";
    /**
     * Handshake response indicating that the requested protocol version is supported.
     */
    private static final String HANDSHAKE_RESPONSE_OK = "OK";
    /**
     * Handshake response indicating that the requested protocol version is unsupported. The daemon
     * will terminate the connection after sending this response.
     */
    private static final String HANDSHAKE_RESPONSE_UNSUPPORTED = "UNSUPPORTED";

    /** mbtool socket */
    private LocalSocket mSocket;
    /** Socket's input stream */
    private InputStream mSocketIS;
    /** Socket's output stream */
    private OutputStream mSocketOS;

    /** mbtool interface */
    private MbtoolInterface mInterface;

    /**
     * Bind to mbtool socket
     *
     * @throws MbtoolException
     */
    private static LocalSocket initConnectToSocket() throws MbtoolException {
        try {
            LocalSocket socket = new LocalSocket();
            socket.connect(new LocalSocketAddress(SOCKET_ADDRESS, Namespace.ABSTRACT));
            return socket;
        } catch (IOException e) {
            Log.e(TAG, "Could not connect to mbtool socket", e);
            throw new MbtoolException(Reason.DAEMON_NOT_RUNNING, e);
        }
    }

    /**
     * Check if mbtool rejected the connection due to the signature check failing
     *
     * @throws IOException
     * @throws MbtoolException
     */
    private static void initVerifyCredentials(InputStream is) throws IOException, MbtoolException {
        String response = SocketUtils.readString(is);
        if (HANDSHAKE_RESPONSE_DENY.equals(response)) {
            Log.e(TAG, "mbtool connection was denied. This app might have been tampered with!");
            throw new MbtoolException(Reason.SIGNATURE_CHECK_FAIL,
                    "Access was explicitly denied by the daemon");
        } else if (!HANDSHAKE_RESPONSE_ALLOW.equals(response)) {
            throw new MbtoolException(Reason.PROTOCOL_ERROR, "Unexpected reply: " + response);
        }
    }

    /**
     * Request protocol version from mbtool
     *
     * @throws IOException
     * @throws MbtoolException
     */
    private static void initRequestInterface(InputStream is, OutputStream os, int version)
            throws IOException, MbtoolException {
        SocketUtils.writeInt32(os, version);
        String response = SocketUtils.readString(is);
        if (HANDSHAKE_RESPONSE_UNSUPPORTED.equals(response)) {
            throw new MbtoolException(Reason.INTERFACE_NOT_SUPPORTED,
                    "Daemon does not support protocol version: " + version);
        } else if (!HANDSHAKE_RESPONSE_OK.equals(response)) {
            throw new MbtoolException(Reason.PROTOCOL_ERROR, "Unexpected reply: " + response);
        }
    }

    /**
     * Check that the minimum mbtool version is satisfied
     *
     * @throws IOException
     * @throws MbtoolException
     */
    private static void initVerifyVersion(MbtoolInterface iface, Version minVersion)
            throws IOException, MbtoolException {
        Version version;
        try {
            version = iface.getVersion();
        } catch (MbtoolCommandException e) {
            Log.w(TAG, "Failed to get mbtool version. Assuming to be old version", e);
            throw new MbtoolException(Reason.VERSION_TOO_OLD, "Could not get mbtool version");
        }

        Log.v(TAG, "mbtool version: " + version);
        Log.v(TAG, "minimum version: " + minVersion);

        // Ensure that the version is newer than the minimum required version
        if (version.compareTo(minVersion) < 0) {
            throw new MbtoolException(Reason.VERSION_TOO_OLD,
                    "mbtool version is: " + version + ", " + "minimum needed is: " + minVersion);
        }
    }

    @Nullable
    private static MbtoolInterface createInterface(InputStream is, OutputStream os, int version) {
        switch (version) {
        case 3:
            return new MbtoolInterfaceV3(is, os);
        default:
            return null;
        }
    }

    private void connect() throws IOException, MbtoolException {
        // Try connecting to the socket
        mSocket = initConnectToSocket();
        mSocketIS = mSocket.getInputStream();
        mSocketOS = mSocket.getOutputStream();

        // mbtool will immediately send a message telling us whether the app signature is allowed
        // or denied
        initVerifyCredentials(mSocketIS);

        // Request interface version
        initRequestInterface(mSocketIS, mSocketOS, PROTOCOL_VERSION);

        // Set up interface
        mInterface = createInterface(mSocketIS, mSocketOS, PROTOCOL_VERSION);

        // Check version
        initVerifyVersion(mInterface, MbtoolUtils.getMinimumRequiredVersion(Feature.DAEMON));
    }

    public MbtoolConnection(Context context) throws IOException, MbtoolException {
        ThreadUtils.enforceExecutionOnNonMainThread();

        try {
            connect();
            return;
        } catch (MbtoolException e) {
            if (e.getReason() != Reason.INTERFACE_NOT_SUPPORTED
                    && e.getReason() != Reason.VERSION_TOO_OLD) {
                throw e;
            }
        }

        // Try to update mbtool if needed
        replaceViaSignedExec(context);

        // Reconnect
        connect();
    }

    @Override
    public void close() throws IOException {
        if (mSocketIS != null) {
            mSocketIS.close();
            mSocketIS = null;
        }
        if (mSocketOS != null) {
            mSocketOS.close();
            mSocketOS = null;
        }
        if (mSocket != null) {
            mSocket.close();
            mSocket = null;
        }
    }

    @NonNull
    public MbtoolInterface getInterface() {
        return mInterface;
    }

    @SuppressWarnings("deprecation")
    private static String getAbi() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            return Build.SUPPORTED_ABIS[0];
        } else {
            return Build.CPU_ABI;
        }
    }

    public static boolean replaceViaRoot(Context context) {
        ThreadUtils.enforceExecutionOnNonMainThread();

        PatcherUtils.extractPatcher(context);
        String abi = getAbi();

        String mbtool = PatcherUtils.getTargetDirectory(context)
                + "/binaries/android/" + abi + "/mbtool";
        mbtool = mbtool.replace("'", "'\"'\"'");

        return CommandUtils.runRootCommand("mount -o remount,rw /") == 0
                && CommandUtils.runRootCommand("mv /mbtool /mbtool.bak || :") == 0
                && CommandUtils.runRootCommand("cp '" + mbtool + "' /mbtool") == 0
                && CommandUtils.runRootCommand("chmod 755 /mbtool") == 0
                && CommandUtils.runRootCommand("mount -o remount,ro / || :") == 0
                && CommandUtils.runRootCommand("/mbtool daemon --replace --daemonize") == 0;

        // Give mbtool a little bit of time to start listening on the socket
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        return ret;
    }

    public static boolean replaceViaSignedExec(Context context) {
        ThreadUtils.enforceExecutionOnNonMainThread();

        PatcherUtils.extractPatcher(context);

        String abi = getAbi();

        File mbtool = new File(PatcherUtils.getTargetDirectory(context) +
                File.separator + "data" +
                File.separator + "binaries" +
                File.separator + abi +
                File.separator + "mbtool");
        File mbtoolSig = new File(PatcherUtils.getTargetDirectory(context) +
                File.separator + "data" +
                File.separator + "binaries" +
                File.separator + abi +
                File.separator + "mbtool.sig");

        for (int i = SIGNED_EXEC_MAX_PROTOCOL_VERSION; i >= SIGNED_EXEC_MIN_PROTOCOL_VERSION; i--) {
            try {
                // Try connecting to the socket
                LocalSocket socket = initConnectToSocket();
                InputStream socketIS = socket.getInputStream();
                OutputStream socketOS = socket.getOutputStream();

                // mbtool will immediately send a message telling us whether the app signature is
                // allowed or denied
                initVerifyCredentials(socketIS);

                // Request interface version
                initRequestInterface(socketIS, socketOS, i);

                // Create interface
                MbtoolInterface iface = createInterface(socketIS, socketOS, i);
                if (iface == null) {
                    throw new IllegalStateException("Failed to create interface for version: " + i);
                }

                // Use signed exec to replace mbtool
                SignedExecCompletion completion = iface.signedExec(
                        mbtool.getAbsolutePath(), mbtoolSig.getAbsolutePath(),
                        "daemon", new String[] { "--replace", "--daemonize" }, null);

                switch (completion.result) {
                case SignedExecResult.PROCESS_EXITED:
                    Log.d(TAG, "mbtool signed exec exited with status: " + completion.exitStatus);
                    return completion.exitStatus == 0;
                case SignedExecResult.PROCESS_KILLED_BY_SIGNAL:
                    Log.d(TAG, "mbtool signed exec killed by signal: " + completion.termSig);
                    return false;
                case SignedExecResult.INVALID_SIGNATURE:
                    Log.d(TAG, "mbtool signed exec failed due to invalid signature");
                    return false;
                case SignedExecResult.OTHER_ERROR:
                default:
                    Log.d(TAG, "mbtool signed exec failed: " + completion.errorMsg);
                    return false;
                }
            } catch (IOException e) {
                // Keep trying
                Log.w(TAG, "mbtool connection error", e);
            } catch (MbtoolException e) {
                // Keep trying unless the daemon is not running or if the signature check fails
                Log.w(TAG, "mbtool error", e);
                if (e.getReason() == Reason.DAEMON_NOT_RUNNING
                        || e.getReason() == Reason.SIGNATURE_CHECK_FAIL) {
                    break;
                }
            } catch (MbtoolCommandException e) {
                // Keep trying
                Log.w(TAG, "mbtool command error", e);
            }
        }

        return false;
    }
}
