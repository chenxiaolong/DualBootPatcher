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
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.ThreadUtils;
import com.github.chenxiaolong.dualbootpatcher.Version;
import com.github.chenxiaolong.dualbootpatcher.Version.VersionParseException;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils.Feature;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;
import com.google.flatbuffers.FlatBufferBuilder;

import org.apache.commons.io.IOUtils;

import java.io.FileDescriptor;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;

import mbtool.daemon.v2.ChmodRequest;
import mbtool.daemon.v2.ChmodResponse;
import mbtool.daemon.v2.CopyRequest;
import mbtool.daemon.v2.CopyResponse;
import mbtool.daemon.v2.GetBuiltinRomIdsRequest;
import mbtool.daemon.v2.GetBuiltinRomIdsResponse;
import mbtool.daemon.v2.GetCurrentRomRequest;
import mbtool.daemon.v2.GetCurrentRomResponse;
import mbtool.daemon.v2.GetRomsListRequest;
import mbtool.daemon.v2.GetRomsListResponse;
import mbtool.daemon.v2.GetVersionRequest;
import mbtool.daemon.v2.OpenRequest;
import mbtool.daemon.v2.OpenResponse;
import mbtool.daemon.v2.RebootRequest;
import mbtool.daemon.v2.RebootResponse;
import mbtool.daemon.v2.Request;
import mbtool.daemon.v2.RequestType;
import mbtool.daemon.v2.Response;
import mbtool.daemon.v2.ResponseType;
import mbtool.daemon.v2.Rom;
import mbtool.daemon.v2.SetKernelRequest;
import mbtool.daemon.v2.SetKernelResponse;
import mbtool.daemon.v2.SwitchRomRequest;
import mbtool.daemon.v2.SwitchRomResponse;
import mbtool.daemon.v2.WipeRomRequest;
import mbtool.daemon.v2.WipeRomResponse;

public class MbtoolSocket {
    private static final String TAG = MbtoolSocket.class.getSimpleName();

    private static final String SOCKET_ADDRESS = "mbtool.daemon";

    // Same as the C++ default
    private static final int FBB_SIZE = 1024;

    private static final String RESPONSE_ALLOW = "ALLOW";
    private static final String RESPONSE_DENY = "DENY";
    private static final String RESPONSE_OK = "OK";
    private static final String RESPONSE_UNSUPPORTED = "UNSUPPORTED";

    private static MbtoolSocket sInstance;

    private LocalSocket mSocket;
    private InputStream mSocketIS;
    private OutputStream mSocketOS;
    private int mInterfaceVersion;
    private String mMbtoolVersion;

