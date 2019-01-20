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

package com.github.chenxiaolong.dualbootpatcher.socket

import android.content.Context
import android.net.LocalSocket
import android.net.LocalSocketAddress
import android.net.LocalSocketAddress.Namespace
import android.os.Build
import android.util.Log
import com.github.chenxiaolong.dualbootpatcher.CommandUtils
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.RootExecutionException
import com.github.chenxiaolong.dualbootpatcher.ThreadUtils
import com.github.chenxiaolong.dualbootpatcher.Version
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils.Feature
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterfaceV3
import com.stericson.RootShell.exceptions.RootDeniedException
import mbtool.daemon.v3.SignedExecResult
import java.io.Closeable
import java.io.File
import java.io.IOException
import java.io.InputStream
import java.io.OutputStream

class MbtoolConnection @Throws(IOException::class, MbtoolException::class)
constructor(context: Context) : Closeable {
    /** mbtool socket  */
    private var socket: LocalSocket? = null
    /** Socket's input stream  */
    private var socketIS: InputStream? = null
    /** Socket's output stream  */
    private var socketOS: OutputStream? = null

    /** mbtool interface  */
    var `interface`: MbtoolInterface? = null
        private set

    @Throws(IOException::class, MbtoolException::class)
    private fun connect() {
        // Try connecting to the socket
        socket = initConnectToSocket()
        socketIS = socket!!.inputStream
        socketOS = socket!!.outputStream

        // mbtool will immediately send a message telling us whether the app signature is allowed
        // or denied
        initVerifyCredentials(socketIS!!)

        // Request interface version
        initRequestInterface(socketIS!!, socketOS!!, PROTOCOL_VERSION)

        // Set up interface
        `interface` = createInterface(socketIS!!, socketOS!!, PROTOCOL_VERSION)

        // Check version
        initVerifyVersion(`interface`!!, MbtoolUtils.getMinimumRequiredVersion(Feature.DAEMON))
    }

    init {
        ThreadUtils.enforceExecutionOnNonMainThread()

        try {
            connect()
        } catch (e: MbtoolException) {
            if (e.reason !== Reason.INTERFACE_NOT_SUPPORTED
                    && e.reason !== Reason.VERSION_TOO_OLD) {
                throw e
            }

            // Try to update mbtool if needed
            replaceViaSignedExec(context)

            // Reconnect
            connect()
        }
    }

    @Throws(IOException::class)
    override fun close() {
        // Streams are closed when the socket is closed
        socketOS = null
        socketIS = null

        socket?.close()
        socket = null
    }

    companion object {
        private val TAG = MbtoolConnection::class.java.simpleName

        /** mbtool daemon's abstract socket address  */
        private const val SOCKET_ADDRESS = "mbtool.daemon"

        /** Protocol version to use  */
        private const val PROTOCOL_VERSION = 3

        /** Minimum protocol version for signed exec  */
        private const val SIGNED_EXEC_MIN_PROTOCOL_VERSION = 3
        /** Maximum protocol version for signed exec  */
        private const val SIGNED_EXEC_MAX_PROTOCOL_VERSION = 3

        // Handshake responses

        /**
         * Handshake response indicating that the app signature has been verified and the connection
         * is allowed.
         */
        private const val HANDSHAKE_RESPONSE_ALLOW = "ALLOW"
        /**
         * Handshake response indicating the the app signature is invalid and the negotiation has
         * been denied. The daemon will terminate the connection after sending this response.
         */
        private const val HANDSHAKE_RESPONSE_DENY = "DENY"
        /**
         * Handshake response indicating that the requested protocol version is supported.
         */
        private const val HANDSHAKE_RESPONSE_OK = "OK"
        /**
         * Handshake response indicating that the requested protocol version is unsupported. The
         * daemon will terminate the connection after sending this response.
         */
        private const val HANDSHAKE_RESPONSE_UNSUPPORTED = "UNSUPPORTED"

        // Paths

        private const val TMPFS_MOUNTPOINT = "/mnt/mb_tmp"
        private const val MBTOOL_TMPFS_PATH = "$TMPFS_MOUNTPOINT/mbtool"
        private const val MBTOOL_ROOTFS_PATH = "/mbtool"

        /**
         * Bind to mbtool socket
         *
         * @throws MbtoolException
         */
        @Throws(MbtoolException::class)
        private fun initConnectToSocket(): LocalSocket {
            try {
                val socket = LocalSocket()
                socket.connect(LocalSocketAddress(SOCKET_ADDRESS, Namespace.ABSTRACT))
                return socket
            } catch (e: IOException) {
                Log.e(TAG, "Could not connect to mbtool socket", e)
                throw MbtoolException(Reason.DAEMON_NOT_RUNNING, e)
            }

        }

        /**
         * Check if mbtool rejected the connection due to the signature check failing
         *
         * @throws IOException
         * @throws MbtoolException
         */
        @Throws(IOException::class, MbtoolException::class)
        private fun initVerifyCredentials(`is`: InputStream) {
            val response = SocketUtils.readString(`is`)
            if (HANDSHAKE_RESPONSE_DENY == response) {
                Log.e(TAG, "mbtool connection was denied. This app might have been tampered with!")
                throw MbtoolException(Reason.SIGNATURE_CHECK_FAIL,
                        "Access was explicitly denied by the daemon")
            } else if (HANDSHAKE_RESPONSE_ALLOW != response) {
                throw MbtoolException(Reason.PROTOCOL_ERROR, "Unexpected reply: $response")
            }
        }

        /**
         * Request protocol version from mbtool
         *
         * @throws IOException
         * @throws MbtoolException
         */
        @Throws(IOException::class, MbtoolException::class)
        private fun initRequestInterface(`is`: InputStream, os: OutputStream, version: Int) {
            SocketUtils.writeInt32(os, version)
            val response = SocketUtils.readString(`is`)
            if (HANDSHAKE_RESPONSE_UNSUPPORTED == response) {
                throw MbtoolException(Reason.INTERFACE_NOT_SUPPORTED,
                        "Daemon does not support protocol version: $version")
            } else if (HANDSHAKE_RESPONSE_OK != response) {
                throw MbtoolException(Reason.PROTOCOL_ERROR, "Unexpected reply: $response")
            }
        }

        /**
         * Check that the minimum mbtool version is satisfied
         *
         * @throws IOException
         * @throws MbtoolException
         */
        @Throws(IOException::class, MbtoolException::class)
        private fun initVerifyVersion(iface: MbtoolInterface, minVersion: Version) {
            val version: Version
            try {
                version = iface.getVersion()
            } catch (e: MbtoolCommandException) {
                Log.w(TAG, "Failed to get mbtool version. Assuming to be old version", e)
                throw MbtoolException(Reason.VERSION_TOO_OLD, "Could not get mbtool version")
            }

            Log.v(TAG, "mbtool version: $version")
            Log.v(TAG, "minimum version: $minVersion")

            // Ensure that the version is newer than the minimum required version
            if (version < minVersion) {
                throw MbtoolException(Reason.VERSION_TOO_OLD,
                        "mbtool version is: $version, minimum needed is: $minVersion")
            }
        }

        private fun createInterface(`is`: InputStream, os: OutputStream, version: Int):
                MbtoolInterface? {
            return when (version) {
                3 -> MbtoolInterfaceV3(`is`, os)
                else -> null
            }
        }

        @Suppress("DEPRECATION")
        private val abi: String
            get() = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
                Build.SUPPORTED_ABIS[0]
            } else {
                Build.CPU_ABI
            }

        @Throws(RootDeniedException::class, RootExecutionException::class)
        private fun runMbtoolDaemon(path: String): Int {
            return CommandUtils.runRootCommand(path, "daemon", "--replace", "--daemonize")
        }

        @Throws(RootDeniedException::class, RootExecutionException::class)
        private fun launchMbtoolFromTmpfs(path: String): Boolean {
            // Mount tmpfs if it doesn't already exist
            if (CommandUtils.runRootCommand("mountpoint", TMPFS_MOUNTPOINT) != 0
                    && (CommandUtils.runRootCommand("mkdir", "-p", TMPFS_MOUNTPOINT) != 0
                            || CommandUtils.runRootCommand("mount", "-t", "tmpfs", "tmpfs", TMPFS_MOUNTPOINT) != 0)) {
                return false
            }

            // Move away old binary if it exists
            CommandUtils.runRootCommand("mv", MBTOOL_TMPFS_PATH, "$MBTOOL_TMPFS_PATH.bak")

            // Copy new executable and execute it
            return (CommandUtils.runRootCommand("cp", path, MBTOOL_TMPFS_PATH) == 0
                    && CommandUtils.runRootCommand("chmod", "755", MBTOOL_TMPFS_PATH) == 0
                    && runMbtoolDaemon(MBTOOL_TMPFS_PATH) == 0)
        }

        @Throws(RootDeniedException::class, RootExecutionException::class)
        private fun launchMbtoolFromRootfs(path: String): Boolean {
            // Make rootfs writable
            if (CommandUtils.runRootCommand("mount", "-o", "remount,rw", "/") != 0) {
                return false
            }

            // Move away old binary if it exists
            CommandUtils.runRootCommand("mv", MBTOOL_ROOTFS_PATH, "$MBTOOL_ROOTFS_PATH.bak")

            // Copy new executable
            val ret = CommandUtils.runRootCommand("cp", path, MBTOOL_ROOTFS_PATH) == 0
                    && CommandUtils.runRootCommand("chmod", "755", MBTOOL_ROOTFS_PATH) == 0

            // Try to make rootfs read-only
            CommandUtils.runRootCommand("mount", "-o", "remount,ro", "/")

            // Run daemon
            return ret && runMbtoolDaemon(MBTOOL_ROOTFS_PATH) == 0
        }

        fun replaceViaRoot(context: Context): Boolean {
            ThreadUtils.enforceExecutionOnNonMainThread()

            PatcherUtils.extractPatcher(context)

            val mbtool = PatcherUtils.getTargetDirectory(context).toString() +
                    File.separator + "binaries" +
                    File.separator + "android" +
                    File.separator + abi +
                    File.separator + "mbtool"

            // We can't run mbtool directly from /data because TouchWiz kernels severely restrict
            // the exec*() family of syscalls for executables that reside in /data.

            return try {
                launchMbtoolFromTmpfs(mbtool) || launchMbtoolFromRootfs(mbtool)
            } catch (e: RootDeniedException) {
                Log.e(TAG, "Root was denied", e)
                false
            } catch (e: RootExecutionException) {
                Log.e(TAG, "Root execution failed", e)
                false
            }
        }

        fun replaceViaSignedExec(context: Context): Boolean {
            ThreadUtils.enforceExecutionOnNonMainThread()

            PatcherUtils.extractPatcher(context)

            val abi = abi

            val mbtool = File(PatcherUtils.getTargetDirectory(context).toString() +
                    File.separator + "binaries" +
                    File.separator + "android" +
                    File.separator + abi +
                    File.separator + "mbtool")
            val mbtoolSig = File(PatcherUtils.getTargetDirectory(context).toString() +
                    File.separator + "binaries" +
                    File.separator + "android" +
                    File.separator + abi +
                    File.separator + "mbtool.sig")

            for (i in SIGNED_EXEC_MAX_PROTOCOL_VERSION downTo SIGNED_EXEC_MIN_PROTOCOL_VERSION) {
                try {
                    // Try connecting to the socket
                    initConnectToSocket().use { socket ->
                        val socketIS = socket.inputStream
                        val socketOS = socket.outputStream

                        // mbtool will immediately send a message telling us whether the app signature is
                        // allowed or denied
                        initVerifyCredentials(socketIS)

                        // Request interface version
                        initRequestInterface(socketIS, socketOS, i)

                        // Create interface
                        val iface = createInterface(socketIS, socketOS, i)
                                ?: throw IllegalStateException("Failed to create interface for version: $i")

                        // Use signed exec to replace mbtool. This purposely sets argv[0] to "mbtool" and
                        // argv[1] to "daemon" instead of just setting argv[0] to "daemon" because --replace
                        // kills processes with cmdlines matching the former case.
                        val completion = iface.signedExec(
                                mbtool.absolutePath, mbtoolSig.absolutePath,
                                "mbtool", arrayOf("daemon", "--replace", "--daemonize"), null)

                        return when (completion.result) {
                            SignedExecResult.PROCESS_EXITED -> {
                                Log.d(TAG, "mbtool signed exec exited with status: ${completion.exitStatus}")
                                completion.exitStatus == 0
                            }
                            SignedExecResult.PROCESS_KILLED_BY_SIGNAL -> {
                                Log.d(TAG, "mbtool signed exec killed by signal: ${completion.termSig}")
                                false
                            }
                            SignedExecResult.INVALID_SIGNATURE -> {
                                Log.d(TAG, "mbtool signed exec failed due to invalid signature")
                                false
                            }
                            SignedExecResult.OTHER_ERROR -> {
                                Log.d(TAG, "mbtool signed exec failed: ${completion.errorMsg}")
                                false
                            }
                            else -> {
                                Log.d(TAG, "mbtool signed exec failed: ${completion.errorMsg}")
                                false
                            }
                        }
                    }
                } catch (e: IOException) {
                    // Keep trying
                    Log.w(TAG, "mbtool connection error", e)
                } catch (e: MbtoolException) {
                    // Keep trying unless the daemon is not running or if the signature check fails
                    Log.w(TAG, "mbtool error", e)
                    if (e.reason === Reason.DAEMON_NOT_RUNNING
                            || e.reason === Reason.SIGNATURE_CHECK_FAIL) {
                        break
                    }
                } catch (e: MbtoolCommandException) {
                    // Keep trying
                    Log.w(TAG, "mbtool command error", e)
                }
            }

            return false
        }
    }
}
