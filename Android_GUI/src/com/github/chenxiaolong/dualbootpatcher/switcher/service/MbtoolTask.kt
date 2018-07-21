/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.switcher.service

import android.content.Context
import android.net.Uri
import android.os.Build
import android.text.TextUtils
import android.util.Log
import com.github.chenxiaolong.dualbootpatcher.AnsiStuff
import com.github.chenxiaolong.dualbootpatcher.AnsiStuff.Attribute
import com.github.chenxiaolong.dualbootpatcher.AnsiStuff.Color
import com.github.chenxiaolong.dualbootpatcher.FileUtils
import com.github.chenxiaolong.dualbootpatcher.LogUtils
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibC
import com.github.chenxiaolong.dualbootpatcher.nativelib.libmiscstuff.Constants
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherUtils
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolCommandException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SignedExecOutputListener
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.BackupRestoreParams
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.MbtoolAction
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.RomInstallerParams
import mbtool.daemon.v3.PathDeleteFlag
import mbtool.daemon.v3.SignedExecResult
import java.io.File
import java.io.FileNotFoundException
import java.io.IOException
import java.util.ArrayList
import java.util.Arrays
import java.util.EnumSet

class MbtoolTask(
        taskId: Int,
        context: Context,
        private val actions: Array<MbtoolAction>
) : BaseServiceTask(taskId, context), SignedExecOutputListener {
    private val stateLock = Any()
    private var finished: Boolean = false

    private val lines = ArrayList<String>()
    private var attempted = -1
    private var succeeded = -1

    interface MbtoolTaskListener : BaseServiceTask.BaseServiceTaskListener, MbtoolErrorListener {
        fun onMbtoolTasksCompleted(taskId: Int, actions: Array<MbtoolAction>, attempted: Int,
                                   succeeded: Int)

        fun onCommandOutput(taskId: Int, line: String)
    }

    @Throws(IOException::class, MbtoolException::class, MbtoolCommandException::class)
    private fun deleteWithPrejudice(path: String, iface: MbtoolInterface) {
        iface.pathDelete(path, PathDeleteFlag.REMOVE)
    }

    private fun translateEmulatedPath(path: String): String {
        val emuSourcePath = System.getenv("EMULATED_STORAGE_SOURCE")
        val emuTargetPath = System.getenv("EMULATED_STORAGE_TARGET")

        if (emuSourcePath != null && emuTargetPath != null) {
            if (path.startsWith(emuTargetPath)) {
                printBoldText(Color.YELLOW, "Path uses EMULATED_STORAGE_TARGET\n")
                return emuSourcePath + path.substring(emuTargetPath.length)
            }
        }

        return path
    }

    @Throws(IOException::class, MbtoolException::class)
    private fun getPathFromUri(uri: Uri, iface: MbtoolInterface): String? {
        // Get URI from DocumentFile, which will handle the translation of tree URIs to document
        // URIs, which continuing to work with file:// URIs
        val documentUri = FileUtils.getDocumentFile(context, uri).uri

        val pfd = context.contentResolver.openFileDescriptor(documentUri, "r")
        if (pfd == null) {
            printBoldText(Color.RED, "Failed to open: $documentUri")
            return null
        }

        pfd.use {
            val fdSource = "/proc/${LibC.CWrapper.getpid()}/fd/${it.fd}"
            return try {
                iface.pathReadlink(fdSource)
            } catch (e: MbtoolCommandException) {
                printBoldText(Color.RED, "Failed to resolve symlink: $fdSource: ${e.message}\n")
                null
            }
        }
    }

    @Throws(IOException::class, MbtoolCommandException::class, MbtoolException::class)
    private fun performRomInstallation(params: RomInstallerParams, iface: MbtoolInterface): Boolean {
        printSeparator()

        printBoldText(Color.MAGENTA, "Processing action: Flash file\n")
        printBoldText(Color.MAGENTA, "- File URI: ${params.uri}\n")
        printBoldText(Color.MAGENTA, "- File name: ${params.displayName}\n")
        printBoldText(Color.MAGENTA, "- Destination: ${params.romId}\n")

        // Extract mbtool from the zip file
        val zipInstaller = File(context.cacheDir, "rom-installer")
        val zipInstallerSig = File(context.cacheDir, "rom-installer.sig")

        try {
            // An attacker can't exploit this by creating an undeletable malicious file because
            // mbtool will not execute an untrusted binary
            try {
                deleteWithPrejudice(zipInstaller.absolutePath, iface)
                deleteWithPrejudice(zipInstallerSig.absolutePath, iface)
            } catch (e: MbtoolCommandException) {
                if (e.errno != Constants.ENOENT) {
                    throw e
                }
            }

            @Suppress("ConstantConditionIf")
            if (DEBUG_USE_ALTERNATE_MBTOOL) {
                printBoldText(Color.YELLOW, "[DEBUG] Copying alternate mbtool binary: "
                        + "$ALTERNATE_MBTOOL_PATH\n")
                try {
                    File(ALTERNATE_MBTOOL_PATH).copyTo(zipInstaller, overwrite = true)
                } catch (e: IOException) {
                    printBoldText(Color.RED, "Failed to copy alternate mbtool binary\n")
                    return false
                }

                printBoldText(Color.YELLOW, "[DEBUG] Copying alternate mbtool signature: "
                        + "$ALTERNATE_MBTOOL_SIG_PATH\n")
                try {
                    File(ALTERNATE_MBTOOL_SIG_PATH).copyTo(zipInstallerSig, overwrite = true)
                } catch (e: IOException) {
                    printBoldText(Color.RED, "Failed to copy alternate mbtool signature\n")
                    return false
                }

            } else {
                printBoldText(Color.YELLOW, "Extracting mbtool ROM installer from the zip file\n")
                try {
                    FileUtils.zipExtractFile(context, params.uri!!, UPDATE_BINARY,
                            zipInstaller.path)
                } catch (e: IOException) {
                    printBoldText(Color.RED, "Failed to extract update-binary\n")
                    return false
                }

                printBoldText(Color.YELLOW, "Extracting mbtool signature from the zip file\n")
                try {
                    FileUtils.zipExtractFile(context, params.uri!!, UPDATE_BINARY_SIG,
                            zipInstallerSig.path)
                } catch (e: IOException) {
                    printBoldText(Color.RED, "Failed to extract update-binary.sig\n")
                    return false
                }
            }

            val zipPath = getPathFromUri(params.uri!!, iface) ?: return false

            val argsList = ArrayList<String>()
            argsList.add("--romid")
            argsList.add(params.romId!!)
            argsList.add(translateEmulatedPath(zipPath))

            if (params.skipMounts) {
                argsList.add("--skip-mount")
            }
            if (params.allowOverwrite) {
                argsList.add("--allow-overwrite")
            }

            val args = argsList.toTypedArray()

            printBoldText(Color.YELLOW, "Running rom-installer with arguments: [" +
                    "${args.joinToString(", ")}]\n")

            val completion = iface.signedExec(
                    zipInstaller.absolutePath, zipInstallerSig.absolutePath,
                    "rom-installer", args, this)

            when (completion.result) {
                SignedExecResult.PROCESS_EXITED -> {
                    printBoldText(if (completion.exitStatus == 0) Color.GREEN else Color.RED,
                            "\nCommand returned: ${completion.exitStatus}\n")
                    return completion.exitStatus == 0
                }
                SignedExecResult.PROCESS_KILLED_BY_SIGNAL -> {
                    printBoldText(Color.RED, "\nProcess killed by signal: " +
                            "${completion.termSig}\n")
                    return false
                }
                SignedExecResult.INVALID_SIGNATURE -> {
                    printBoldText(Color.RED, "\nThe mbtool binary has an invalid signature." +
                            " This can happen if an unofficial app was used to patch the file or" +
                            " if the zip was maliciously modified. The file was NOT flashed.\n")
                    return false
                }
                SignedExecResult.OTHER_ERROR -> {
                    printBoldText(Color.RED, "\nError: ${completion.errorMsg}\n")
                    return false
                }
                else -> {
                    printBoldText(Color.RED, "\nError: ${completion.errorMsg}\n")
                    return false
                }
            }
        } finally {
            // Clean up installer files
            try {
                deleteWithPrejudice(zipInstaller.absolutePath, iface)
                deleteWithPrejudice(zipInstallerSig.absolutePath, iface)
            } catch (e: Exception) {
                if (e is MbtoolCommandException && e.errno != Constants.ENOENT) {
                    Log.e(TAG, "Failed to clean up", e)
                }
            }
        }
    }

    @Throws(IOException::class, MbtoolCommandException::class, MbtoolException::class)
    private fun performBackupRestore(params: BackupRestoreParams, iface: MbtoolInterface): Boolean {
        printSeparator()

        when (params.action) {
            BackupRestoreParams.Action.BACKUP -> printBoldText(Color.MAGENTA, "Backup:\n")
            BackupRestoreParams.Action.RESTORE -> printBoldText(Color.MAGENTA, "Restore:\n")
        }

        printBoldText(Color.MAGENTA, "- ROM ID: ${params.romId}\n")
        printBoldText(Color.MAGENTA, "- Targets: ${Arrays.toString(params.targets)}\n")
        printBoldText(Color.MAGENTA, "- Backup name: ${params.backupName}\n")
        printBoldText(Color.MAGENTA, "- Backup dir URI: ${params.backupDirUri}\n")
        printBoldText(Color.MAGENTA, "- Force/overwrite: ${params.force}\n")

        printSeparator()

        printBoldText(Color.YELLOW, "Extracting patcher data archive\n")
        PatcherUtils.extractPatcher(context)

        printSeparator()

        val backupDir = try {
            getPathFromUri(params.backupDirUri!!, iface) ?: return false
        } catch (e: FileNotFoundException) {
            printBoldText(Color.RED, "Backup directory does not exist: ${params.backupDirUri}\n")
            return false
        }

        @Suppress("DEPRECATION")
        val abi = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            Build.SUPPORTED_ABIS[0]
        } else {
            Build.CPU_ABI
        }

        val mbtoolRecovery = File(PatcherUtils.getTargetDirectory(context).toString() +
                File.separator + "binaries" +
                File.separator + "android" +
                File.separator + abi +
                File.separator + "mbtool_recovery")
        val mbtoolRecoverySig = File(PatcherUtils.getTargetDirectory(context).toString() +
                File.separator + "binaries" +
                File.separator + "android" +
                File.separator + abi +
                File.separator + "mbtool_recovery.sig")

        val argv0 = when (params.action) {
            BackupRestoreParams.Action.BACKUP -> "backup"
            BackupRestoreParams.Action.RESTORE -> "restore"
            else -> throw IllegalStateException("Invalid action: ${params.action}")
        }

        val args = ArrayList<String>()
        args.add("-r")
        args.add(params.romId!!)
        args.add("-t")
        args.add(params.targets!!.joinToString(","))
        args.add("-d")
        args.add(translateEmulatedPath(backupDir) + File.separator + params.backupName)
        if (params.force) {
            args.add("-f")
        }

        val argsArray = args.toTypedArray()

        printBoldText(Color.YELLOW, "Running $argv0 with arguments: [" +
                "${TextUtils.join(", ", args)}]\n")

        val completion = iface.signedExec(
                mbtoolRecovery.absolutePath, mbtoolRecoverySig.absolutePath,
                argv0, argsArray, this)

        when (completion.result) {
            SignedExecResult.PROCESS_EXITED -> {
                printBoldText(if (completion.exitStatus == 0) Color.GREEN else Color.RED,
                        "\nCommand returned: ${completion.exitStatus}\n")
                return completion.exitStatus == 0
            }
            SignedExecResult.PROCESS_KILLED_BY_SIGNAL -> {
                printBoldText(Color.RED, "\nProcess killed by signal: ${completion.termSig}\n")
                return false
            }
            SignedExecResult.INVALID_SIGNATURE -> {
                printBoldText(Color.RED, "\nThe mbtool binary has an invalid signature. This" +
                        " means DualBootPatcher's data files have been maliciously modified" +
                        " by somebody or another app. The backup/restore process has been" +
                        " cancelled.\n")
                return false
            }
            SignedExecResult.OTHER_ERROR -> {
                printBoldText(Color.RED, "\nError: ${completion.errorMsg}\n")
                return false
            }
            else -> {
                printBoldText(Color.RED, "\nError: ${completion.errorMsg}\n")
                return false
            }
        }
    }

    public override fun execute() {
        var succeeded = 0
        var attempted = 0

        try {
            printBoldText(Color.YELLOW, "Connecting to mbtool...")

            MbtoolConnection(context).use { conn ->
                val iface = conn.`interface`!!

                printBoldText(Color.YELLOW, " connected\n")

                for (action in actions) {
                    attempted++

                    val result = when (action.type) {
                        MbtoolAction.Type.ROM_INSTALLER ->
                            performRomInstallation(action.romInstallerParams!!, iface)
                        MbtoolAction.Type.BACKUP_RESTORE ->
                            performBackupRestore(action.backupRestoreParams!!, iface)
                        else -> throw IllegalStateException("Invalid action type: ${action.type}")
                    }

                    if (result) {
                        succeeded++
                    } else {
                        break
                    }
                }
            }
        } catch (e: IOException) {
            Log.e(TAG, "mbtool communication error", e)
            printBoldText(Color.RED, "mbtool connection error: ${e.message}\n")
        } catch (e: MbtoolException) {
            Log.e(TAG, "mbtool error", e)
            printBoldText(Color.RED, "mbtool error: ${e.message}\n")
            sendOnMbtoolError(e.reason)
        } catch (e: MbtoolCommandException) {
            Log.w(TAG, "mbtool command error", e)
            printBoldText(Color.RED, "mbtool command error: ${e.message}\n")
        } finally {
            printSeparator()

            val color = if (attempted == actions.size && succeeded == actions.size) {
                Color.GREEN
            } else {
                Color.RED
            }

            printBoldText(color, "Completed:\n")
            printBoldText(color, "$attempted actions attempted\n")
            printBoldText(color, "$succeeded actions succeeded\n")
            printBoldText(color, "${actions.size - attempted} actions skipped\n")

            synchronized(stateLock) {
                this.attempted = attempted
                this.succeeded = succeeded
                sendTasksCompleted()
                finished = true
            }

            LogUtils.dump("mbtool-action.log")
        }
    }

    override fun onOutputLine(line: String) {
        onCommandOutput(line)
    }

    private fun onCommandOutput(line: String) {
        synchronized(stateLock) {
            lines.add(line)
            sendOnCommandOutput(line)
        }
    }

    private fun printBoldText(color: Color, text: String) {
        onCommandOutput(
                AnsiStuff.format(text, EnumSet.of(color), null, EnumSet.of(Attribute.BOLD)))
    }

    private fun printSeparator() {
        printBoldText(Color.WHITE, "-".repeat(16) + "\n")
    }

    override fun onListenerAdded(listener: BaseServiceTask.BaseServiceTaskListener) {
        synchronized(stateLock) {
            super.onListenerAdded(listener)

            for (line in lines) {
                sendOnCommandOutput(line)
            }
            if (finished) {
                sendTasksCompleted()
            }
        }
    }

    private fun sendOnCommandOutput(line: String) {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as MbtoolTaskListener).onCommandOutput(taskId, line)
            }
        })
    }

    private fun sendTasksCompleted() {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as MbtoolTaskListener).onMbtoolTasksCompleted(
                        taskId, actions, attempted, succeeded)
            }
        })
    }

    private fun sendOnMbtoolError(reason: Reason) {
        forEachListener(object : BaseServiceTask.CallbackRunnable {
            override fun call(listener: BaseServiceTask.BaseServiceTaskListener) {
                (listener as MbtoolTaskListener).onMbtoolConnectionFailed(taskId, reason)
            }
        })
    }

    companion object {
        private val TAG = MbtoolTask::class.java.simpleName

        private const val DEBUG_USE_ALTERNATE_MBTOOL = false
        private const val ALTERNATE_MBTOOL_PATH = "/data/local/tmp/mbtool_recovery"
        private const val ALTERNATE_MBTOOL_SIG_PATH = "$ALTERNATE_MBTOOL_PATH.sig"

        private const val UPDATE_BINARY = "META-INF/com/google/android/update-binary"
        private const val UPDATE_BINARY_SIG = "$UPDATE_BINARY.sig"
    }
}