    // Keep this as a singleton class for now
    private MbtoolSocket() {
        mInterfaceVersion = 2;
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
        FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
        GetVersionRequest.startGetVersionRequest(builder);
        int request = GetVersionRequest.endGetVersionRequest(builder);
        Request.startRequest(builder);
        Request.addType(builder, RequestType.GET_VERSION);
        Request.addGetVersionRequest(builder, request);
        builder.finish(Request.endRequest(builder));
        Response fbresponse = sendRequest(builder, ResponseType.GET_VERSION);
        mMbtoolVersion = fbresponse.getVersionResponse().version();
        if (mMbtoolVersion == null) {
            throw new IOException("Could not determine mbtool version");
        }

        Version v1;
        Version v2 = MbtoolUtils.getMinimumRequiredVersion(Feature.DAEMON);

        try {
            v1 = new Version(mMbtoolVersion);
        } catch (VersionParseException e) {
            throw new IOException("Invalid version number: " + mMbtoolVersion);
        }

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
     */
    public void connect(Context context) throws IOException {
        // If we're already connected, then we're good
        if (mSocket != null) {
            return;
        }

        // Try connecting to the socket
        try {
            mSocket = new LocalSocket();
            mSocket.connect(new LocalSocketAddress(SOCKET_ADDRESS, Namespace.ABSTRACT));
            initializeConnection();
            return;
        } catch (IOException e) {
            Log.e(TAG, "Could not connect to mbtool socket", e);
            disconnect();
        }

        Log.v(TAG, "Launching bundled mbtool");

        if (!executeMbtool(context)) {
            throw new IOException("Failed to execute mbtool");
        }

        // Give mbtool a little bit of time to start listening on the socket
        try {
            Thread.sleep(1000);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }

        try {
            mSocket = new LocalSocket();
            mSocket.connect(new LocalSocketAddress(SOCKET_ADDRESS, Namespace.ABSTRACT));
            initializeConnection();
        } catch (IOException e) {
            disconnect();
            throw new IOException("Could not connect to mbtool socket", e);
        }
    }

    @SuppressWarnings("deprecation")
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

    /**
     * Get version of the mbtool daemon.
     *
     * @param context Application context
     * @return String containing the mbtool version
     * @throws IOException When any socket communication error occurs
     */
    @NonNull
    public String version(Context context) throws IOException {
        connect(context);

        return mMbtoolVersion;
    }

    /**
     * Get list of installed ROMs.
     *
     * NOTE: The list of ROMs will be re-queried and a new array will be returned every time this
     *       method is called. It may be a good idea to cache the results after the initial call.
     *
     * @param context Application context
     * @return Array of {@link RomInformation} objects representing the list of installed ROMs
     * @throws IOException When any socket communication error occurs
     */
    @NonNull
    public RomInformation[] getInstalledRoms(Context context) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            GetRomsListRequest.startGetRomsListRequest(builder);
            // No parameters
            int request = GetRomsListRequest.endGetRomsListRequest(builder);

            // Wrap request
            Request.startRequest(builder);
            Request.addType(builder, RequestType.GET_ROMS_LIST);
            Request.addGetRomsListRequest(builder, request);
            builder.finish(Request.endRequest(builder));

            // Send request
            Response fbresponse = sendRequest(builder, ResponseType.GET_ROMS_LIST);
            GetRomsListResponse response = fbresponse.getRomsListResponse();

            RomInformation[] roms = new RomInformation[response.romsLength()];

            for (int i = 0; i < response.romsLength(); i++) {
                RomInformation rom = roms[i] = new RomInformation();
                Rom fbrom = response.roms(i);

                rom.setId(fbrom.id());
                rom.setSystemPath(fbrom.systemPath());
                rom.setCachePath(fbrom.cachePath());
                rom.setDataPath(fbrom.dataPath());
                rom.setVersion(fbrom.version());
                rom.setBuild(fbrom.build());
            }

            return roms;
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    /**
     * Get list of supported non-data-slot ROM IDs.
     *
     * @param context Application context
     * @return List of strings containing the ROM IDs
     * @throws IOException When any socket communication error occurs
     */
    @NonNull
    public String[] getBuiltinRomIds(Context context) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            GetBuiltinRomIdsRequest.startGetBuiltinRomIdsRequest(builder);
            // No parameters
            int request = GetBuiltinRomIdsRequest.endGetBuiltinRomIdsRequest(builder);

            // Wrap request
            Request.startRequest(builder);
            Request.addType(builder, RequestType.GET_BUILTIN_ROM_IDS);
            Request.addGetBuiltinRomIdsRequest(builder, request);
            builder.finish(Request.endRequest(builder));

            // Send request
            Response fbresponse = sendRequest(builder, ResponseType.GET_BUILTIN_ROM_IDS);
            GetBuiltinRomIdsResponse response = fbresponse.getBuiltinRomIdsResponse();

            String[] romIds = new String[response.romIdsLength()];
            for (int i = 0; i < response.romIdsLength(); i++) {
                romIds[i] = response.romIds(i);
            }

            return romIds;
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    /**
     * Get the current ROM ID.
     *
     * @param context Application context
     * @return String containing the current ROM ID
     * @throws IOException When any socket communication error occurs
     */
    @NonNull
    public String getCurrentRom(Context context) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            GetCurrentRomRequest.startGetCurrentRomRequest(builder);
            // No parameters
            int request = GetCurrentRomRequest.endGetCurrentRomRequest(builder);

            // Wrap request
            Request.startRequest(builder);
            Request.addType(builder, RequestType.GET_CURRENT_ROM);
            Request.addGetCurrentRomRequest(builder, request);
            builder.finish(Request.endRequest(builder));

            // Send request
            Response fbresponse = sendRequest(builder, ResponseType.GET_CURRENT_ROM);
            GetCurrentRomResponse response = fbresponse.getCurrentRomResponse();

            return response.romId();
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    public enum SwitchRomResult {
        UNKNOWN_BOOT_PARTITION,
        SUCCEEDED,
        FAILED,
        CHECKSUM_INVALID,
        CHECKSUM_NOT_FOUND
    }

    /**
     * Switch to another ROM.
     *
     * NOTE: If {@link SwitchRomResult#FAILED} is returned, there is no way of determining the cause
     *       of failure programmatically. However, mbtool will likely print debugging information
     *       (errno, etc.) to the logcat for manual reviewing.
     *
     * @param context Application context
     * @param id ID of ROM to switch to
     * @return {@link SwitchRomResult#UNKNOWN_BOOT_PARTITION} if the boot partition could not be determined
     *         {@link SwitchRomResult#SUCCEEDED} if the ROM was successfully switched
     *         {@link SwitchRomResult#FAILED} if the ROM failed to switch
     * @throws IOException When any socket communication error occurs
     */
    @NonNull
    public SwitchRomResult chooseRom(Context context, String id,
                                     boolean forceChecksumsUpdate) throws IOException {
        connect(context);

        try {
            String bootBlockDev = SwitcherUtils.getBootPartition(context);
            if (bootBlockDev == null) {
                Log.e(TAG, "Failed to determine boot partition");
                return SwitchRomResult.UNKNOWN_BOOT_PARTITION;
            }

            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbRomId = builder.createString(id);
            int fbBootBlockDev = builder.createString(bootBlockDev);

            // Blockdev search dirs
            String[] searchDirs = SwitcherUtils.getBlockDevSearchDirs(context);
            int fbSearchDirs = 0;
            if (searchDirs != null) {
                int[] searchDirsOffsets = new int[searchDirs.length];
                for (int i = 0; i < searchDirs.length; i++) {
                    searchDirsOffsets[i] = builder.createString(searchDirs[i]);
                }

                fbSearchDirs = SwitchRomRequest.createBlockdevBaseDirsVector(builder,
                        searchDirsOffsets);
            }

            SwitchRomRequest.startSwitchRomRequest(builder);
            SwitchRomRequest.addRomId(builder, fbRomId);
            SwitchRomRequest.addBootBlockdev(builder, fbBootBlockDev);
            SwitchRomRequest.addBlockdevBaseDirs(builder, fbSearchDirs);
            SwitchRomRequest.addForceUpdateChecksums(builder, forceChecksumsUpdate);
            int request = SwitchRomRequest.endSwitchRomRequest(builder);

            // Wrap request
            Request.startRequest(builder);
            Request.addType(builder, RequestType.SWITCH_ROM);
            Request.addSwitchRomRequest(builder, request);
            builder.finish(Request.endRequest(builder));

            // Send request
            Response fbresponse = sendRequest(builder, ResponseType.SWITCH_ROM);
            SwitchRomResponse response = fbresponse.switchRomResponse();

            // Crappy naming, I know...
            SwitchRomResult result;
            switch (response.result()) {
            case mbtool.daemon.v2.SwitchRomResult.SUCCEEDED:
                result = SwitchRomResult.SUCCEEDED;
                break;
            case mbtool.daemon.v2.SwitchRomResult.FAILED:
                result = SwitchRomResult.FAILED;
                break;
            case mbtool.daemon.v2.SwitchRomResult.CHECKSUM_INVALID:
                result = SwitchRomResult.CHECKSUM_INVALID;
                break;
            case mbtool.daemon.v2.SwitchRomResult.CHECKSUM_NOT_FOUND:
                result = SwitchRomResult.CHECKSUM_NOT_FOUND;
                break;
            default:
                throw new IOException("Invalid SwitchRomResult: " + response.result());
            }

            return result;
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    public enum SetKernelResult {
        UNKNOWN_BOOT_PARTITION,
        SUCCEEDED,
        FAILED
    }

    /**
     * Set the kernel for a ROM.
     *
     * NOTE: If {@link SetKernelResult#FAILED} is returned, there is no way of determining the cause
     *       of failure programmatically. However, mbtool will likely print debugging information
     *       (errno, etc.) to the logcat for manual reviewing.
     *
     * @param context Application context
     * @param id ID of ROM to set the kernel for
     * @return {@link SetKernelResult#UNKNOWN_BOOT_PARTITION} if the boot partition could not be determined
     *         {@link SetKernelResult#SUCCEEDED} if setting the kernel was successful
     *         {@link SetKernelResult#FAILED} if setting the kernel failed
     * @throws IOException When any socket communication error occurs
     */
    @NonNull
    public SetKernelResult setKernel(Context context, String id) throws IOException {
        connect(context);

        try {
            String bootBlockDev = SwitcherUtils.getBootPartition(context);
            if (bootBlockDev == null) {
                Log.e(TAG, "Failed to determine boot partition");
                return SetKernelResult.UNKNOWN_BOOT_PARTITION;
            }

            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbRomId = builder.createString(id);
            int fbBootBlockDev = builder.createString(bootBlockDev);
            SetKernelRequest.startSetKernelRequest(builder);
            SetKernelRequest.addRomId(builder, fbRomId);
            SetKernelRequest.addBootBlockdev(builder, fbBootBlockDev);
            int request = SetKernelRequest.endSetKernelRequest(builder);

            // Wrap request
            Request.startRequest(builder);
            Request.addType(builder, RequestType.SET_KERNEL);
            Request.addSetKernelRequest(builder, request);
            builder.finish(Request.endRequest(builder));

            // Send request
            Response fbresponse = sendRequest(builder, ResponseType.SET_KERNEL);
            SetKernelResponse response = fbresponse.setKernelResponse();

            return response.success() ? SetKernelResult.SUCCEEDED : SetKernelResult.FAILED;
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    /**
     * Reboots the device.
     *
     * @param context Application context
     * @param arg Reboot argument (eg. "recovery", "download", "bootloader"). Pass "" for a regular
     *            reboot.
     * @return True if the call to init succeeded and a reboot is pending. False, otherwise.
     * @throws IOException When any socket communication error occurs
     */
    public boolean restart(Context context, String arg) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbArg = builder.createString(arg != null ? arg : "");
            RebootRequest.startRebootRequest(builder);
            RebootRequest.addArg(builder, fbArg);
            int request = RebootRequest.endRebootRequest(builder);

            // Wrap request
            Request.startRequest(builder);
            Request.addType(builder, RequestType.REBOOT);
            Request.addRebootRequest(builder, request);
            builder.finish(Request.endRequest(builder));

            // Send request
            Response fbresponse = sendRequest(builder, ResponseType.REBOOT);
            RebootResponse response = fbresponse.rebootResponse();

            return response.success();
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    /**
     * Opens a file with mbtool and pass the file descriptor to the app.
     *
     * @param context Application context
     * @param filename Absolute path to the file
     * @param flags List of {@link mbtool.daemon.v2.OpenFlag}s
     * @return {@link FileDescriptor} if file was successfully opened. null, otherwise.
     * @throws IOException When any socket communication error occurs
     */
    @Nullable
    public FileDescriptor open(Context context, String filename, short[] flags) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbFilename = builder.createString(filename);
            int fbFlags = OpenRequest.createFlagsVector(builder, flags);
            OpenRequest.startOpenRequest(builder);
            OpenRequest.addPath(builder, fbFilename);
            OpenRequest.addFlags(builder, fbFlags);
            int request = OpenRequest.endOpenRequest(builder);

            // Wrap request
            Request.startRequest(builder);
            Request.addType(builder, RequestType.OPEN);
            Request.addOpenRequest(builder, request);
            builder.finish(Request.endRequest(builder));

            // Send request
            Response fbresponse = sendRequest(builder, ResponseType.OPEN);
            OpenResponse response = fbresponse.openResponse();

            if (!response.success()) {
                Log.e(TAG, "Failed to open file: " + response.errorMsg());
                return null;
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
            disconnect();
            throw e;
        }
    }

    /**
     * Copy a file using mbtool.
     *
     * NOTE: If false is returned, there is no way of determining the cause of failure
     *       programmatically. However, mbtool will likely print debugging information (errno, etc.)
     *       to the logcat for manual reviewing.
     *
     * @param context Application context
     * @param source Absolute source path
     * @param target Absolute target path
     * @return True if the operation was successful. False, otherwise.
     * @throws IOException When any socket communication error occurs
     */
    public boolean copy(Context context, String source, String target) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbSource = builder.createString(source);
            int fbTarget = builder.createString(target);
            CopyRequest.startCopyRequest(builder);
            CopyRequest.addSource(builder, fbSource);
            CopyRequest.addTarget(builder, fbTarget);
            int request = CopyRequest.endCopyRequest(builder);

            // Wrap request
            Request.startRequest(builder);
            Request.addType(builder, RequestType.COPY);
            Request.addCopyRequest(builder, request);
            builder.finish(Request.endRequest(builder));

            // Send request
            Response fbresponse = sendRequest(builder, ResponseType.COPY);
            CopyResponse response = fbresponse.copyResponse();

            if (!response.success()) {
                Log.e(TAG, "Failed to copy from " + source + " to " + target + ": " +
                        response.errorMsg());
                return false;
            }

            return true;
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    /**
     * Chmod a file using mbtool.
     *
     * NOTE: If false is returned, there is no way of determining the cause of failure
     *       programmatically. However, mbtool will likely print debugging information (errno, etc.)
     *       to the logcat for manual reviewing.
     *
     * @param context Application context
     * @param filename Absolute path
     * @param mode Unix permissions number (will be AND'ed with 0777 by mbtool for security reasons)
     * @return True if the operation was successful. False, otherwise.
     * @throws IOException When any socket communication error occurs
     */
    public boolean chmod(Context context, String filename, int mode) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbFilename = builder.createString(filename);
            ChmodRequest.startChmodRequest(builder);
            ChmodRequest.addPath(builder, fbFilename);
            ChmodRequest.addMode(builder, mode);
            int request = ChmodRequest.endChmodRequest(builder);

            // Wrap request
            Request.startRequest(builder);
            Request.addType(builder, RequestType.CHMOD);
            Request.addChmodRequest(builder, request);
            builder.finish(Request.endRequest(builder));

            // Send request
            Response fbresponse = sendRequest(builder, ResponseType.CHMOD);
            ChmodResponse response = fbresponse.chmodResponse();

            if (!response.success()) {
                Log.e(TAG, "Failed to chmod " + filename + ": " + response.errorMsg());
                return false;
            }

            return true;
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    public static class WipeResult {
        // Targets as listed in WipeTarget
        public short[] succeeded;
        public short[] failed;
    }

    /**
     * Wipe a ROM.
     *
     * @param context Application context
     * @param romId ROM ID to wipe
     * @param targets List of {@link mbtool.daemon.v2.WipeTarget}s indicating the wipe targets
     * @return {@link WipeResult} containing the list of succeeded and failed wipe targets
     * @throws IOException When any socket communication error occurs
     */
    @NonNull
    public WipeResult wipeRom(Context context, String romId, short[] targets) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbRomId = builder.createString(romId);
            int fbTargets = WipeRomRequest.createTargetsVector(builder, targets);
            WipeRomRequest.startWipeRomRequest(builder);
            WipeRomRequest.addRomId(builder, fbRomId);
            WipeRomRequest.addTargets(builder, fbTargets);
            int request = WipeRomRequest.endWipeRomRequest(builder);

            // Wrap request
            Request.startRequest(builder);
            Request.addType(builder, RequestType.WIPE_ROM);
            Request.addWipeRomRequest(builder, request);
            builder.finish(Request.endRequest(builder));

            // Send request
            Response fbresponse = sendRequest(builder, ResponseType.WIPE_ROM);
            WipeRomResponse response = fbresponse.wipeRomResponse();

            WipeResult result = new WipeResult();
            result.succeeded = new short[response.succeededLength()];
            result.failed = new short[response.failedLength()];

            for (int i = 0; i < response.succeededLength(); i++) {
                result.succeeded[i] = response.succeeded(i);
            }
            for (int i = 0; i < response.failedLength(); i++) {
                result.failed[i] = response.failed(i);
            }

            return result;
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    // Private helper functions

    private Response sendRequest(FlatBufferBuilder builder, short expected) throws IOException {
        ThreadUtils.enforceExecutionOnNonMainThread();

        SocketUtils.writeBytes(mSocketOS, builder.sizedByteArray());

        byte[] responseBytes = SocketUtils.readBytes(mSocketIS);
        ByteBuffer bb = ByteBuffer.wrap(responseBytes);
        Response response = Response.getRootAsResponse(bb);

        if (response.type() == ResponseType.UNSUPPORTED) {
            throw new IOException("Unsupported command");
        } else if (response.type() == ResponseType.INVALID) {
            throw new IOException("Invalid command request");
        } else if (response.type() != expected) {
            throw new IOException("Unexpected response type");
        }

        // We're not expecting any null responses in the methods above, so just handle them here
        if (response.type() == ResponseType.GET_VERSION
                && response.getVersionResponse() == null) {
            throw new IOException("null GET_VERSION response");
        } else if (response.type() == ResponseType.GET_ROMS_LIST
                && response.getRomsListResponse() == null) {
            throw new IOException("null GET_ROMS_LIST response");
        } else if (response.type() == ResponseType.GET_BUILTIN_ROM_IDS
                && response.getBuiltinRomIdsResponse() == null) {
            throw new IOException("null GET_BUILTIN_ROM_IDS response");
        } else if (response.type() == ResponseType.GET_CURRENT_ROM
                && response.getCurrentRomResponse() == null) {
            throw new IOException("null GET_CURRENT_ROM response");
        } else if (response.type() == ResponseType.SWITCH_ROM
                && response.switchRomResponse() == null) {
            throw new IOException("null SWITCH_ROM response");
        } else if (response.type() == ResponseType.SET_KERNEL
                && response.setKernelResponse() == null) {
            throw new IOException("null SET_KERNEL response");
        } else if (response.type() == ResponseType.REBOOT
                && response.rebootResponse() == null) {
            throw new IOException("null REBOOT response");
        } else if (response.type() == ResponseType.OPEN
                && response.openResponse() == null) {
            throw new IOException("null OPEN response");
        } else if (response.type() == ResponseType.COPY
                && response.copyResponse() == null) {
            throw new IOException("null COPY response");
        } else if (response.type() == ResponseType.CHMOD
                && response.chmodResponse() == null) {
            throw new IOException("null CHMOD response");
        } else if (response.type() == ResponseType.WIPE_ROM
                && response.wipeRomResponse() == null) {
            throw new IOException("null WIPE_ROM response");
        }

        return response;
    }
}
