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
import com.google.flatbuffers.Table;

import org.apache.commons.io.IOUtils;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;

import mbtool.daemon.v3.FileChmodRequest;
import mbtool.daemon.v3.FileChmodResponse;
import mbtool.daemon.v3.FileCloseRequest;
import mbtool.daemon.v3.FileCloseResponse;
import mbtool.daemon.v3.FileOpenRequest;
import mbtool.daemon.v3.FileOpenResponse;
import mbtool.daemon.v3.FileReadRequest;
import mbtool.daemon.v3.FileReadResponse;
import mbtool.daemon.v3.FileSELinuxGetLabelRequest;
import mbtool.daemon.v3.FileSELinuxGetLabelResponse;
import mbtool.daemon.v3.FileSELinuxSetLabelRequest;
import mbtool.daemon.v3.FileSELinuxSetLabelResponse;
import mbtool.daemon.v3.FileSeekRequest;
import mbtool.daemon.v3.FileSeekResponse;
import mbtool.daemon.v3.FileStatRequest;
import mbtool.daemon.v3.FileStatResponse;
import mbtool.daemon.v3.FileWriteRequest;
import mbtool.daemon.v3.FileWriteResponse;
import mbtool.daemon.v3.MbGetBootedRomIdRequest;
import mbtool.daemon.v3.MbGetBootedRomIdResponse;
import mbtool.daemon.v3.MbGetInstalledRomsRequest;
import mbtool.daemon.v3.MbGetInstalledRomsResponse;
import mbtool.daemon.v3.MbGetPackagesCountRequest;
import mbtool.daemon.v3.MbGetPackagesCountResponse;
import mbtool.daemon.v3.MbGetVersionRequest;
import mbtool.daemon.v3.MbGetVersionResponse;
import mbtool.daemon.v3.MbRom;
import mbtool.daemon.v3.MbSetKernelRequest;
import mbtool.daemon.v3.MbSetKernelResponse;
import mbtool.daemon.v3.MbSwitchRomRequest;
import mbtool.daemon.v3.MbSwitchRomResponse;
import mbtool.daemon.v3.MbSwitchRomResult;
import mbtool.daemon.v3.MbWipeRomRequest;
import mbtool.daemon.v3.MbWipeRomResponse;
import mbtool.daemon.v3.PathChmodRequest;
import mbtool.daemon.v3.PathChmodResponse;
import mbtool.daemon.v3.PathCopyRequest;
import mbtool.daemon.v3.PathCopyResponse;
import mbtool.daemon.v3.PathGetDirectorySizeRequest;
import mbtool.daemon.v3.PathGetDirectorySizeResponse;
import mbtool.daemon.v3.PathSELinuxGetLabelRequest;
import mbtool.daemon.v3.PathSELinuxGetLabelResponse;
import mbtool.daemon.v3.PathSELinuxSetLabelRequest;
import mbtool.daemon.v3.PathSELinuxSetLabelResponse;
import mbtool.daemon.v3.RebootRequest;
import mbtool.daemon.v3.RebootResponse;
import mbtool.daemon.v3.RebootType;
import mbtool.daemon.v3.Request;
import mbtool.daemon.v3.RequestType;
import mbtool.daemon.v3.Response;
import mbtool.daemon.v3.ResponseType;
import mbtool.daemon.v3.SignedExecOutputResponse;
import mbtool.daemon.v3.SignedExecRequest;
import mbtool.daemon.v3.SignedExecResponse;
import mbtool.daemon.v3.SignedExecResult;
import mbtool.daemon.v3.StructStat;

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
        mInterfaceVersion = 3;
    }

    public synchronized static MbtoolSocket getInstance() {
        if (sInstance == null) {
            sInstance = new MbtoolSocket();
        }
        return sInstance;
    }

    /**
     * Check if mbtool rejected the connection due to the signature check failing
     *
     * @throws IOException Signature check failed or unexpected response
     */
    private synchronized void verifyCredentials() throws IOException {
        String response = SocketUtils.readString(mSocketIS);
        if (RESPONSE_DENY.equals(response)) {
            throw new IOException("mbtool explicitly denied access to the daemon. " +
                    "WARNING: This app is probably not officially signed!");
        } else if (!RESPONSE_ALLOW.equals(response)) {
            throw new IOException("Unexpected reply: " + response);
        }
    }

    /**
     * Request protocol version from mbtool
     *
     * @throws IOException Protocol version not supported or unexpected reply
     */
    private synchronized void requestInterfaceVersion() throws IOException {
        SocketUtils.writeInt32(mSocketOS, mInterfaceVersion);
        String response = SocketUtils.readString(mSocketIS);
        if (RESPONSE_UNSUPPORTED.equals(response)) {
            throw new IOException("Daemon does not support interface " + mInterfaceVersion);
        } else if (!RESPONSE_OK.equals(response)) {
            throw new IOException("Unexpected reply: " + response);
        }
    }

    /**
     * Check that the minimum mbtool version is satisfied
     *
     * @throws IOException Could not determine mbtool version, invalid mbtool version, or mbtool
     *                     version is too old
     */
    private synchronized void verifyMbtoolVersion() throws IOException {
        // Get mbtool version
        FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
        MbGetVersionRequest.startMbGetVersionRequest(builder);
        int fbRequest = MbGetVersionRequest.endMbGetVersionRequest(builder);
        MbGetVersionResponse response = (MbGetVersionResponse)
                sendRequest(builder, fbRequest, RequestType.MbGetVersionRequest,
                        ResponseType.MbGetVersionResponse);
        mMbtoolVersion = response.version();
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
     * Initializes the mbtool connection.
     *
     * 1. Setup input and output streams
     * 2. Check to make sure mbtool authorized our connection
     * 3. Request interface version and check if the daemon supports it
     * 4. Get mbtool version from daemon
     *
     * @throws IOException
     */
    private synchronized void initializeConnection() throws IOException {
        mSocketIS = mSocket.getInputStream();
        mSocketOS = mSocket.getOutputStream();

        verifyCredentials();
        requestInterfaceVersion();
        verifyMbtoolVersion();
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
    public synchronized void connect(Context context) throws IOException {
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
    private synchronized boolean executeMbtool(Context context) {
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
                && CommandUtils.runRootCommand("mount -o remount,ro / || :") == 0
                && CommandUtils.runRootCommand("/mbtool daemon --replace --daemonize") == 0;
    }

    public synchronized void disconnect() {
        Log.i(TAG, "Disconnecting from mbtool");
        IOUtils.closeQuietly(mSocket);
        IOUtils.closeQuietly(mSocketIS);
        IOUtils.closeQuietly(mSocketOS);
        mSocket = null;
        mSocketIS = null;
        mSocketOS = null;

        mMbtoolVersion = null;
    }

    // RPC calls

    public synchronized boolean fileChmod(Context context, int id, int mode) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            FileChmodRequest.startFileChmodRequest(builder);
            FileChmodRequest.addId(builder, id);
            FileChmodRequest.addMode(builder, mode);
            int fbRequest = FileChmodRequest.endFileChmodRequest(builder);

            // Send request
            FileChmodResponse response = (FileChmodResponse)
                    sendRequest(builder, fbRequest, RequestType.FileChmodRequest,
                            ResponseType.FileChmodResponse);

            if (!response.success()) {
                Log.e(TAG, "[" + id + "]: chmod failed: " + response.errorMsg());
                return false;
            }
            return true;
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    public synchronized boolean fileClose(Context context, int id) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            FileCloseRequest.startFileCloseRequest(builder);
            FileCloseRequest.addId(builder, id);
            int fbRequest = FileCloseRequest.endFileCloseRequest(builder);

            // Send request
            FileCloseResponse response = (FileCloseResponse)
                    sendRequest(builder, fbRequest, RequestType.FileCloseRequest,
                            ResponseType.FileCloseResponse);

            if (!response.success()) {
                Log.e(TAG, "[" + id + "]: close failed: " + response.errorMsg());
                return false;
            }
            return true;
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    public synchronized int fileOpen(Context context, String path, short[] flags,
                                     int perms) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);

            int fbPath = builder.createString(path);
            int fbFlags = FileOpenRequest.createFlagsVector(builder, flags);

            FileOpenRequest.startFileOpenRequest(builder);
            FileOpenRequest.addPath(builder, fbPath);
            FileOpenRequest.addFlags(builder, fbFlags);
            FileOpenRequest.addPerms(builder, perms);
            int fbRequest = FileOpenRequest.endFileOpenRequest(builder);

            // Send request
            FileOpenResponse response = (FileOpenResponse)
                    sendRequest(builder, fbRequest, RequestType.FileOpenRequest,
                            ResponseType.FileOpenResponse);

            if (!response.success()) {
                Log.e(TAG, "[" + path + "]: open failed: " + response.errorMsg());
                return -1;
            }
            return response.id();
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    @Nullable
    public synchronized ByteBuffer fileRead(Context context, int id, long size) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            FileReadRequest.startFileReadRequest(builder);
            FileReadRequest.addId(builder, id);
            FileReadRequest.addCount(builder, size);
            int fbRequest = FileReadRequest.endFileReadRequest(builder);

            // Send request
            FileReadResponse response = (FileReadResponse)
                    sendRequest(builder, fbRequest, RequestType.FileReadRequest,
                            ResponseType.FileReadResponse);

            if (!response.success()) {
                Log.e(TAG, "[" + id + "]: read failed: " + response.errorMsg());
                return null;
            }
            return response.dataAsByteBuffer();
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    public synchronized long fileSeek(Context context, int id, long offset,
                                      short whence) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            FileSeekRequest.startFileSeekRequest(builder);
            FileSeekRequest.addId(builder, id);
            FileSeekRequest.addOffset(builder, offset);
            FileSeekRequest.addWhence(builder, whence);
            int fbRequest = FileSeekRequest.endFileSeekRequest(builder);

            // Send request
            FileSeekResponse response = (FileSeekResponse)
                    sendRequest(builder, fbRequest, RequestType.FileSeekRequest,
                            ResponseType.FileSeekResponse);

            if (!response.success()) {
                Log.e(TAG, "[" + id + "]: seek failed: " + response.errorMsg());
                return -1;
            }
            return response.offset();
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    public static class StatBuf {
        public long st_dev;
        public long st_ino;
        public int st_mode;
        public long st_nlink;
        public int st_uid;
        public int st_gid;
        public long st_rdev;
        public long st_size;
        public long st_blksize;
        public long st_blocks;
        public long st_atime;
        public long st_mtime;
        public long st_ctime;
    }

    @Nullable
    public synchronized StatBuf fileStat(Context context, int id) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            FileStatRequest.startFileStatRequest(builder);
            FileStatRequest.addId(builder, id);
            int fbRequest = FileStatRequest.endFileStatRequest(builder);

            // Send request
            FileStatResponse response = (FileStatResponse)
                    sendRequest(builder, fbRequest, RequestType.FileStatRequest,
                            ResponseType.FileStatResponse);

            if (!response.success()) {
                Log.e(TAG, "[" + id + "]: stat failed: " + response.errorMsg());
                return null;
            }

            StructStat ss = response.stat();
            StatBuf sb = new StatBuf();
            sb.st_dev = ss.stDev();
            sb.st_ino = ss.stIno();
            sb.st_mode = (int) ss.stMode();
            sb.st_nlink = ss.stNlink();
            sb.st_uid = (int) ss.stUid();
            sb.st_gid = (int) ss.stGid();
            sb.st_rdev = ss.stRdev();
            sb.st_size = ss.stSize();
            sb.st_blksize = ss.stBlksize();
            sb.st_blocks = ss.stBlocks();
            sb.st_atime = ss.stAtime();
            sb.st_mtime = ss.stMtime();
            sb.st_ctime = ss.stCtime();
            return sb;
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    public synchronized long fileWrite(Context context, int id, byte[] data) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbData = FileWriteRequest.createDataVector(builder, data);
            FileWriteRequest.startFileWriteRequest(builder);
            FileWriteRequest.addId(builder, id);
            FileWriteRequest.addData(builder, fbData);
            int fbRequest = FileWriteRequest.endFileWriteRequest(builder);

            // Send request
            FileWriteResponse response = (FileWriteResponse)
                    sendRequest(builder, fbRequest, RequestType.FileWriteRequest,
                            ResponseType.FileWriteResponse);

            if (!response.success()) {
                Log.e(TAG, "[" + id + "]: write failed: " + response.errorMsg());
                return -1;
            }
            return response.bytesWritten();
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    @Nullable
    public synchronized String fileSelinuxGetLabel(Context context, int id) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            FileSELinuxGetLabelRequest.startFileSELinuxGetLabelRequest(builder);
            FileSELinuxGetLabelRequest.addId(builder, id);
            int fbRequest = FileSELinuxGetLabelRequest.endFileSELinuxGetLabelRequest(builder);

            // Send request
            FileSELinuxGetLabelResponse response = (FileSELinuxGetLabelResponse)
                    sendRequest(builder, fbRequest, RequestType.FileSELinuxGetLabelRequest,
                            ResponseType.FileSELinuxGetLabelResponse);

            if (!response.success()) {
                Log.e(TAG, "[" + id + "]: SELinux get label failed: " + response.errorMsg());
                return null;
            }
            return response.label();
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    public synchronized boolean fileSelinuxSetLabel(Context context, int id,
                                                    String label) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            FileSELinuxSetLabelRequest.startFileSELinuxSetLabelRequest(builder);
            FileSELinuxSetLabelRequest.addId(builder, id);
            int fbRequest = FileSELinuxSetLabelRequest.endFileSELinuxSetLabelRequest(builder);

            // Send request
            FileSELinuxSetLabelResponse response = (FileSELinuxSetLabelResponse)
                    sendRequest(builder, fbRequest, RequestType.FileSELinuxSetLabelRequest,
                            ResponseType.FileSELinuxSetLabelResponse);

            if (!response.success()) {
                Log.e(TAG, "[" + id + "]: SELinux set label failed: " + response.errorMsg());
                return false;
            }
            return true;
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    /**
     * Get version of the mbtool daemon.
     *
     * @param context Application context
     * @return String containing the mbtool version
     * @throws IOException When any socket communication error occurs
     */
    @NonNull
    public synchronized String version(Context context) throws IOException {
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
    public synchronized RomInformation[] getInstalledRoms(Context context) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            MbGetInstalledRomsRequest.startMbGetInstalledRomsRequest(builder);
            // No parameters
            int fbRequest = MbGetInstalledRomsRequest.endMbGetInstalledRomsRequest(builder);

            // Send request
            MbGetInstalledRomsResponse response = (MbGetInstalledRomsResponse)
                    sendRequest(builder, fbRequest, RequestType.MbGetInstalledRomsRequest,
                            ResponseType.MbGetInstalledRomsResponse);

            RomInformation[] roms = new RomInformation[response.romsLength()];

            for (int i = 0; i < response.romsLength(); i++) {
                RomInformation rom = roms[i] = new RomInformation();
                MbRom fbrom = response.roms(i);

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
     * Get the current ROM ID.
     *
     * @param context Application context
     * @return String containing the current ROM ID
     * @throws IOException When any socket communication error occurs
     */
    @NonNull
    public synchronized String getBootedRomId(Context context) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            MbGetBootedRomIdRequest.startMbGetBootedRomIdRequest(builder);
            // No parameters
            int fbRequest = MbGetBootedRomIdRequest.endMbGetBootedRomIdRequest(builder);

            // Send request
            MbGetBootedRomIdResponse response = (MbGetBootedRomIdResponse)
                    sendRequest(builder, fbRequest, RequestType.MbGetBootedRomIdRequest,
                            ResponseType.MbGetBootedRomIdResponse);

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
    public synchronized SwitchRomResult switchRom(Context context, String id,
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

                fbSearchDirs = MbSwitchRomRequest.createBlockdevBaseDirsVector(
                        builder, searchDirsOffsets);
            }

            MbSwitchRomRequest.startMbSwitchRomRequest(builder);
            MbSwitchRomRequest.addRomId(builder, fbRomId);
            MbSwitchRomRequest.addBootBlockdev(builder, fbBootBlockDev);
            MbSwitchRomRequest.addBlockdevBaseDirs(builder, fbSearchDirs);
            MbSwitchRomRequest.addForceUpdateChecksums(builder, forceChecksumsUpdate);
            int fbRequest = MbSwitchRomRequest.endMbSwitchRomRequest(builder);

            // Send request
            MbSwitchRomResponse response = (MbSwitchRomResponse)
                    sendRequest(builder, fbRequest, RequestType.MbSwitchRomRequest,
                            ResponseType.MbSwitchRomResponse);

            SwitchRomResult result;
            switch (response.result()) {
            case MbSwitchRomResult.SUCCEEDED:
                result = SwitchRomResult.SUCCEEDED;
                break;
            case MbSwitchRomResult.FAILED:
                result = SwitchRomResult.FAILED;
                break;
            case MbSwitchRomResult.CHECKSUM_INVALID:
                result = SwitchRomResult.CHECKSUM_INVALID;
                break;
            case MbSwitchRomResult.CHECKSUM_NOT_FOUND:
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
    public synchronized SetKernelResult setKernel(Context context, String id) throws IOException {
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
            MbSetKernelRequest.startMbSetKernelRequest(builder);
            MbSetKernelRequest.addRomId(builder, fbRomId);
            MbSetKernelRequest.addBootBlockdev(builder, fbBootBlockDev);
            int fbRequest = MbSetKernelRequest.endMbSetKernelRequest(builder);

            // Send request
            MbSetKernelResponse response = (MbSetKernelResponse)
                    sendRequest(builder, fbRequest, RequestType.MbSetKernelRequest,
                            ResponseType.MbSetKernelResponse);

            return response.success() ? SetKernelResult.SUCCEEDED : SetKernelResult.FAILED;
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    /**
     * Reboots the device via the framework.
     *
     * mbtool will launch an intent to start Android's ShutdownActivity
     *
     * @param context Application context
     * @param confirm Whether Android's reboot dialog should be shown
     * @return True if Android's ShutdownActivity was successfully launched. False, otherwise.
     * @throws IOException When any socket communication error occurs
     */
    public synchronized boolean restartViaFramework(Context context, boolean confirm) throws
            IOException{
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            RebootRequest.startRebootRequest(builder);
            RebootRequest.addType(builder, RebootType.FRAMEWORK);
            RebootRequest.addConfirm(builder, confirm);
            int fbRequest = RebootRequest.endRebootRequest(builder);

            // Send request
            RebootResponse response = (RebootResponse)
                    sendRequest(builder, fbRequest, RequestType.RebootRequest,
                            ResponseType.RebootResponse);

            return response.success();
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    /**
     * Reboots the device via init.
     *
     * NOTE: May result in an unclean shutdown as Android's init will simply kill all processes,
     * attempt to unmount filesystems, and then reboot.
     *
     * @param context Application context
     * @param arg Reboot argument (eg. "recovery", "download", "bootloader"). Pass "" for a regular
     *            reboot.
     * @return True if the call to init succeeded and a reboot is pending. False, otherwise.
     * @throws IOException When any socket communication error occurs
     */
    public synchronized boolean restartViaInit(Context context, String arg) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbArg = builder.createString(arg != null ? arg : "");
            RebootRequest.startRebootRequest(builder);
            RebootRequest.addType(builder, RebootType.INIT);
            RebootRequest.addArg(builder, fbArg);
            int fbRequest = RebootRequest.endRebootRequest(builder);

            // Send request
            RebootResponse response = (RebootResponse)
                    sendRequest(builder, fbRequest, RequestType.RebootRequest,
                            ResponseType.RebootResponse);

            return response.success();
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    /**
     * Reboots the device via mbtool.
     *
     * NOTE: May result in an unclean shutdown as mbtool will simply kill all processes, attempt to
     * unmount filesystems, and then reboot.
     *
     * @param context Application context
     * @param arg Reboot argument (eg. "recovery", "download", "bootloader"). Pass "" for a regular
     *            reboot.
     * @return False if reboot fails. Otherwise, does not return.
     * @throws IOException When any socket communication error occurs
     */
    public synchronized boolean restartViaMbtool(Context context, String arg) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbArg = builder.createString(arg != null ? arg : "");
            RebootRequest.startRebootRequest(builder);
            RebootRequest.addType(builder, RebootType.DIRECT);
            RebootRequest.addArg(builder, fbArg);
            int fbRequest = RebootRequest.endRebootRequest(builder);

            // Send request
            RebootResponse response = (RebootResponse)
                    sendRequest(builder, fbRequest, RequestType.RebootRequest,
                            ResponseType.RebootResponse);

            return response.success();
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
    public synchronized boolean pathCopy(Context context, String source,
                                         String target) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbSource = builder.createString(source);
            int fbTarget = builder.createString(target);
            PathCopyRequest.startPathCopyRequest(builder);
            PathCopyRequest.addSource(builder, fbSource);
            PathCopyRequest.addTarget(builder, fbTarget);
            int fbRequest = PathCopyRequest.endPathCopyRequest(builder);

            // Send request
            PathCopyResponse response = (PathCopyResponse)
                    sendRequest(builder, fbRequest, RequestType.PathCopyRequest,
                            ResponseType.PathCopyResponse);

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
    public synchronized boolean pathChmod(Context context, String filename,
                                          int mode) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbFilename = builder.createString(filename);
            PathChmodRequest.startPathChmodRequest(builder);
            PathChmodRequest.addPath(builder, fbFilename);
            PathChmodRequest.addMode(builder, mode);
            int fbRequest = PathChmodRequest.endPathChmodRequest(builder);

            // Send request
            PathChmodResponse response = (PathChmodResponse)
                    sendRequest(builder, fbRequest, RequestType.PathChmodRequest,
                            ResponseType.PathChmodResponse);

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
     * @param targets List of {@link mbtool.daemon.v3.MbWipeTarget}s indicating the wipe targets
     * @return {@link WipeResult} containing the list of succeeded and failed wipe targets
     * @throws IOException When any socket communication error occurs
     */
    @NonNull
    public synchronized WipeResult wipeRom(Context context, String romId,
                                           short[] targets) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbRomId = builder.createString(romId);
            int fbTargets = MbWipeRomRequest.createTargetsVector(builder, targets);
            MbWipeRomRequest.startMbWipeRomRequest(builder);
            MbWipeRomRequest.addRomId(builder, fbRomId);
            MbWipeRomRequest.addTargets(builder, fbTargets);
            int fbRequest = MbWipeRomRequest.endMbWipeRomRequest(builder);

            // Send request
            MbWipeRomResponse response = (MbWipeRomResponse)
                    sendRequest(builder, fbRequest, RequestType.MbWipeRomRequest,
                            ResponseType.MbWipeRomResponse);

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

    public static class PackageCounts {
        public int systemPackages;
        public int systemUpdatePackages;
        public int nonSystemPackages;
    }

    @Nullable
    public synchronized PackageCounts getPackagesCounts(Context context,
                                                        String romId) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbRomId = builder.createString(romId);
            MbGetPackagesCountRequest.startMbGetPackagesCountRequest(builder);
            MbGetPackagesCountRequest.addRomId(builder, fbRomId);
            int fbRequest = MbGetPackagesCountRequest.endMbGetPackagesCountRequest(builder);

            // Send request
            MbGetPackagesCountResponse response = (MbGetPackagesCountResponse)
                    sendRequest(builder, fbRequest, RequestType.MbGetPackagesCountRequest,
                            ResponseType.MbGetPackagesCountResponse);

            if (!response.success()) {
                return null;
            }

            PackageCounts pc = new PackageCounts();
            pc.systemPackages = (int) response.systemPackages();
            pc.systemUpdatePackages = (int) response.systemUpdatePackages();
            pc.nonSystemPackages = (int) response.nonSystemPackages();
            return pc;
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    /**
     * Get the SELinux label of a path.
     *
     * NOTE: If false is returned, there is no way of determining the cause of failure
     *       programmatically. However, mbtool will likely print debugging information (errno, etc.)
     *       to the logcat for manual reviewing.
     *
     * @param context Application context
     * @param path Absolute path
     * @param followSymlinks Whether to follow symlinks
     * @return SELinux label if it was successfully retrieved. False, otherwise.
     * @throws IOException When any socket communication error occurs
     */
    public synchronized String pathSelinuxGetLabel(Context context, String path,
                                                   boolean followSymlinks) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbPath = builder.createString(path);
            PathSELinuxGetLabelRequest.startPathSELinuxGetLabelRequest(builder);
            PathSELinuxGetLabelRequest.addPath(builder, fbPath);
            PathSELinuxGetLabelRequest.addFollowSymlinks(builder, followSymlinks);
            int fbRequest = PathSELinuxGetLabelRequest.endPathSELinuxGetLabelRequest(builder);

            // Send request
            PathSELinuxGetLabelResponse response = (PathSELinuxGetLabelResponse)
                    sendRequest(builder, fbRequest, RequestType.PathSELinuxGetLabelRequest,
                            ResponseType.PathSELinuxGetLabelResponse);

            if (!response.success()) {
                Log.e(TAG, "Failed to get SELinux label for " + path + ": " + response.errorMsg());
                return null;
            }

            return response.label();
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    /**
     * Set the SELinux label for a path.
     *
     * NOTE: If false is returned, there is no way of determining the cause of failure
     *       programmatically. However, mbtool will likely print debugging information (errno, etc.)
     *       to the logcat for manual reviewing.
     *
     * @param context Application context
     * @param path Absolute path
     * @param label SELinux label
     * @param followSymlinks Whether to follow symlinks
     * @return True if the SELinux label was successfully set. False, otherwise.
     * @throws IOException When any socket communication error occurs
     */
    public synchronized boolean pathSelinuxSetLabel(Context context, String path, String label,
                                                    boolean followSymlinks) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbPath = builder.createString(path);
            int fbLabel = builder.createString(label);
            PathSELinuxSetLabelRequest.startPathSELinuxSetLabelRequest(builder);
            PathSELinuxSetLabelRequest.addPath(builder, fbPath);
            PathSELinuxSetLabelRequest.addLabel(builder, fbLabel);
            PathSELinuxSetLabelRequest.addFollowSymlinks(builder, followSymlinks);
            int fbRequest = PathSELinuxSetLabelRequest.endPathSELinuxSetLabelRequest(builder);

            // Send request
            PathSELinuxSetLabelResponse response = (PathSELinuxSetLabelResponse)
                    sendRequest(builder, fbRequest, RequestType.PathSELinuxSetLabelRequest,
                            ResponseType.PathSELinuxSetLabelResponse);

            if (!response.success()) {
                Log.e(TAG, "Failed to set SELinux label for " + path + ": " + response.errorMsg());
                return false;
            }

            return true;
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    public synchronized long pathGetDirectorySize(Context context, String path,
                                                  String[] exclusions) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbPath = builder.createString(path);
            int fbExclusions = 0;
            if (exclusions != null) {
                int[] exclusionOffsets = new int[exclusions.length];
                for (int i = 0; i < exclusions.length; i++) {
                    exclusionOffsets[i] = builder.createString(exclusions[i]);
                }

                fbExclusions = PathGetDirectorySizeRequest.createExclusionsVector(
                        builder, exclusionOffsets);
            }
            PathGetDirectorySizeRequest.startPathGetDirectorySizeRequest(builder);
            PathGetDirectorySizeRequest.addPath(builder, fbPath);
            PathGetDirectorySizeRequest.addExclusions(builder, fbExclusions);
            int fbRequest = PathGetDirectorySizeRequest.endPathGetDirectorySizeRequest(builder);

            // Send request
            PathGetDirectorySizeResponse response = (PathGetDirectorySizeResponse)
                    sendRequest(builder, fbRequest, RequestType.PathGetDirectorySizeRequest,
                            ResponseType.PathGetDirectorySizeResponse);

            if (!response.success()) {
                Log.e(TAG, "Failed to set directory size for " + path + ": " + response.errorMsg());
                return -1;
            }

            return response.size();
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    public interface SignedExecOutputCallback {
        void onOutputLine(String line);
    }

    public static class SignedExecCompletion {
        public short result;
        public String errorMsg;
        public int exitStatus;
        public int termSig;
    }

    public synchronized SignedExecCompletion signedExec(Context context, String path,
                                                        String sigPath, String arg0, String[] args,
                                                        SignedExecOutputCallback callback) throws IOException {
        connect(context);

        try {
            // Create request
            FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
            int fbPath = builder.createString(path);
            int fbSigPath = builder.createString(sigPath);
            int fbArg0 = 0;
            int fbArgv = 0;
            if (arg0 != null) {
                fbArg0 = builder.createString(arg0);
            }
            if (args != null) {
                int[] argsOffsets = new int[args.length];
                for (int i = 0; i < args.length; i++) {
                    argsOffsets[i] = builder.createString(args[i]);
                }

                fbArgv = SignedExecRequest.createArgsVector(builder, argsOffsets);
            }

            SignedExecRequest.startSignedExecRequest(builder);
            SignedExecRequest.addBinaryPath(builder, fbPath);
            SignedExecRequest.addSignaturePath(builder, fbSigPath);
            SignedExecRequest.addArg0(builder, fbArg0);
            SignedExecRequest.addArgs(builder, fbArgv);
            int fbRequest = SignedExecRequest.endSignedExecRequest(builder);

            // Send request

            Request.startRequest(builder);
            Request.addRequestType(builder, RequestType.SignedExecRequest);
            Request.addRequest(builder, fbRequest);
            builder.finish(Request.endRequest(builder));

            SocketUtils.writeBytes(mSocketOS, builder.sizedByteArray());

            // Loop until we get the completion response
            while (true) {
                byte[] responseBytes = SocketUtils.readBytes(mSocketIS);
                ByteBuffer bb = ByteBuffer.wrap(responseBytes);
                Response root = Response.getRootAsResponse(bb);

                Table table;

                switch (root.responseType()) {
                case ResponseType.SignedExecResponse:
                    table = new SignedExecResponse();
                    break;
                case ResponseType.SignedExecOutputResponse:
                    table = new SignedExecOutputResponse();
                    break;
                default:
                    throw new IOException("Invalid response type: " + root.responseType());
                }

                table = root.response(table);
                if (table == null) {
                    throw new IOException("Invalid union data");
                }

                if (root.responseType() == ResponseType.SignedExecResponse) {
                    SignedExecResponse response = (SignedExecResponse) table;

                    SignedExecCompletion result = new SignedExecCompletion();
                    result.result = response.result();
                    result.errorMsg = response.errorMsg();
                    result.exitStatus = response.exitStatus();
                    result.termSig = response.termSig();

                    if (response.result() != SignedExecResult.PROCESS_EXITED
                            && response.result() != SignedExecResult.PROCESS_KILLED_BY_SIGNAL) {
                        Log.e(TAG, "Signed exec error: " + response.errorMsg());
                    }

                    return result;
                } else if (root.responseType() == ResponseType.SignedExecOutputResponse) {
                    SignedExecOutputResponse response = (SignedExecOutputResponse) table;
                    if (callback != null) {
                        String line = response.line();
                        if (line != null) {
                            callback.onOutputLine(line);
                        }
                    }
                }
            }
        } catch (IOException e) {
            disconnect();
            throw e;
        }
    }

    // Private helper functions

    @NonNull
    private synchronized Table sendRequest(FlatBufferBuilder builder, int fbRequest,
                                           byte fbRequestType, byte expected) throws IOException {
        ThreadUtils.enforceExecutionOnNonMainThread();

        Request.startRequest(builder);
        Request.addRequestType(builder, fbRequestType);
        Request.addRequest(builder, fbRequest);
        builder.finish(Request.endRequest(builder));

        SocketUtils.writeBytes(mSocketOS, builder.sizedByteArray());

        byte[] responseBytes = SocketUtils.readBytes(mSocketIS);
        ByteBuffer bb = ByteBuffer.wrap(responseBytes);
        Response response = Response.getRootAsResponse(bb);

        if (response.responseType() == ResponseType.Unsupported) {
            throw new IOException("Unsupported command");
        } else if (response.responseType() == ResponseType.Invalid) {
            throw new IOException("Invalid command request");
        } else if (response.responseType() != expected) {
            throw new IOException("Unexpected response type (response=" + response.responseType() +
                    ", expected=" + expected + ")");
        }

        Table table;

        switch (response.responseType()) {
        case ResponseType.FileChmodResponse:
            table = new FileChmodResponse();
            break;
        case ResponseType.FileCloseResponse:
            table = new FileCloseResponse();
            break;
        case ResponseType.FileOpenResponse:
            table = new FileOpenResponse();
            break;
        case ResponseType.FileReadResponse:
            table = new FileReadResponse();
            break;
        case ResponseType.FileSeekResponse:
            table = new FileSeekResponse();
            break;
        case ResponseType.FileStatResponse:
            table = new FileStatResponse();
            break;
        case ResponseType.FileWriteResponse:
            table = new FileWriteResponse();
            break;
        case ResponseType.FileSELinuxGetLabelResponse:
            table = new FileSELinuxGetLabelResponse();
            break;
        case ResponseType.FileSELinuxSetLabelResponse:
            table = new FileSELinuxSetLabelResponse();
            break;
        case ResponseType.PathChmodResponse:
            table = new PathChmodResponse();
            break;
        case ResponseType.PathCopyResponse:
            table = new PathCopyResponse();
            break;
        case ResponseType.PathSELinuxGetLabelResponse:
            table = new PathSELinuxGetLabelResponse();
            break;
        case ResponseType.PathSELinuxSetLabelResponse:
            table = new PathSELinuxSetLabelResponse();
            break;
        case ResponseType.PathGetDirectorySizeResponse:
            table = new PathGetDirectorySizeResponse();
            break;
        case ResponseType.SignedExecResponse:
            table = new SignedExecResponse();
            break;
        case ResponseType.SignedExecOutputResponse:
            table = new SignedExecOutputResponse();
            break;
        case ResponseType.MbGetVersionResponse:
            table = new MbGetVersionResponse();
            break;
        case ResponseType.MbGetInstalledRomsResponse:
            table = new MbGetInstalledRomsResponse();
            break;
        case ResponseType.MbGetBootedRomIdResponse:
            table = new MbGetBootedRomIdResponse();
            break;
        case ResponseType.MbSwitchRomResponse:
            table = new MbSwitchRomResponse();
            break;
        case ResponseType.MbSetKernelResponse:
            table = new MbSetKernelResponse();
            break;
        case ResponseType.MbWipeRomResponse:
            table = new MbWipeRomResponse();
            break;
        case ResponseType.MbGetPackagesCountResponse:
            table = new MbGetPackagesCountResponse();
            break;
        case ResponseType.RebootResponse:
            table = new RebootResponse();
            break;
        default:
            throw new IOException("Invalid response type");
        }

        Table ret = response.response(table);

        if (ret == null) {
            throw new IOException("Invalid union data");
        }

        return ret;
    }
}
