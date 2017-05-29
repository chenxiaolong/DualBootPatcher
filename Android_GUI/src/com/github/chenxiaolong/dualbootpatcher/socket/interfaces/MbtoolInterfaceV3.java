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

package com.github.chenxiaolong.dualbootpatcher.socket.interfaces;

import android.content.Context;
import android.support.annotation.NonNull;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.ThreadUtils;
import com.github.chenxiaolong.dualbootpatcher.Version;
import com.github.chenxiaolong.dualbootpatcher.Version.VersionParseException;
import com.github.chenxiaolong.dualbootpatcher.socket.SocketUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;
import com.google.flatbuffers.FlatBufferBuilder;
import com.google.flatbuffers.Table;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;

import mbtool.daemon.v3.FileChmodError;
import mbtool.daemon.v3.FileChmodRequest;
import mbtool.daemon.v3.FileChmodResponse;
import mbtool.daemon.v3.FileCloseError;
import mbtool.daemon.v3.FileCloseRequest;
import mbtool.daemon.v3.FileCloseResponse;
import mbtool.daemon.v3.FileOpenError;
import mbtool.daemon.v3.FileOpenRequest;
import mbtool.daemon.v3.FileOpenResponse;
import mbtool.daemon.v3.FileReadError;
import mbtool.daemon.v3.FileReadRequest;
import mbtool.daemon.v3.FileReadResponse;
import mbtool.daemon.v3.FileSELinuxGetLabelError;
import mbtool.daemon.v3.FileSELinuxGetLabelRequest;
import mbtool.daemon.v3.FileSELinuxGetLabelResponse;
import mbtool.daemon.v3.FileSELinuxSetLabelError;
import mbtool.daemon.v3.FileSELinuxSetLabelRequest;
import mbtool.daemon.v3.FileSELinuxSetLabelResponse;
import mbtool.daemon.v3.FileSeekError;
import mbtool.daemon.v3.FileSeekRequest;
import mbtool.daemon.v3.FileSeekResponse;
import mbtool.daemon.v3.FileStatError;
import mbtool.daemon.v3.FileStatRequest;
import mbtool.daemon.v3.FileStatResponse;
import mbtool.daemon.v3.FileWriteError;
import mbtool.daemon.v3.FileWriteRequest;
import mbtool.daemon.v3.FileWriteResponse;
import mbtool.daemon.v3.MbGetBootedRomIdRequest;
import mbtool.daemon.v3.MbGetBootedRomIdResponse;
import mbtool.daemon.v3.MbGetInstalledRomsRequest;
import mbtool.daemon.v3.MbGetInstalledRomsResponse;
import mbtool.daemon.v3.MbGetPackagesCountError;
import mbtool.daemon.v3.MbGetPackagesCountRequest;
import mbtool.daemon.v3.MbGetPackagesCountResponse;
import mbtool.daemon.v3.MbGetVersionRequest;
import mbtool.daemon.v3.MbGetVersionResponse;
import mbtool.daemon.v3.MbRom;
import mbtool.daemon.v3.MbSetKernelError;
import mbtool.daemon.v3.MbSetKernelRequest;
import mbtool.daemon.v3.MbSetKernelResponse;
import mbtool.daemon.v3.MbSwitchRomRequest;
import mbtool.daemon.v3.MbSwitchRomResponse;
import mbtool.daemon.v3.MbSwitchRomResult;
import mbtool.daemon.v3.MbWipeRomRequest;
import mbtool.daemon.v3.MbWipeRomResponse;
import mbtool.daemon.v3.PathChmodError;
import mbtool.daemon.v3.PathChmodRequest;
import mbtool.daemon.v3.PathChmodResponse;
import mbtool.daemon.v3.PathCopyError;
import mbtool.daemon.v3.PathCopyRequest;
import mbtool.daemon.v3.PathCopyResponse;
import mbtool.daemon.v3.PathDeleteError;
import mbtool.daemon.v3.PathDeleteRequest;
import mbtool.daemon.v3.PathDeleteResponse;
import mbtool.daemon.v3.PathGetDirectorySizeError;
import mbtool.daemon.v3.PathGetDirectorySizeRequest;
import mbtool.daemon.v3.PathGetDirectorySizeResponse;
import mbtool.daemon.v3.PathMkdirError;
import mbtool.daemon.v3.PathMkdirRequest;
import mbtool.daemon.v3.PathMkdirResponse;
import mbtool.daemon.v3.PathReadlinkError;
import mbtool.daemon.v3.PathReadlinkRequest;
import mbtool.daemon.v3.PathReadlinkResponse;
import mbtool.daemon.v3.PathSELinuxGetLabelError;
import mbtool.daemon.v3.PathSELinuxGetLabelRequest;
import mbtool.daemon.v3.PathSELinuxGetLabelResponse;
import mbtool.daemon.v3.PathSELinuxSetLabelError;
import mbtool.daemon.v3.PathSELinuxSetLabelRequest;
import mbtool.daemon.v3.PathSELinuxSetLabelResponse;
import mbtool.daemon.v3.RebootError;
import mbtool.daemon.v3.RebootRequest;
import mbtool.daemon.v3.RebootResponse;
import mbtool.daemon.v3.RebootType;
import mbtool.daemon.v3.Request;
import mbtool.daemon.v3.RequestType;
import mbtool.daemon.v3.Response;
import mbtool.daemon.v3.ResponseType;
import mbtool.daemon.v3.ShutdownError;
import mbtool.daemon.v3.ShutdownRequest;
import mbtool.daemon.v3.ShutdownResponse;
import mbtool.daemon.v3.ShutdownType;
import mbtool.daemon.v3.SignedExecError;
import mbtool.daemon.v3.SignedExecOutputResponse;
import mbtool.daemon.v3.SignedExecRequest;
import mbtool.daemon.v3.SignedExecResponse;
import mbtool.daemon.v3.SignedExecResult;
import mbtool.daemon.v3.StructStat;

