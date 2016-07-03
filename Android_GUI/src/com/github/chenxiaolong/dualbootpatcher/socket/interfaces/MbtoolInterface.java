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

import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.Version;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException;
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException;

import java.io.IOException;
import java.nio.ByteBuffer;

import mbtool.daemon.v3.FileOpenFlag;
import mbtool.daemon.v3.FileSeekWhence;
import mbtool.daemon.v3.PathDeleteFlag;

public interface MbtoolInterface {
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
    void fileChmod(int id, int mode) throws IOException, MbtoolException, MbtoolCommandException;

    /**
     * Close an opened file.
     *
     * @param id File ID
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    void fileClose(int id) throws IOException, MbtoolException, MbtoolCommandException;

    /**
     * Open a file
     *
     * @param path Path to file
     * @param flags Flags (see {@link FileOpenFlag})
     * @param perms File mode (ignored unless {@link FileOpenFlag#CREAT} is provided)
     * @return File ID
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    int fileOpen(String path, short[] flags, int perms) throws IOException, MbtoolException,
            MbtoolCommandException;

    /**
     * Read data from an opened file
     *
     * @param id File ID
     * @param size Bytes to read
     * @return {@link ByteBuffer} containing the data read (possibly fewer bytes than specified)
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @NonNull
    ByteBuffer fileRead(int id, long size) throws IOException, MbtoolException,
            MbtoolCommandException;

    /**
     * Seek an opened file.
     *
     * Logically equivalent to lseek().
     *
     * @param id File ID
     * @param offset Offset
     * @param whence Seek type (see {@link FileSeekWhence})
     * @return New file position
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    long fileSeek(int id, long offset, short whence) throws IOException, MbtoolException,
            MbtoolCommandException;

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
    @NonNull
    StatBuf fileStat(int id) throws IOException, MbtoolException, MbtoolCommandException;

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
    long fileWrite(int id, byte[] data) throws IOException, MbtoolException, MbtoolCommandException;

    /**
     * Get SELinux label of an opened file.
     *
     * @param id File ID
     * @return SELinux label
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @NonNull
    String fileSelinuxGetLabel(int id) throws IOException, MbtoolException, MbtoolCommandException;

    /**
     * Set SELinux label of an opened file.
     *
     * @param id File ID
     * @param label SELinux label
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    void fileSelinuxSetLabel(int id, String label) throws IOException, MbtoolException,
            MbtoolCommandException;

    /**
     * Get version of the mbtool daemon.
     *
     * @return mbtool version
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @NonNull
    Version getVersion() throws IOException, MbtoolException, MbtoolCommandException;

    /**
     * Get list of installed ROMs.
     *
     * NOTE: The list of ROMs will be re-queried and a new array will be returned every time this
     *       method is called. It may be a good idea to cache the results after the initial call.
     *
     * @return Array of {@link RomInformation} objects representing the list of installed ROMs
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @NonNull
    RomInformation[] getInstalledRoms() throws IOException, MbtoolException, MbtoolCommandException;

    /**
     * Get the current ROM ID.
     *
     * @return String containing the current ROM ID
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @NonNull
    String getBootedRomId() throws IOException, MbtoolException, MbtoolCommandException;

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
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @NonNull
    SwitchRomResult switchRom(Context context, String id, boolean forceChecksumsUpdate)
            throws IOException, MbtoolException, MbtoolCommandException;

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
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @NonNull
    SetKernelResult setKernel(Context context, String id) throws IOException, MbtoolException,
            MbtoolCommandException;

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
    void rebootViaFramework(boolean confirm) throws IOException, MbtoolException,
            MbtoolCommandException;

    /**
     * Reboots the device via init.
     *
     * NOTE: May result in an unclean shutdown as Android's init will simply kill all processes,
     * attempt to unmount filesystems, and then reboot.
     *
     * @param arg Reboot argument (eg. "recovery", "download", "bootloader"). Pass "" for a regular
     *            reboot.
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    void rebootViaInit(String arg) throws IOException, MbtoolException, MbtoolCommandException;

    /**
     * Reboots the device via mbtool.
     *
     * NOTE: May result in an unclean shutdown as mbtool will simply kill all processes, attempt to
     * unmount filesystems, and then reboot.
     *
     * @param arg Reboot argument (eg. "recovery", "download", "bootloader"). Pass "" for a regular
     *            reboot.
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    void rebootViaMbtool(String arg) throws IOException, MbtoolException, MbtoolCommandException;

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
    void shutdownViaInit() throws IOException, MbtoolException, MbtoolCommandException;

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
    void shutdownViaMbtool() throws IOException, MbtoolException, MbtoolCommandException;

    /**
     * Copy a file using mbtool.
     *
     * @param source Absolute source path
     * @param target Absolute target path
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    void pathCopy(String source, String target) throws IOException, MbtoolException,
            MbtoolCommandException;

    /**
     * Delete a path using mbtool.
     *
     * @param path Path to delete
     * @param flag {@link PathDeleteFlag}
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    void pathDelete(String path, short flag) throws IOException, MbtoolException,
            MbtoolCommandException;

    /**
     * Chmod a file using mbtool.
     *
     * @param filename Absolute path
     * @param mode Unix permissions number (will be AND'ed with 0777 by mbtool for security reasons)
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    void pathChmod(String filename, int mode) throws IOException, MbtoolException,
            MbtoolCommandException;

    /**
     * Create a directory using mbtool.
     *
     * @param filename Absolute path
     * @param mode Unix permissions number (will be AND'ed with 0777 by mbtool for security reasons)
     * @param recursive Whether to create directories recursively
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    void pathMkdir(String path, int mode, boolean recursive) throws IOException, MbtoolException,
            MbtoolCommandException;

    /**
     * Wipe a ROM.
     *
     * @param romId ROM ID to wipe
     * @param targets List of {@link mbtool.daemon.v3.MbWipeTarget}s indicating the wipe targets
     * @return {@link WipeResult} containing the list of succeeded and failed wipe targets
     * @throws IOException When any socket communication error occurs
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @NonNull
    WipeResult wipeRom(String romId, short[] targets) throws IOException, MbtoolException,
            MbtoolCommandException;

    /**
     * Get package counts for a ROM.
     *
     * @param romId ROM ID
     * @return {@link PackageCounts} object with counts of system packages, updated packages, and
     *         user packages
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    @NonNull
    PackageCounts getPackagesCounts(String romId) throws IOException, MbtoolException,
            MbtoolCommandException;

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
    @NonNull
    String pathSelinuxGetLabel(String path, boolean followSymlinks) throws IOException,
            MbtoolException, MbtoolCommandException;

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
    void pathSelinuxSetLabel(String path, String label, boolean followSymlinks) throws IOException,
            MbtoolException, MbtoolCommandException;

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
    long pathGetDirectorySize(String path, String[] exclusions) throws IOException, MbtoolException,
            MbtoolCommandException;

    /**
     * Run signed executable.
     *
     * @param path Path to executable
     * @param sigPath Path to signature file
     * @param arg0 Desired argv[0] value (or null to use the binary path)
     * @param args Arguments to pass to the executable
     * @param callback Callback for receiving output
     * @return {@link SignedExecCompletion}
     * @throws IOException
     * @throws MbtoolException
     * @throws MbtoolCommandException
     */
    SignedExecCompletion signedExec(String path, String sigPath, String arg0, String[] args,
                                    SignedExecOutputListener callback)
            throws IOException, MbtoolException, MbtoolCommandException;
}
