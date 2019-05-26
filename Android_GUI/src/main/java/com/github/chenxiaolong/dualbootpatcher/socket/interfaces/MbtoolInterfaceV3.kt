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

package com.github.chenxiaolong.dualbootpatcher.socket.interfaces

import android.content.Context
import android.util.Log
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.ThreadUtils
import com.github.chenxiaolong.dualbootpatcher.Version
import com.github.chenxiaolong.dualbootpatcher.Version.VersionParseException
import com.github.chenxiaolong.dualbootpatcher.socket.SocketUtils
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils
import com.google.flatbuffers.FlatBufferBuilder
import com.google.flatbuffers.Table
import mbtool.daemon.v3.*
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream
import java.nio.ByteBuffer

class MbtoolInterfaceV3(
        private val sis: InputStream,
        private val sos: OutputStream
) : MbtoolInterface {
    @Throws(MbtoolException::class)
    private fun getTableFromResponse(response: Response): Table {
        var table: Table? = when (response.responseType()) {
            ResponseType.FileChmodResponse -> FileChmodResponse()
            ResponseType.FileCloseResponse -> FileCloseResponse()
            ResponseType.FileOpenResponse -> FileOpenResponse()
            ResponseType.FileReadResponse -> FileReadResponse()
            ResponseType.FileSeekResponse -> FileSeekResponse()
            ResponseType.FileStatResponse -> FileStatResponse()
            ResponseType.FileWriteResponse -> FileWriteResponse()
            ResponseType.FileSELinuxGetLabelResponse -> FileSELinuxGetLabelResponse()
            ResponseType.FileSELinuxSetLabelResponse -> FileSELinuxSetLabelResponse()
            ResponseType.PathChmodResponse -> PathChmodResponse()
            ResponseType.PathCopyResponse -> PathCopyResponse()
            ResponseType.PathDeleteResponse -> PathDeleteResponse()
            ResponseType.PathMkdirResponse -> PathMkdirResponse()
            ResponseType.PathReadlinkResponse -> PathReadlinkResponse()
            ResponseType.PathSELinuxGetLabelResponse -> PathSELinuxGetLabelResponse()
            ResponseType.PathSELinuxSetLabelResponse -> PathSELinuxSetLabelResponse()
            ResponseType.PathGetDirectorySizeResponse -> PathGetDirectorySizeResponse()
            ResponseType.SignedExecResponse -> SignedExecResponse()
            ResponseType.SignedExecOutputResponse -> SignedExecOutputResponse()
            ResponseType.MbGetVersionResponse -> MbGetVersionResponse()
            ResponseType.MbGetInstalledRomsResponse -> MbGetInstalledRomsResponse()
            ResponseType.MbGetBootedRomIdResponse -> MbGetBootedRomIdResponse()
            ResponseType.MbSwitchRomResponse -> MbSwitchRomResponse()
            ResponseType.MbSetKernelResponse -> MbSetKernelResponse()
            ResponseType.MbWipeRomResponse -> MbWipeRomResponse()
            ResponseType.MbGetPackagesCountResponse -> MbGetPackagesCountResponse()
            ResponseType.RebootResponse -> RebootResponse()
            ResponseType.ShutdownResponse -> ShutdownResponse()
            else -> throw MbtoolException(Reason.PROTOCOL_ERROR,
                    "Unknown response type: ${response.responseType()}")
        }

        table = response.response(table)
        if (table == null) {
            throw MbtoolException(Reason.PROTOCOL_ERROR,
                    "Invalid union data in daemon response")
        }

        return table
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    private fun sendRequest(builder: FlatBufferBuilder, fbRequest: Int, fbRequestType: Byte,
                            expected: Byte): Table {
        ThreadUtils.enforceExecutionOnNonMainThread()

        // Build request table
        Request.startRequest(builder)
        Request.addRequestType(builder, fbRequestType)
        Request.addRequest(builder, fbRequest)
        builder.finish(Request.endRequest(builder))

        // Send request to daemon
        SocketUtils.writeBytes(sos, builder.sizedByteArray())

        // Read response back as table
        val responseBytes = SocketUtils.readBytes(sis)
        val bb = ByteBuffer.wrap(responseBytes)
        val response = Response.getRootAsResponse(bb)

        when {
            response.responseType() == ResponseType.Unsupported -> throw MbtoolCommandException(
                    "Daemon does not support request type: $fbRequestType")
            response.responseType() == ResponseType.Invalid -> throw MbtoolCommandException(
                    "Daemon says request is invalid: $fbRequestType")
            response.responseType() != expected -> throw MbtoolException(Reason.PROTOCOL_ERROR,
                    "Unexpected response type (actual=${response.responseType()}"
                            + ", expected=$expected)")
            else -> return getTableFromResponse(response)
        }
    }

    // RPC calls

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun fileChmod(id: Int, mode: Int) {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        FileChmodRequest.startFileChmodRequest(builder)
        FileChmodRequest.addId(builder, id)
        FileChmodRequest.addMode(builder, mode.toLong())
        val fbRequest = FileChmodRequest.endFileChmodRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.FileChmodRequest,
                ResponseType.FileChmodResponse) as FileChmodResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "[$id]: chmod failed: ${error.msg()}")
        }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun fileClose(id: Int) {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        FileCloseRequest.startFileCloseRequest(builder)
        FileCloseRequest.addId(builder, id)
        val fbRequest = FileCloseRequest.endFileCloseRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.FileCloseRequest,
                ResponseType.FileCloseResponse) as FileCloseResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "[$id]: close failed: ${error.msg()}")
        }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun fileOpen(path: String, flags: ShortArray, perms: Int): Int {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)

        val fbPath = builder.createString(path)
        val fbFlags = FileOpenRequest.createFlagsVector(builder, flags)

        FileOpenRequest.startFileOpenRequest(builder)
        FileOpenRequest.addPath(builder, fbPath)
        FileOpenRequest.addFlags(builder, fbFlags)
        FileOpenRequest.addPerms(builder, perms.toLong())
        val fbRequest = FileOpenRequest.endFileOpenRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.FileOpenRequest,
                ResponseType.FileOpenResponse) as FileOpenResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "[$path]: open failed: ${error.msg()}")
        }

        return response.id()
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun fileRead(id: Int, size: Long): ByteBuffer {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        FileReadRequest.startFileReadRequest(builder)
        FileReadRequest.addId(builder, id)
        FileReadRequest.addCount(builder, size)
        val fbRequest = FileReadRequest.endFileReadRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.FileReadRequest,
                ResponseType.FileReadResponse) as FileReadResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "[$id]: read failed: ${error.msg()}")
        }

        return response.dataAsByteBuffer()
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun fileSeek(id: Int, offset: Long, whence: Short): Long {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        FileSeekRequest.startFileSeekRequest(builder)
        FileSeekRequest.addId(builder, id)
        FileSeekRequest.addOffset(builder, offset)
        FileSeekRequest.addWhence(builder, whence)
        val fbRequest = FileSeekRequest.endFileSeekRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.FileSeekRequest,
                ResponseType.FileSeekResponse) as FileSeekResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "[$id]: seek failed: ${error.msg()}")
        }

        return response.offset()
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun fileStat(id: Int): StatBuf {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        FileStatRequest.startFileStatRequest(builder)
        FileStatRequest.addId(builder, id)
        val fbRequest = FileStatRequest.endFileStatRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.FileStatRequest,
                ResponseType.FileStatResponse) as FileStatResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "[$id]: stat failed: ${error.msg()}")
        }

        val ss = response.stat()
        val sb = StatBuf()
        sb.st_dev = ss.dev()
        sb.st_ino = ss.ino()
        sb.st_mode = ss.mode().toInt()
        sb.st_nlink = ss.nlink()
        sb.st_uid = ss.uid().toInt()
        sb.st_gid = ss.gid().toInt()
        sb.st_rdev = ss.rdev()
        sb.st_size = ss.size()
        sb.st_blksize = ss.blksize()
        sb.st_blocks = ss.blocks()
        sb.st_atime = ss.atime()
        sb.st_mtime = ss.mtime()
        sb.st_ctime = ss.ctime()
        return sb
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun fileWrite(id: Int, data: ByteArray): Long {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbData = FileWriteRequest.createDataVector(builder, data)
        FileWriteRequest.startFileWriteRequest(builder)
        FileWriteRequest.addId(builder, id)
        FileWriteRequest.addData(builder, fbData)
        val fbRequest = FileWriteRequest.endFileWriteRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.FileWriteRequest,
                ResponseType.FileWriteResponse) as FileWriteResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "[$id]: write failed: ${error.msg()}")
        }

        return response.bytesWritten()
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun fileSelinuxGetLabel(id: Int): String {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        FileSELinuxGetLabelRequest.startFileSELinuxGetLabelRequest(builder)
        FileSELinuxGetLabelRequest.addId(builder, id)
        val fbRequest = FileSELinuxGetLabelRequest.endFileSELinuxGetLabelRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.FileSELinuxGetLabelRequest,
                ResponseType.FileSELinuxGetLabelResponse) as FileSELinuxGetLabelResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "[$id]: SELinux get label failed: ${error.msg()}")
        }

        return response.label() ?: throw MbtoolCommandException(
                "[$id]: SELinux get label returned null label")
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun fileSelinuxSetLabel(id: Int, label: String) {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        FileSELinuxSetLabelRequest.startFileSELinuxSetLabelRequest(builder)
        FileSELinuxSetLabelRequest.addId(builder, id)
        val fbRequest = FileSELinuxSetLabelRequest.endFileSELinuxSetLabelRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.FileSELinuxSetLabelRequest,
                ResponseType.FileSELinuxSetLabelResponse) as FileSELinuxSetLabelResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "[$id]: SELinux set label failed: ${error.msg()}")
        }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun getVersion(): Version {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        MbGetVersionRequest.startMbGetVersionRequest(builder)
        val fbRequest = MbGetVersionRequest.endMbGetVersionRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.MbGetVersionRequest,
                ResponseType.MbGetVersionResponse) as MbGetVersionResponse

        val version = response.version() ?:
                throw MbtoolCommandException("Daemon returned null version")

        try {
            return Version(version)
        } catch (e: VersionParseException) {
            throw MbtoolCommandException("Invalid version number", e)
        }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun getInstalledRoms(): Array<RomInformation> {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        MbGetInstalledRomsRequest.startMbGetInstalledRomsRequest(builder)
        // No parameters
        val fbRequest = MbGetInstalledRomsRequest.endMbGetInstalledRomsRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.MbGetInstalledRomsRequest,
                ResponseType.MbGetInstalledRomsResponse) as MbGetInstalledRomsResponse

        return (0 until response.romsLength()).map {
            val rom = RomInformation()
            val mbRom = response.roms(it)

            rom.id = mbRom.id()
            rom.systemPath = mbRom.systemPath()
            rom.cachePath = mbRom.cachePath()
            rom.dataPath = mbRom.dataPath()
            rom.version = mbRom.version()
            rom.build = mbRom.build()

            rom
        }.toTypedArray()
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun getBootedRomId(): String {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        MbGetBootedRomIdRequest.startMbGetBootedRomIdRequest(builder)
        // No parameters
        val fbRequest = MbGetBootedRomIdRequest.endMbGetBootedRomIdRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.MbGetBootedRomIdRequest,
                ResponseType.MbGetBootedRomIdResponse) as MbGetBootedRomIdResponse

        return response.romId() ?:
                throw MbtoolCommandException("Daemon returned null booted ROM ID")
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun switchRom(context: Context, id: String,
                           forceChecksumsUpdate: Boolean): SwitchRomResult {
        val bootBlockDev = SwitcherUtils.getBootPartition(context, this)
        if (bootBlockDev == null) {
            Log.e(TAG, "Failed to determine boot partition")
            return SwitchRomResult.UNKNOWN_BOOT_PARTITION
        }

        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbRomId = builder.createString(id)
        val fbBootBlockDev = builder.createString(bootBlockDev)

        // Blockdev search dirs
        val searchDirs = SwitcherUtils.getBlockDevSearchDirs(context)
        var fbSearchDirs = 0
        if (searchDirs != null) {
            val searchDirsOffsets = IntArray(searchDirs.size)
            for (i in searchDirs.indices) {
                searchDirsOffsets[i] = builder.createString(searchDirs[i])
            }

            fbSearchDirs = MbSwitchRomRequest.createBlockdevBaseDirsVector(
                    builder, searchDirsOffsets)
        }

        MbSwitchRomRequest.startMbSwitchRomRequest(builder)
        MbSwitchRomRequest.addRomId(builder, fbRomId)
        MbSwitchRomRequest.addBootBlockdev(builder, fbBootBlockDev)
        MbSwitchRomRequest.addBlockdevBaseDirs(builder, fbSearchDirs)
        MbSwitchRomRequest.addForceUpdateChecksums(builder, forceChecksumsUpdate)
        val fbRequest = MbSwitchRomRequest.endMbSwitchRomRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.MbSwitchRomRequest,
                ResponseType.MbSwitchRomResponse) as MbSwitchRomResponse

        return when (response.result()) {
            MbSwitchRomResult.SUCCEEDED -> SwitchRomResult.SUCCEEDED
            MbSwitchRomResult.FAILED -> SwitchRomResult.FAILED
            MbSwitchRomResult.CHECKSUM_INVALID -> SwitchRomResult.CHECKSUM_INVALID
            MbSwitchRomResult.CHECKSUM_NOT_FOUND -> SwitchRomResult.CHECKSUM_NOT_FOUND
            else -> throw MbtoolCommandException("Invalid SwitchRomResult: ${response.result()}")
        }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun setKernel(context: Context, id: String): SetKernelResult {
        val bootBlockDev = SwitcherUtils.getBootPartition(context, this)
        if (bootBlockDev == null) {
            Log.e(TAG, "Failed to determine boot partition")
            return SetKernelResult.UNKNOWN_BOOT_PARTITION
        }

        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbRomId = builder.createString(id)
        val fbBootBlockDev = builder.createString(bootBlockDev)
        MbSetKernelRequest.startMbSetKernelRequest(builder)
        MbSetKernelRequest.addRomId(builder, fbRomId)
        MbSetKernelRequest.addBootBlockdev(builder, fbBootBlockDev)
        val fbRequest = MbSetKernelRequest.endMbSetKernelRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.MbSetKernelRequest,
                ResponseType.MbSetKernelResponse) as MbSetKernelResponse

        val error = response.error()
        return if (error != null) SetKernelResult.FAILED else SetKernelResult.SUCCEEDED
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun rebootViaFramework(confirm: Boolean) {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        RebootRequest.startRebootRequest(builder)
        RebootRequest.addType(builder, RebootType.FRAMEWORK)
        RebootRequest.addConfirm(builder, confirm)
        val fbRequest = RebootRequest.endRebootRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.RebootRequest,
                ResponseType.RebootResponse) as RebootResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException("Failed to reboot via framework")
        }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun rebootViaInit(arg: String?) {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbArg = builder.createString(arg ?: "")
        RebootRequest.startRebootRequest(builder)
        RebootRequest.addType(builder, RebootType.INIT)
        RebootRequest.addArg(builder, fbArg)
        val fbRequest = RebootRequest.endRebootRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.RebootRequest,
                ResponseType.RebootResponse) as RebootResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException("Failed to reboot via init")
        }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun rebootViaMbtool(arg: String?) {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbArg = builder.createString(arg ?: "")
        RebootRequest.startRebootRequest(builder)
        RebootRequest.addType(builder, RebootType.DIRECT)
        RebootRequest.addArg(builder, fbArg)
        val fbRequest = RebootRequest.endRebootRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.RebootRequest,
                ResponseType.RebootResponse) as RebootResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException("Failed to reboot directly via mbtool")
        }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun shutdownViaFramework(confirm: Boolean) {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        ShutdownRequest.startShutdownRequest(builder);
        ShutdownRequest.addType(builder, ShutdownType.FRAMEWORK);
        ShutdownRequest.addConfirm(builder, confirm);
        val fbRequest = ShutdownRequest.endShutdownRequest(builder);

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.ShutdownRequest,
                ResponseType.ShutdownResponse) as ShutdownResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException("Failed to shut down via framework")
        }
    }


    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun shutdownViaInit() {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        ShutdownRequest.startShutdownRequest(builder)
        ShutdownRequest.addType(builder, ShutdownType.INIT)
        val fbRequest = ShutdownRequest.endShutdownRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.ShutdownRequest,
                ResponseType.ShutdownResponse) as ShutdownResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException("Failed to shut down via init")
        }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun shutdownViaMbtool() {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        ShutdownRequest.startShutdownRequest(builder)
        ShutdownRequest.addType(builder, ShutdownType.DIRECT)
        val fbRequest = ShutdownRequest.endShutdownRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.ShutdownRequest,
                ResponseType.ShutdownResponse) as ShutdownResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException("Failed to shut down directly via mbtool")
        }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun pathCopy(source: String, target: String) {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbSource = builder.createString(source)
        val fbTarget = builder.createString(target)
        PathCopyRequest.startPathCopyRequest(builder)
        PathCopyRequest.addSource(builder, fbSource)
        PathCopyRequest.addTarget(builder, fbTarget)
        val fbRequest = PathCopyRequest.endPathCopyRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.PathCopyRequest,
                ResponseType.PathCopyResponse) as PathCopyResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "Failed to copy from $source to $target: ${error.msg()}")
        }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun pathDelete(path: String, flag: Short) {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbPath = builder.createString(path)
        PathDeleteRequest.startPathDeleteRequest(builder)
        PathDeleteRequest.addPath(builder, fbPath)
        PathDeleteRequest.addFlag(builder, flag)
        val fbRequest = PathDeleteRequest.endPathDeleteRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.PathDeleteRequest,
                ResponseType.PathDeleteResponse) as PathDeleteResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "Failed to delete $path: ${error.msg()}")
        }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun pathChmod(filename: String, mode: Int) {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbFilename = builder.createString(filename)
        PathChmodRequest.startPathChmodRequest(builder)
        PathChmodRequest.addPath(builder, fbFilename)
        PathChmodRequest.addMode(builder, mode.toLong())
        val fbRequest = PathChmodRequest.endPathChmodRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.PathChmodRequest,
                ResponseType.PathChmodResponse) as PathChmodResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "Failed to chmod $filename: ${error.msg()}")
        }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun pathMkdir(path: String, mode: Int, recursive: Boolean) {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbFilename = builder.createString(path)
        PathMkdirRequest.startPathMkdirRequest(builder)
        PathMkdirRequest.addPath(builder, fbFilename)
        PathMkdirRequest.addMode(builder, mode.toLong())
        PathMkdirRequest.addRecursive(builder, recursive)
        val fbRequest = PathMkdirRequest.endPathMkdirRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.PathMkdirRequest,
                ResponseType.PathMkdirResponse) as PathMkdirResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "$path: Failed to mkdir: ${error.msg()}")
        }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun pathReadlink(path: String): String {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbPath = builder.createString(path)
        val fbRequest = PathReadlinkRequest.createPathReadlinkRequest(builder, fbPath)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.PathReadlinkRequest,
                ResponseType.PathReadlinkResponse) as PathReadlinkResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "$path: Failed to resolve symlink: ${error.msg()}")
        }

        return response.target() ?:
                throw MbtoolCommandException("$path: Got null symlink target on success")
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun wipeRom(romId: String, targets: ShortArray): WipeResult {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbRomId = builder.createString(romId)
        val fbTargets = MbWipeRomRequest.createTargetsVector(builder, targets)
        MbWipeRomRequest.startMbWipeRomRequest(builder)
        MbWipeRomRequest.addRomId(builder, fbRomId)
        MbWipeRomRequest.addTargets(builder, fbTargets)
        val fbRequest = MbWipeRomRequest.endMbWipeRomRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.MbWipeRomRequest,
                ResponseType.MbWipeRomResponse) as MbWipeRomResponse

        val result = WipeResult(ShortArray(response.succeededLength()),
                ShortArray(response.failedLength()))

        for (i in 0 until response.succeededLength()) {
            result.succeeded[i] = response.succeeded(i)
        }
        for (i in 0 until response.failedLength()) {
            result.failed[i] = response.failed(i)
        }

        return result
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun getPackagesCounts(romId: String): PackageCounts {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbRomId = builder.createString(romId)
        MbGetPackagesCountRequest.startMbGetPackagesCountRequest(builder)
        MbGetPackagesCountRequest.addRomId(builder, fbRomId)
        val fbRequest = MbGetPackagesCountRequest.endMbGetPackagesCountRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.MbGetPackagesCountRequest,
                ResponseType.MbGetPackagesCountResponse) as MbGetPackagesCountResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException("Failed to get packages counts")
        }

        return PackageCounts(response.systemPackages().toInt(),
                response.systemUpdatePackages().toInt(), response.nonSystemPackages().toInt())
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun pathSelinuxGetLabel(path: String, followSymlinks: Boolean): String {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbPath = builder.createString(path)
        PathSELinuxGetLabelRequest.startPathSELinuxGetLabelRequest(builder)
        PathSELinuxGetLabelRequest.addPath(builder, fbPath)
        PathSELinuxGetLabelRequest.addFollowSymlinks(builder, followSymlinks)
        val fbRequest = PathSELinuxGetLabelRequest.endPathSELinuxGetLabelRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.PathSELinuxGetLabelRequest,
                ResponseType.PathSELinuxGetLabelResponse) as PathSELinuxGetLabelResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "Failed to get SELinux label for $path: ${error.msg()}")
        }

        return response.label() ?: throw MbtoolCommandException("Got null SELinux label for $path")
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun pathSelinuxSetLabel(path: String, label: String, followSymlinks: Boolean) {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbPath = builder.createString(path)
        val fbLabel = builder.createString(label)
        PathSELinuxSetLabelRequest.startPathSELinuxSetLabelRequest(builder)
        PathSELinuxSetLabelRequest.addPath(builder, fbPath)
        PathSELinuxSetLabelRequest.addLabel(builder, fbLabel)
        PathSELinuxSetLabelRequest.addFollowSymlinks(builder, followSymlinks)
        val fbRequest = PathSELinuxSetLabelRequest.endPathSELinuxSetLabelRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.PathSELinuxSetLabelRequest,
                ResponseType.PathSELinuxSetLabelResponse) as PathSELinuxSetLabelResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "Failed to set SELinux label for $path: ${error.msg()}")
        }
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun pathGetDirectorySize(path: String, exclusions: Array<String>?): Long {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbPath = builder.createString(path)
        var fbExclusions = 0
        if (exclusions != null) {
            val exclusionOffsets = IntArray(exclusions.size)
            for (i in exclusions.indices) {
                exclusionOffsets[i] = builder.createString(exclusions[i])
            }

            fbExclusions = PathGetDirectorySizeRequest.createExclusionsVector(
                    builder, exclusionOffsets)
        }
        PathGetDirectorySizeRequest.startPathGetDirectorySizeRequest(builder)
        PathGetDirectorySizeRequest.addPath(builder, fbPath)
        PathGetDirectorySizeRequest.addExclusions(builder, fbExclusions)
        val fbRequest = PathGetDirectorySizeRequest.endPathGetDirectorySizeRequest(builder)

        // Send request
        val response = sendRequest(builder, fbRequest, RequestType.PathGetDirectorySizeRequest,
                ResponseType.PathGetDirectorySizeResponse) as PathGetDirectorySizeResponse

        val error = response.error()
        if (error != null) {
            throw MbtoolCommandException(
                    error.errnoValue(), "Failed to get directory size for $path: ${error.msg()}")
        }

        return response.size()
    }

    @Synchronized
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    override fun signedExec(path: String, sigPath: String, arg0: String?, args: Array<String>?,
                            callback: SignedExecOutputListener?): SignedExecCompletion {
        // Create request
        val builder = FlatBufferBuilder(FBB_SIZE)
        val fbPath = builder.createString(path)
        val fbSigPath = builder.createString(sigPath)
        var fbArg0 = 0
        var fbArgv = 0
        if (arg0 != null) {
            fbArg0 = builder.createString(arg0)
        }
        if (args != null) {
            val argsOffsets = IntArray(args.size)
            for (i in args.indices) {
                argsOffsets[i] = builder.createString(args[i])
            }

            fbArgv = SignedExecRequest.createArgsVector(builder, argsOffsets)
        }

        SignedExecRequest.startSignedExecRequest(builder)
        SignedExecRequest.addBinaryPath(builder, fbPath)
        SignedExecRequest.addSignaturePath(builder, fbSigPath)
        SignedExecRequest.addArg0(builder, fbArg0)
        SignedExecRequest.addArgs(builder, fbArgv)
        val fbRequest = SignedExecRequest.endSignedExecRequest(builder)

        // Send request

        Request.startRequest(builder)
        Request.addRequestType(builder, RequestType.SignedExecRequest)
        Request.addRequest(builder, fbRequest)
        builder.finish(Request.endRequest(builder))

        SocketUtils.writeBytes(sos, builder.sizedByteArray())

        // Loop until we get the completion response
        while (true) {
            val responseBytes = SocketUtils.readBytes(sis)
            val bb = ByteBuffer.wrap(responseBytes)
            val root = Response.getRootAsResponse(bb)

            var table: Table? = when (root.responseType()) {
                ResponseType.SignedExecResponse -> SignedExecResponse()
                ResponseType.SignedExecOutputResponse -> SignedExecOutputResponse()
                else -> throw MbtoolException(Reason.PROTOCOL_ERROR,
                        "Invalid response type: ${root.responseType()}")
            }

            table = root.response(table)
            if (table == null) {
                throw MbtoolException(Reason.PROTOCOL_ERROR, "Invalid union data")
            }

            if (root.responseType() == ResponseType.SignedExecResponse) {
                val response = table as SignedExecResponse?
                val error = response!!.error()

                val result = SignedExecCompletion()
                result.result = response.result()
                result.exitStatus = response.exitStatus()
                result.termSig = response.termSig()
                if (error != null) {
                    result.errorMsg = error.msg()
                }

                if (response.result() != SignedExecResult.PROCESS_EXITED
                        && response.result() != SignedExecResult.PROCESS_KILLED_BY_SIGNAL) {
                    if (error != null) {
                        Log.e(TAG, "Signed exec error: ${error.msg()}")
                    } else {
                        Log.e(TAG, "No signed exec error given")
                    }
                }

                return result
            } else if (root.responseType() == ResponseType.SignedExecOutputResponse) {
                val response = table as SignedExecOutputResponse?
                if (callback != null) {
                    val line = response!!.line()
                    if (line != null) {
                        callback.onOutputLine(line)
                    }
                }
            }
        }
    }

    companion object {
        private val TAG = MbtoolInterfaceV3::class.java.simpleName

        /** Flatbuffers buffer size (same as the C++ default)  */
        private const val FBB_SIZE = 1024
    }
}
