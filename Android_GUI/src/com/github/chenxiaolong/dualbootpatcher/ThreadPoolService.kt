/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import android.annotation.SuppressLint
import android.app.Service
import android.content.Intent
import android.os.Binder
import android.os.IBinder
import android.util.Log
import java.util.*
import java.util.concurrent.LinkedBlockingQueue
import java.util.concurrent.ThreadPoolExecutor
import java.util.concurrent.TimeUnit

@SuppressLint("Registered")
open class ThreadPoolService : Service() {
    /** Thread pool for executing operations  */
    private val threadPools = HashMap<String, ThreadPoolExecutor>()

    private val wrappedRunnables = HashMap<Runnable, Runnable>()

    /** Number of currently running or pending operations  */
    private var numOperations = 0

    private val lock = Any()

    /** Whether there are bound clients  */
    private var isBound = false

    /** Binder instance  */
    private val binder = ThreadPoolServiceBinder()

    /** Local Binder class  */
    inner class ThreadPoolServiceBinder : Binder() {
        val service: ThreadPoolService
            get() = this@ThreadPoolService
    }

    /** Log debug messages if [.DEBUG] is true  */
    private fun log(message: String) {
        @Suppress("ConstantConditionIf")
        if (DEBUG) {
            Log.d(TAG, message)
        }
    }

    /** {@inheritDoc}  */
    override fun onCreate() {
        super.onCreate()
        log("onCreate()")
    }

    /** {@inheritDoc}  */
    override fun onDestroy() {
        super.onDestroy()
        log("onDestroy()")

        // Attempt to stop thread pools. This shouldn't be an issue as there should be no tasks
        // running at this point. The service stops when all clients have unbound from it and there
        // are no pending tasks.
        for ((id, executor) in threadPools) {
            executor.shutdownNow()
            try {
                executor.awaitTermination(60, TimeUnit.SECONDS)
            } catch (e: InterruptedException) {
                Log.e(TAG, "Failed to wait 60 seconds for thread pool termination: $id", e)
            }
        }
    }

    /** {@inheritDoc}  */
    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        log("onStartCommand(intent=$intent, flags=$flags, startId=$startId)")
        return Service.START_STICKY
    }

    /** {@inheritDoc}  */
    override fun onBind(intent: Intent): IBinder? {
        log("onBind(intent=$intent)")
        synchronized(lock) {
            isBound = true
        }
        return binder
    }

    /** {@inheritDoc}  */
    override fun onUnbind(intent: Intent): Boolean {
        log("onUnbind(intent=$intent)")
        synchronized(lock) {
            isBound = false
        }
        attemptToStop()
        // We're don't need to differentiate between bind and rebind
        return false
    }

    /**
     * Add a new thread pool
     *
     * @param id ID for the thread pool
     * @param maxThreads Maximum threads in the thread pool (defaults to number of CPU cores)
     * @throws IllegalArgumentException if maxThreads <= 0 or if the ID already exists
     */
    @JvmOverloads protected fun addThreadPool(id: String, maxThreads: Int = NUMBER_OF_CORES) {
        if (threadPools.containsKey(id)) {
            throw IllegalStateException("Thread pool already exists: $id")
        }

        val executor = ThreadPoolExecutor(maxThreads, maxThreads,
                KEEP_ALIVE_TIME.toLong(), KEEP_ALIVE_TIME_UNITS, LinkedBlockingQueue())
        threadPools[id] = executor
    }

    /**
     * Enqueue operation
     *
     * @param id Thread pool ID
     * @param runnable Task to run in thread pool
     * @throws IllegalArgumentException if the thread pool ID doesn't exist
     */
    protected fun enqueueOperation(id: String, runnable: Runnable) {
        val executor = threadPools[id] ?:
                throw IllegalArgumentException("Thread pool does not exist: $id")

        synchronized(lock) {
            if (wrappedRunnables.containsKey(runnable)) {
                throw IllegalStateException(
                        "Runnable $runnable already queued in thread pool: $id")
            }

            numOperations++
            val wrapped = Runnable {
                try {
                    runnable.run()
                } finally {
                    synchronized(lock) {
                        numOperations--
                        attemptToStop()
                    }
                }
            }
            wrappedRunnables[runnable] = wrapped
            executor.execute(wrapped)
            log("Added runnable $runnable to thread pool: $id")
        }
    }

    /**
     * Cancel operation
     *
     * @param id Thread pool ID
     * @param runnable Task to cancel
     * @throws IllegalArgumentException if the thread pool ID doesn't exist
     */
    protected fun cancelOperation(id: String, runnable: Runnable): Boolean {
        val executor = threadPools[id] ?:
                throw IllegalArgumentException("Thread pool does not exist: $id")

        log("Trying to cancel $runnable in thread pool: $id")

        return synchronized(lock) {
            val wrapped = wrappedRunnables.remove(runnable)
            if (wrapped == null) {
                log("Runnable $runnable does not exist in thread pool: $id")
                return false
            }

            val ret = executor.remove(wrapped)
            if (ret) {
                log("Successfully cancelled $runnable in thread pool: $id")
                numOperations--
                attemptToStop()
            }

            ret
        }
    }

    /**
     * Attempt to stop the service
     *
     * This will not call [.stopSelf] unless there are no tasks running and no clients bound
     */
    private fun attemptToStop() {
        synchronized(lock) {
            log("Attempting to stop service")
            if (numOperations > 0 || isBound) {
                log("Not stopping: # of operations: $numOperations, is bound: $isBound")
                return
            }

            log("Calling stopSelf(): there are no more operations")
            stopSelf()
        }
    }

    companion object {
        private val TAG = ThreadPoolService::class.java.simpleName

        private const val DEBUG = true

        /** Number of cores  */
        private val NUMBER_OF_CORES = Runtime.getRuntime().availableProcessors()
        /** Time an idle thread will wait before it exits  */
        private const val KEEP_ALIVE_TIME = 1
        /** Units of [.KEEP_ALIVE_TIME]  */
        private val KEEP_ALIVE_TIME_UNITS = TimeUnit.SECONDS
    }
}