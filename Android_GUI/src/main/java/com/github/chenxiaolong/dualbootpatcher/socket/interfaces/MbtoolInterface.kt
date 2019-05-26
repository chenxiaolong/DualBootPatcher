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

import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.Version
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException

import java.io.IOException
import java.nio.ByteBuffer

import mbtool.daemon.v3.FileOpenFlag
import mbtool.daemon.v3.FileSeekWhence
import mbtool.daemon.v3.PathDeleteFlag

interface MbtoolInterface {
    /**
     * Change the mode of an opened file.
     *
     * Logically equivalent to fchmod().
     *
     * @param id File ID
     * @param mode Desired file mode (will be masked with 0777)
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun fileChmod(id: Int, mode: Int)

    /**
     * Close an opened file.
     *
     * @param id File ID
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun fileClose(id: Int)

    /**
     * Open a file
     *
     * @param path Path to file
     * @param flags Flags (see [FileOpenFlag])
     * @param perms File mode (ignored unless [FileOpenFlag.CREAT] is provided)
     * @return File ID
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun fileOpen(path: String, flags: ShortArray, perms: Int): Int

    /**
     * Read data from an opened file
     *
     * @param id File ID
     * @param size Bytes to read
     * @return [ByteBuffer] containing the data read (possibly fewer bytes than specified)
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun fileRead(id: Int, size: Long): ByteBuffer

    /**
     * Seek an opened file.
     *
     * Logically equivalent to lseek().
     *
     * @param id File ID
     * @param offset Offset
     * @param whence Seek type (see [FileSeekWhence])
     * @return New file position
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun fileSeek(id: Int, offset: Long, whence: Short): Long

    /**
     * Stat an opened file
     *
     * Logically equivalent to fstat().
     *
     * @param id File ID
     * @return File stat buffer
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun fileStat(id: Int): StatBuf

    /**
     * Write to an opened file.
     *
     * @param id File ID
     * @param data Bytes to write
     * @return Number of bytes actually written
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun fileWrite(id: Int, data: ByteArray): Long

    /**
     * Get SELinux label of an opened file.
     *
     * @param id File ID
     * @return SELinux label
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun fileSelinuxGetLabel(id: Int): String

    /**
     * Set SELinux label of an opened file.
     *
     * @param id File ID
     * @param label SELinux label
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun fileSelinuxSetLabel(id: Int, label: String)

    /**
     * Get version of the mbtool daemon.
     *
     * @return mbtool version
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun getVersion() : Version

    /**
     * Get list of installed ROMs.
     *
     * NOTE: The list of ROMs will be re-queried and a new array will be returned every time this
     * method is called. It may be a good idea to cache the results after the initial call.
     *
     * @return Array of [RomInformation] objects representing the list of installed ROMs
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun getInstalledRoms() : Array<RomInformation>

    /**
     * Get the current ROM ID.
     *
     * @return String containing the current ROM ID
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun getBootedRomId() : String

    /**
     * Switch to another ROM.
     *
     * NOTE: If [SwitchRomResult.FAILED] is returned, there is no way of determining the cause
     * of failure programmatically. However, mbtool will likely print debugging information
     * (errno, etc.) to the logcat for manual reviewing.
     *
     * @param context Application context
     * @param id ID of ROM to switch to
     * @return [SwitchRomResult.UNKNOWN_BOOT_PARTITION] if the boot partition could not be determined
     * [SwitchRomResult.SUCCEEDED] if the ROM was successfully switched
     * [SwitchRomResult.FAILED] if the ROM failed to switch
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun switchRom(context: Context, id: String, forceChecksumsUpdate: Boolean): SwitchRomResult

    /**
     * Set the kernel for a ROM.
     *
     * NOTE: If [SetKernelResult.FAILED] is returned, there is no way of determining the cause
     * of failure programmatically. However, mbtool will likely print debugging information
     * (errno, etc.) to the logcat for manual reviewing.
     *
     * @param context Application context
     * @param id ID of ROM to set the kernel for
     * @return [SetKernelResult.UNKNOWN_BOOT_PARTITION] if the boot partition could not be determined
     * [SetKernelResult.SUCCEEDED] if setting the kernel was successful
     * [SetKernelResult.FAILED] if setting the kernel failed
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun setKernel(context: Context, id: String): SetKernelResult

    /**
     * Reboots the device via the framework.
     *
     * mbtool will launch an intent to start Android's ShutdownActivity
     *
     * @param confirm Whether Android's reboot dialog should be shown
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun rebootViaFramework(confirm: Boolean)

    /**
     * Reboots the device via init.
     *
     * NOTE: May result in an unclean shutdown as Android's init will simply kill all processes,
     * attempt to unmount filesystems, and then reboot.
     *
     * @param arg Reboot argument (eg. "recovery", "download", "bootloader"). Pass "" for a regular
     * reboot.
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun rebootViaInit(arg: String?)

    /**
     * Reboots the device via mbtool.
     *
     * NOTE: May result in an unclean shutdown as mbtool will simply kill all processes, attempt to
     * unmount filesystems, and then reboot.
     *
     * @param arg Reboot argument (eg. "recovery", "download", "bootloader"). Pass "" for a regular
     * reboot.
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun rebootViaMbtool(arg: String?)

    /**
     * Shuts down the device via the framework.
     *
     * mbtool will launch an intent to start Android's ShutdownActivity
     *
     * @param confirm Whether Android's shutdown dialog should be shown
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun shutdownViaFramework(confirm: Boolean)

    /**
     * Shuts down the device via init.
     *
     * NOTE: May result in an unclean shutdown as Android's init will simply kill all processes,
     * attempt to unmount filesystems, and then shut down.
     *
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun shutdownViaInit()

    /**
     * Shuts down the device via mbtool.
     *
     * NOTE: May result in an unclean shutdown as mbtool will simply kill all processes, attempt to
     * unmount filesystems, and then shut down.
     *
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun shutdownViaMbtool()

    /**
     * Copy a file using mbtool.
     *
     * @param source Absolute source path
     * @param target Absolute target path
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun pathCopy(source: String, target: String)

    /**
     * Delete a path using mbtool.
     *
     * @param path Path to delete
     * @param flag [PathDeleteFlag]
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun pathDelete(path: String, flag: Short)

    /**
     * Chmod a file using mbtool.
     *
     * @param filename Absolute path
     * @param mode Unix permissions number (will be AND'ed with 0777 by mbtool for security reasons)
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun pathChmod(filename: String, mode: Int)

    /**
     * Create a directory using mbtool.
     *
     * @param path Absolute path
     * @param mode Unix permissions number (will be AND'ed with 0777 by mbtool for security reasons)
     * @param recursive Whether to create directories recursively
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun pathMkdir(path: String, mode: Int, recursive: Boolean)

    /**
     * Get the target of a symlink.
     *
     * @param path Absolute path
     * @return Target of symlink
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun pathReadlink(path: String): String

    /**
     * Wipe a ROM.
     *
     * @param romId ROM ID to wipe
     * @param targets List of [mbtool.daemon.v3.MbWipeTarget]s indicating the wipe targets
     * @return [WipeResult] containing the list of succeeded and failed wipe targets
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun wipeRom(romId: String, targets: ShortArray): WipeResult

    /**
     * Get package counts for a ROM.
     *
     * @param romId ROM ID
     * @return [PackageCounts] object with counts of system packages, updated packages, and
     * user packages
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun getPackagesCounts(romId: String): PackageCounts

    /**
     * Get the SELinux label of a path.
     *
     * @param path Absolute path
     * @param followSymlinks Whether to follow symlinks
     * @return SELinux label if it was successfully retrieved. False, otherwise.
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun pathSelinuxGetLabel(path: String, followSymlinks: Boolean): String

    /**
     * Set the SELinux label for a path.
     *
     * @param path Absolute path
     * @param label SELinux label
     * @param followSymlinks Whether to follow symlinks
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun pathSelinuxSetLabel(path: String, label: String, followSymlinks: Boolean)

    /**
     * Recursively get the size of a directory's contents.
     *
     * @param path Path to directory
     * @param exclusions Top level directories to exclude
     * @return Size of directory in bytes
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun pathGetDirectorySize(path: String, exclusions: Array<String>?): Long

    /**
     * Run signed executable.
     *
     * @param path Path to executable
     * @param sigPath Path to signature file
     * @param arg0 Desired argv[0] value (or null to use the binary path)
     * @param args Arguments to pass to the executable
     * @param callback Callback for receiving output
     * @return [SignedExecCompletion]
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    fun signedExec(path: String, sigPath: String, arg0: String?, args: Array<String>?,
                   callback: SignedExecOutputListener?): SignedExecCompletion
}