public class MbtoolInterfaceV3 implements MbtoolInterface {
    private static final String TAG = MbtoolInterfaceV3.class.getSimpleName();

    /** Flatbuffers buffer size (same as the C++ default) */
    private static final int FBB_SIZE = 1024;

    private InputStream mIS;
    private OutputStream mOS;

    public MbtoolInterfaceV3(InputStream is, OutputStream os) {
        mIS = is;
        mOS = os;
    }

    @NonNull
    private Table getTableFromResponse(Response response) throws MbtoolException {
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
        case ResponseType.PathDeleteResponse:
            table = new PathDeleteResponse();
            break;
        case ResponseType.PathMkdirResponse:
            table = new PathMkdirResponse();
            break;
        case ResponseType.PathReadlinkResponse:
            table = new PathReadlinkResponse();
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
        case ResponseType.ShutdownResponse:
            table = new ShutdownResponse();
            break;
        default:
            throw new MbtoolException(Reason.PROTOCOL_ERROR,
                    "Unknown response type: " + response.responseType());
        }

        table = response.response(table);
        if (table == null) {
            throw new MbtoolException(Reason.PROTOCOL_ERROR,
                    "Invalid union data in daemon response");
        }

        return table;
    }

    @NonNull
    private synchronized Table sendRequest(FlatBufferBuilder builder, int fbRequest,
                                           byte fbRequestType, byte expected)
            throws IOException, MbtoolException, MbtoolCommandException {
        ThreadUtils.enforceExecutionOnNonMainThread();

        // Build request table
        Request.startRequest(builder);
        Request.addRequestType(builder, fbRequestType);
        Request.addRequest(builder, fbRequest);
        builder.finish(Request.endRequest(builder));

        // Send request to daemon
        SocketUtils.writeBytes(mOS, builder.sizedByteArray());

        // Read response back as table
        byte[] responseBytes = SocketUtils.readBytes(mIS);
        ByteBuffer bb = ByteBuffer.wrap(responseBytes);
        Response response = Response.getRootAsResponse(bb);

        if (response.responseType() == ResponseType.Unsupported) {
            throw new MbtoolCommandException(
                    "Daemon does not support request type: " + fbRequestType);
        } else if (response.responseType() == ResponseType.Invalid) {
            throw new MbtoolCommandException(
                    "Daemon says request is invalid: " + fbRequestType);
        } else if (response.responseType() != expected) {
            throw new MbtoolException(Reason.PROTOCOL_ERROR,
                    "Unexpected response type (actual=" + response.responseType()
                            + ", expected=" + expected + ")");
        }

        return getTableFromResponse(response);
    }

    // RPC calls

    public synchronized void fileChmod(int id, int mode) throws IOException, MbtoolException,
            MbtoolCommandException {
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

        FileChmodError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), "[" + id + "]: chmod failed: " + error.msg());
        }
    }

    public synchronized void fileClose(int id) throws IOException, MbtoolException,
            MbtoolCommandException {
        // Create request
        FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
        FileCloseRequest.startFileCloseRequest(builder);
        FileCloseRequest.addId(builder, id);
        int fbRequest = FileCloseRequest.endFileCloseRequest(builder);

        // Send request
        FileCloseResponse response = (FileCloseResponse)
                sendRequest(builder, fbRequest, RequestType.FileCloseRequest,
                        ResponseType.FileCloseResponse);

        FileCloseError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), "[" + id + "]: close failed: " + error.msg());
        }
    }

    public synchronized int fileOpen(String path, short[] flags, int perms) throws IOException,
            MbtoolException, MbtoolCommandException {
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

        FileOpenError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), "[" + path + "]: open failed: " + error.msg());
        }

        return response.id();
    }

    @NonNull
    public synchronized ByteBuffer fileRead(int id, long size) throws IOException, MbtoolException,
            MbtoolCommandException {
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

        FileReadError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), "[" + id + "]: read failed: " + error.msg());
        }

        return response.dataAsByteBuffer();
    }

    public synchronized long fileSeek(int id, long offset, short whence) throws IOException,
            MbtoolException, MbtoolCommandException {
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

        FileSeekError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), "[" + id + "]: seek failed: " + error.msg());
        }

        return response.offset();
    }

    @NonNull
    public synchronized StatBuf fileStat(int id) throws IOException, MbtoolException,
            MbtoolCommandException {
        // Create request
        FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
        FileStatRequest.startFileStatRequest(builder);
        FileStatRequest.addId(builder, id);
        int fbRequest = FileStatRequest.endFileStatRequest(builder);

        // Send request
        FileStatResponse response = (FileStatResponse)
                sendRequest(builder, fbRequest, RequestType.FileStatRequest,
                        ResponseType.FileStatResponse);

        FileStatError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), "[" + id + "]: stat failed: " + error.msg());
        }

        StructStat ss = response.stat();
        StatBuf sb = new StatBuf();
        sb.st_dev = ss.dev();
        sb.st_ino = ss.ino();
        sb.st_mode = (int) ss.mode();
        sb.st_nlink = ss.nlink();
        sb.st_uid = (int) ss.uid();
        sb.st_gid = (int) ss.gid();
        sb.st_rdev = ss.rdev();
        sb.st_size = ss.size();
        sb.st_blksize = ss.blksize();
        sb.st_blocks = ss.blocks();
        sb.st_atime = ss.atime();
        sb.st_mtime = ss.mtime();
        sb.st_ctime = ss.ctime();
        return sb;
    }

    public synchronized long fileWrite(int id, byte[] data) throws IOException, MbtoolException,
            MbtoolCommandException {
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

        FileWriteError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), "[" + id + "]: write failed: " + error.msg());
        }

        return response.bytesWritten();
    }

    @NonNull
    public synchronized String fileSelinuxGetLabel(int id) throws IOException, MbtoolException,
            MbtoolCommandException {
        // Create request
        FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
        FileSELinuxGetLabelRequest.startFileSELinuxGetLabelRequest(builder);
        FileSELinuxGetLabelRequest.addId(builder, id);
        int fbRequest = FileSELinuxGetLabelRequest.endFileSELinuxGetLabelRequest(builder);

        // Send request
        FileSELinuxGetLabelResponse response = (FileSELinuxGetLabelResponse)
                sendRequest(builder, fbRequest, RequestType.FileSELinuxGetLabelRequest,
                        ResponseType.FileSELinuxGetLabelResponse);

        FileSELinuxGetLabelError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), "[" + id + "]: SELinux get label failed: " + error.msg());
        }

        String label = response.label();
        if (label == null) {
            throw new MbtoolCommandException(
                    "[" + id + "]: SELinux get label returned null label");
        }

        return label;
    }

    public synchronized void fileSelinuxSetLabel(int id, String label) throws IOException,
            MbtoolException, MbtoolCommandException {
        // Create request
        FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
        FileSELinuxSetLabelRequest.startFileSELinuxSetLabelRequest(builder);
        FileSELinuxSetLabelRequest.addId(builder, id);
        int fbRequest = FileSELinuxSetLabelRequest.endFileSELinuxSetLabelRequest(builder);

        // Send request
        FileSELinuxSetLabelResponse response = (FileSELinuxSetLabelResponse)
                sendRequest(builder, fbRequest, RequestType.FileSELinuxSetLabelRequest,
                        ResponseType.FileSELinuxSetLabelResponse);

        FileSELinuxSetLabelError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), "[" + id + "]: SELinux set label failed: " + error.msg());
        }
    }

    @NonNull
    public synchronized Version getVersion() throws IOException, MbtoolException,
            MbtoolCommandException {
        // Create request
        FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
        MbGetVersionRequest.startMbGetVersionRequest(builder);
        int fbRequest = MbGetVersionRequest.endMbGetVersionRequest(builder);

        // Send request
        MbGetVersionResponse response = (MbGetVersionResponse)
                sendRequest(builder, fbRequest, RequestType.MbGetVersionRequest,
                        ResponseType.MbGetVersionResponse);

        String version = response.version();
        if (version == null) {
            throw new MbtoolCommandException("Daemon returned null version");
        }

        try {
            return new Version(version);
        } catch (VersionParseException e) {
            throw new MbtoolCommandException("Invalid version number", e);
        }
    }

    @NonNull
    public synchronized RomInformation[] getInstalledRoms() throws IOException, MbtoolException,
            MbtoolCommandException {
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
            MbRom mbRom = response.roms(i);

            rom.setId(mbRom.id());
            rom.setSystemPath(mbRom.systemPath());
            rom.setCachePath(mbRom.cachePath());
            rom.setDataPath(mbRom.dataPath());
            rom.setVersion(mbRom.version());
            rom.setBuild(mbRom.build());
        }

        return roms;
    }

    @NonNull
    public synchronized String getBootedRomId() throws IOException, MbtoolException,
            MbtoolCommandException {
        // Create request
        FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
        MbGetBootedRomIdRequest.startMbGetBootedRomIdRequest(builder);
        // No parameters
        int fbRequest = MbGetBootedRomIdRequest.endMbGetBootedRomIdRequest(builder);

        // Send request
        MbGetBootedRomIdResponse response = (MbGetBootedRomIdResponse)
                sendRequest(builder, fbRequest, RequestType.MbGetBootedRomIdRequest,
                        ResponseType.MbGetBootedRomIdResponse);

        String romId = response.romId();
        if (romId == null) {
            throw new MbtoolCommandException("Daemon returned null booted ROM ID");
        }

        return romId;
    }

    @NonNull
    public synchronized SwitchRomResult switchRom(Context context, String id,
                                                  boolean forceChecksumsUpdate)
            throws IOException, MbtoolException, MbtoolCommandException {
        String bootBlockDev = SwitcherUtils.getBootPartition(context, this);
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
            throw new MbtoolCommandException("Invalid SwitchRomResult: " + response.result());
        }

        return result;
    }

    @NonNull
    public synchronized SetKernelResult setKernel(Context context, String id)
            throws IOException, MbtoolException, MbtoolCommandException {
        String bootBlockDev = SwitcherUtils.getBootPartition(context, this);
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

        MbSetKernelError error = response.error();
        if (error != null) {
            return SetKernelResult.FAILED;
        }

        return SetKernelResult.SUCCEEDED;
    }

    public synchronized void rebootViaFramework(boolean confirm) throws IOException,
            MbtoolException, MbtoolCommandException {
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

        RebootError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException("Failed to reboot via framework");
        }
    }

    public synchronized void rebootViaInit(String arg) throws IOException, MbtoolException,
            MbtoolCommandException {
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

        RebootError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException("Failed to reboot via init");
        }
    }

    public synchronized void rebootViaMbtool(String arg) throws IOException, MbtoolException,
            MbtoolCommandException {
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

        RebootError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException("Failed to reboot directly via mbtool");
        }
    }

    public synchronized void shutdownViaInit() throws IOException, MbtoolException,
            MbtoolCommandException {
        // Create request
        FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
        ShutdownRequest.startShutdownRequest(builder);
        ShutdownRequest.addType(builder, ShutdownType.INIT);
        int fbRequest = ShutdownRequest.endShutdownRequest(builder);

        // Send request
        ShutdownResponse response = (ShutdownResponse)
                sendRequest(builder, fbRequest, RequestType.ShutdownRequest,
                        ResponseType.ShutdownResponse);

        ShutdownError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException("Failed to shut down via init");
        }
    }

    public synchronized void shutdownViaMbtool() throws IOException, MbtoolException,
            MbtoolCommandException {
        // Create request
        FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
        ShutdownRequest.startShutdownRequest(builder);
        ShutdownRequest.addType(builder, ShutdownType.DIRECT);
        int fbRequest = ShutdownRequest.endShutdownRequest(builder);

        // Send request
        ShutdownResponse response = (ShutdownResponse)
                sendRequest(builder, fbRequest, RequestType.ShutdownRequest,
                        ResponseType.ShutdownResponse);

        ShutdownError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException("Failed to shut down directly via mbtool");
        }
    }

    public synchronized void pathCopy(String source, String target) throws IOException,
            MbtoolException, MbtoolCommandException {
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

        PathCopyError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), "Failed to copy from " + source + " to " + target + ": " +
                    error.msg());
        }
    }

    public synchronized void pathDelete(String path, short flag) throws IOException,
            MbtoolException, MbtoolCommandException {
        // Create request
        FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
        int fbPath = builder.createString(path);
        PathDeleteRequest.startPathDeleteRequest(builder);
        PathDeleteRequest.addPath(builder, fbPath);
        PathDeleteRequest.addFlag(builder, flag);
        int fbRequest = PathDeleteRequest.endPathDeleteRequest(builder);

        // Send request
        PathDeleteResponse response = (PathDeleteResponse)
                sendRequest(builder, fbRequest, RequestType.PathDeleteRequest,
                        ResponseType.PathDeleteResponse);

        PathDeleteError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), "Failed to delete " + path + ": " + error.msg());
        }
    }

    public synchronized void pathChmod(String filename, int mode) throws IOException,
            MbtoolException, MbtoolCommandException {
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

        PathChmodError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), "Failed to chmod " + filename + ": " + error.msg());
        }
    }

    public synchronized void pathMkdir(String path, int mode, boolean recursive) throws IOException,
            MbtoolException, MbtoolCommandException {
        // Create request
        FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
        int fbFilename = builder.createString(path);
        PathMkdirRequest.startPathMkdirRequest(builder);
        PathMkdirRequest.addPath(builder, fbFilename);
        PathMkdirRequest.addMode(builder, mode);
        PathMkdirRequest.addRecursive(builder, recursive);
        int fbRequest = PathMkdirRequest.endPathMkdirRequest(builder);

        // Send request
        PathMkdirResponse response = (PathMkdirResponse)
                sendRequest(builder, fbRequest, RequestType.PathMkdirRequest,
                        ResponseType.PathMkdirResponse);

        PathMkdirError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), path + ": Failed to mkdir: " + error.msg());
        }
    }

    @NonNull
    public synchronized String pathReadlink(String path) throws IOException, MbtoolException,
            MbtoolCommandException {
        // Create request
        FlatBufferBuilder builder = new FlatBufferBuilder(FBB_SIZE);
        int fbPath = builder.createString(path);
        int fbRequest = PathReadlinkRequest.createPathReadlinkRequest(builder, fbPath);

        // Send request
        PathReadlinkResponse response = (PathReadlinkResponse)
                sendRequest(builder, fbRequest, RequestType.PathReadlinkRequest,
                        ResponseType.PathReadlinkResponse);

        PathReadlinkError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), path + ": Failed to resolve symlink: " + error.msg());
        }

        String target = response.target();
        if (target == null) {
            throw new MbtoolCommandException(path + ": Got null symlink target on success");
        }

        return target;
    }

    @NonNull
    public synchronized WipeResult wipeRom(String romId, short[] targets) throws IOException,
            MbtoolException, MbtoolCommandException {
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
    }

    @NonNull
    public synchronized PackageCounts getPackagesCounts(String romId) throws IOException,
            MbtoolException, MbtoolCommandException {
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

        MbGetPackagesCountError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException("Failed to get packages counts");
        }

        PackageCounts pc = new PackageCounts();
        pc.systemPackages = (int) response.systemPackages();
        pc.systemUpdatePackages = (int) response.systemUpdatePackages();
        pc.nonSystemPackages = (int) response.nonSystemPackages();
        return pc;
    }

    @NonNull
    public synchronized String pathSelinuxGetLabel(String path, boolean followSymlinks)
            throws IOException, MbtoolException, MbtoolCommandException {
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

        PathSELinuxGetLabelError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), "Failed to get SELinux label for " + path + ": " +
                    error.msg());
        }

        String label = response.label();
        if (label == null) {
            throw new MbtoolCommandException("Got null SELinux label for " + path);
        }

        return label;
    }

    public synchronized void pathSelinuxSetLabel(String path, String label, boolean followSymlinks)
            throws IOException, MbtoolException, MbtoolCommandException {
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

        PathSELinuxSetLabelError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), "Failed to set SELinux label for " + path + ": " +
                    error.msg());
        }
    }

    public synchronized long pathGetDirectorySize(String path, String[] exclusions)
            throws IOException, MbtoolException, MbtoolCommandException {
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

        PathGetDirectorySizeError error = response.error();
        if (error != null) {
            throw new MbtoolCommandException(
                    error.errnoValue(), "Failed to get directory size for " + path + ": " +
                    error.msg());
        }

        return response.size();
    }

    public synchronized SignedExecCompletion signedExec(String path, String sigPath,
                                                        String arg0, String[] args,
                                                        SignedExecOutputListener callback)
            throws IOException, MbtoolException, MbtoolCommandException {
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

        SocketUtils.writeBytes(mOS, builder.sizedByteArray());

        // Loop until we get the completion response
        while (true) {
            byte[] responseBytes = SocketUtils.readBytes(mIS);
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
                throw new MbtoolException(Reason.PROTOCOL_ERROR,
                        "Invalid response type: " + root.responseType());
            }

            table = root.response(table);
            if (table == null) {
                throw new MbtoolException(Reason.PROTOCOL_ERROR, "Invalid union data");
            }

            if (root.responseType() == ResponseType.SignedExecResponse) {
                SignedExecResponse response = (SignedExecResponse) table;
                SignedExecError error = response.error();

                SignedExecCompletion result = new SignedExecCompletion();
                result.result = response.result();
                result.exitStatus = response.exitStatus();
                result.termSig = response.termSig();
                if (error != null) {
                    result.errorMsg = error.msg();
                }

                if (response.result() != SignedExecResult.PROCESS_EXITED
                        && response.result() != SignedExecResult.PROCESS_KILLED_BY_SIGNAL) {
                    if (error != null) {
                        Log.e(TAG, "Signed exec error: " + error.msg());
                    } else {
                        Log.e(TAG, "No signed exec error given");
                    }
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
    }
}
