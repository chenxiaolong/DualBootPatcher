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

package com.github.chenxiaolong.dualbootpatcher

import android.util.Log

import com.stericson.RootShell.RootShell
import com.stericson.RootShell.exceptions.RootDeniedException
import com.stericson.RootShell.execution.Command

import java.io.IOException
import java.util.concurrent.TimeoutException

object CommandUtils {
    private val TAG = CommandUtils::class.java.simpleName

    // Global lock because RootTools' design only supports a single root session
    private val LOCK = Any()

    interface RootOutputListener {
        fun onNewOutputLine(line: String)
    }

    private class RootCommandResult {
        internal var exitCode: Int = 0
        internal var terminationMessage: String? = null
    }

    class RootExecutionException : Exception {
        constructor(detailMessage: String) : super(detailMessage)

        constructor(message: String, cause: Throwable) : super(message, cause)

        constructor(cause: Throwable) : super(cause)
    }

    @Throws(CommandUtils.RootExecutionException::class, RootDeniedException::class)
    fun runRootCommand(listener: RootOutputListener?, commandStr: String): Int {
        synchronized(LOCK) {
            val result = RootCommandResult()

            try {
                Log.v(TAG, "Command: $commandStr")

                val command = object : Command(0, 0, commandStr) {
                    override fun commandOutput(id: Int, line: String) {
                        Log.d(TAG, "Root command output: $line")

                        listener?.onNewOutputLine(line)

                        super.commandOutput(id, line)
                    }

                    override fun commandTerminated(id: Int, reason: String) {
                        synchronized(result) {
                            result.exitCode = -1
                            result.terminationMessage = reason
                            @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN")
                            (result as Object).notify()
                        }

                        super.commandTerminated(id, reason)
                    }

                    override fun commandCompleted(id: Int, exitCode: Int) {
                        synchronized(result) {
                            result.exitCode = exitCode
                            @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN")
                            (result as Object).notify()
                        }

                        super.commandCompleted(id, exitCode)
                    }
                }

                RootShell.getShell(true).add(command)

                synchronized(result) {
                    @Suppress("PLATFORM_CLASS_MAPPED_TO_KOTLIN")
                    (result as Object).wait()
                }

                if (result.exitCode == -1) {
                    throw RootExecutionException(
                            "Process was terminated: ${result.terminationMessage}")
                }

                return result.exitCode
            } catch (e: IOException) {
                throw RootExecutionException("I/O error", e)
            } catch (e: InterruptedException) {
                throw RootExecutionException("Process was interrupted", e)
            } catch (e: TimeoutException) {
                throw RootExecutionException("Process timed out", e)
            }

        }
    }

    @Throws(RootDeniedException::class, CommandUtils.RootExecutionException::class)
    fun runRootCommand(listener: RootOutputListener?, vararg commandArgs: String): Int {
        return runRootCommand(listener, shellQuote(*commandArgs))
    }

    @Throws(RootDeniedException::class, CommandUtils.RootExecutionException::class)
    fun runRootCommand(commandStr: String): Int {
        return runRootCommand(null, commandStr)
    }

    @Throws(RootDeniedException::class, CommandUtils.RootExecutionException::class)
    fun runRootCommand(vararg commandArgs: String): Int {
        return runRootCommand(null, *commandArgs)
    }

    fun shellQuote(vararg args: String): String {
        val command = StringBuilder()
        for (i in args.indices) {
            if (i > 0) {
                command.append(' ')
            }
            command.append(shellQuote(args[i]))
        }
        return command.toString()
    }

    fun shellQuote(text: String): String {
        return "'${text.replace("'", "'\"'\"'")}'"
    }
}
