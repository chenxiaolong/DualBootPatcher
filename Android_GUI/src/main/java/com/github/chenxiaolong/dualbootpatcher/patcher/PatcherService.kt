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

package com.github.chenxiaolong.dualbootpatcher.patcher

import android.content.ContentResolver
import android.net.Uri
import android.provider.OpenableColumns
import android.util.Log
import android.util.SparseArray
import com.github.chenxiaolong.dualbootpatcher.BuildConfig
import com.github.chenxiaolong.dualbootpatcher.LogUtils
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbCommon
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.Device
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.FileInfo
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.Patcher
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.Patcher.ProgressListener
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbPatcher.PatcherConfig
import java.io.FileNotFoundException
import java.io.IOException
import java.lang.ref.WeakReference
import java.util.ArrayList
import java.util.concurrent.atomic.AtomicBoolean
import java.util.concurrent.atomic.AtomicInteger
import java.util.concurrent.atomic.AtomicLong
import java.util.concurrent.atomic.AtomicReference
import java.util.concurrent.locks.ReentrantReadWriteLock
import kotlin.concurrent.read
import kotlin.concurrent.write

class PatcherService : ThreadPoolService() {
    /** List of callbacks for receiving events  */
    private val callbacks = ArrayList<PatcherEventListener>()
    /** Read/write lock for callbacks.  */
    private val callbacksLock = ReentrantReadWriteLock()

    /**
     * Get list of file patching task IDs
     *
     * NOTE: This list should correspond with what is shown in the UI.
     *
     * @return Array of file patching task IDs.
     */
    val patchFileTaskIds: IntArray
        get() {
            patcherTasksLock.read {
                val taskIds = IntArray(patcherTasks.size())
                for (i in 0 until patcherTasks.size()) {
                    taskIds[i] = patcherTasks.keyAt(i)
                }
                return taskIds
            }
        }

    /**
     * {@inheritDoc}
     */
    override fun onCreate() {
        super.onCreate()

        // Update WeakReferences in all of our tasks. This is extremely ugly, but we need to
        // preserve our tasks throughout the service lifecycle.
        callbacksLock.write {
            for (i in 0 until patcherTasks.size()) {
                patcherTasks.valueAt(i).service = this
            }
        }

        val prefs = getSharedPreferences("settings", 0)
        var threads = prefs.getInt("parallel_patching_threads", DEFAULT_PATCHING_THREADS)
        if (threads < 1) {
            threads = DEFAULT_PATCHING_THREADS
        }

        addThreadPool(THREAD_POOL_DEFAULT, THREAD_POOL_DEFAULT_THREADS)
        addThreadPool(THREAD_POOL_PATCHING, threads)
    }

    fun registerCallback(callback: PatcherEventListener?) {
        if (callback == null) {
            Log.w(TAG, "Tried to register null callback!")
            return
        }

        callbacksLock.write {
            callbacks.add(callback)
        }
    }

    fun unregisterCallback(callback: PatcherEventListener?) {
        if (callback == null) {
            Log.w(TAG, "Tried to unregister null callback!")
            return
        }

        callbacksLock.write {
            if (!callbacks.remove(callback)) {
                Log.w(TAG, "Callback was never registered: $callback")
            }
        }
    }

    private fun executeAllCallbacks(runnable: CallbackRunnable) {
        callbacksLock.read {
            for (cb in callbacks) {
                runnable.call(cb)
            }
        }
    }

    private interface CallbackRunnable {
        fun call(callback: PatcherEventListener)
    }

    // Patcher helper methods

    private fun addTask(taskId: Int, task: PatchFileTask) {
        patcherTasksLock.write {
            patcherTasks.put(taskId, task)
        }
    }

    private fun getTask(taskId: Int): PatchFileTask {
        patcherTasksLock.read {
            return patcherTasks[taskId]!!
        }
    }

    private fun removeTask(taskId: Int): PatchFileTask? {
        patcherTasksLock.write {
            val task = patcherTasks[taskId]
            patcherTasks.remove(taskId)
            return task
        }
    }

    /**
     * Asynchronously initialize the patcher
     *
     * This will:
     * - Extract the data archive
     * - Clean up old data directories
     *
     * After the task finishes, [PatcherEventListener.onPatcherInitialized] will be called
     * on all registered callbacks.
     */
    fun initializePatcher() {
        val task = InitializePatcherTask(this)
        enqueueOperation(THREAD_POOL_DEFAULT, task)
    }

    /**
     * Add file patching task
     *
     * The task does not execute until [.startPatching] is called.
     *
     * @param patcherId libmbpatcher patcher ID
     * @param inputUri URI for input file to patch
     * @param outputUri URI for output file
     * @param device Target device
     * @param romId Target ROM ID
     * @return Task ID for the new task
     */
    fun addPatchFileTask(patcherId: String, inputUri: Uri, outputUri: Uri, displayName: String,
                         device: Device, romId: String): Int {
        val taskId = patcherNewTaskId.getAndIncrement()
        addTask(taskId, PatchFileTask(
                this,
                taskId,
                patcherId,
                inputUri,
                outputUri,
                displayName,
                device,
                romId
        ))
        return taskId
    }

    /**
     * Remove file patching task
     *
     * @param taskId Task ID
     * @return Whether the task was removed
     */
    fun removePatchFileTask(taskId: Int): Boolean {
        cancelPatching(taskId)
        val task = removeTask(taskId)
        return task != null
    }

    /**
     * Asynchronously start patching a file
     *
     * During patching, the following callback methods may be called:
     * - [PatcherEventListener.onPatcherUpdateDetails] : Called when libmbpatcher
     * reports a single-line status text item (usually the file inside the archive being
     * processed)
     * - [PatcherEventListener.onPatcherUpdateProgress] : Called when libmbpatcher
     * reports the current progress value and the maximum progress value. Make no assumptions
     * about the current and maximum progress values. It is not guaranteed that current <= maximum
     * or that they are positive.
     * - [PatcherEventListener.onPatcherUpdateFilesProgress] : Called when
     * libmbpatcher reports the current progress and maximum progress in terms of the number of files
     * within the archive that have been processed. Make no assumptions about the current and
     * maximum progress values. It is not guaranteed that current <= maximum or that they are
     * positive.
     *
     * When the patching is finished,
     * [PatcherEventListener.onPatcherFinished] will be
     * called.
     *
     * @param taskId Task ID
     */
    fun startPatching(taskId: Int) {
        val task = getTask(taskId)
        task.state.set(PatchFileState.PENDING)
        enqueueOperation(THREAD_POOL_PATCHING, task)
    }

    /**
     * Cancel file patching
     *
     * This method will attempt to cancel a patching operation in progress. The task will only be
     * cancelled if the corresponding libmbpatcher patcher respects the cancelled flag and stops
     * when it is set. If a task has been cancelled,
     * [PatcherEventListener.onPatcherFinished] will be
     * called in the same manner as described in [.startPatching]. The returned error
     * code may or may not specify that the task has been cancelled.
     *
     * @param taskId Task ID
     */
    fun cancelPatching(taskId: Int) {
        val task = getTask(taskId)
        cancelOperation(THREAD_POOL_PATCHING, task)
        task.cancel()
    }

    private fun enforceQueuedState(task: PatchFileTask) {
        val state = task.state.get()
        if (state !== PatchFileState.QUEUED) {
            throw IllegalStateException("Cannot change task properties in ${state.name} state")
        }
    }

    fun getPatcherId(taskId: Int): String {
        val task = getTask(taskId)
        return task.patcherId
    }

    fun getInputUri(taskId: Int): Uri {
        val task = getTask(taskId)
        return task.inputUri
    }

    fun getOutputUri(taskId: Int): Uri {
        val task = getTask(taskId)
        return task.outputUri
    }

    fun getDisplayName(taskId: Int): String {
        val task = getTask(taskId)
        return task.displayName
    }

    fun setDevice(taskId: Int, device: Device) {
        val task = getTask(taskId)
        enforceQueuedState(task)
        task.device = device
    }

    fun getDevice(taskId: Int): Device? {
        val task = getTask(taskId)
        return task.device
    }

    fun setRomId(taskId: Int, romId: String) {
        val task = getTask(taskId)
        enforceQueuedState(task)
        task.romId = romId
    }

    fun getRomId(taskId: Int): String? {
        val task = getTask(taskId)
        return task.romId
    }

    fun getState(taskId: Int): PatchFileState {
        val task = getTask(taskId)
        return task.state.get()
    }

    fun getDetails(taskId: Int): String? {
        val task = getTask(taskId)
        return task.details.get()
    }

    fun getCurrentBytes(taskId: Int): Long {
        val task = getTask(taskId)
        return task.bytes.get()
    }

    fun getMaximumBytes(taskId: Int): Long {
        val task = getTask(taskId)
        return task.maxBytes.get()
    }

    fun getCurrentFiles(taskId: Int): Long {
        val task = getTask(taskId)
        return task.files.get()
    }

    fun getMaximumFiles(taskId: Int): Long {
        val task = getTask(taskId)
        return task.maxFiles.get()
    }

    fun isSuccessful(taskId: Int): Boolean {
        val task = getTask(taskId)
        return task.successful.get()
    }

    fun getErrorCode(taskId: Int): Int {
        val task = getTask(taskId)
        return task.errorCode.get()
    }

    // Patcher event dispatch methods

    interface PatcherEventListener {
        fun onPatcherInitialized()

        fun onPatcherUpdateDetails(taskId: Int, details: String)

        fun onPatcherUpdateProgress(taskId: Int, bytes: Long, maxBytes: Long)

        fun onPatcherUpdateFilesProgress(taskId: Int, files: Long, maxFiles: Long)

        fun onPatcherStarted(taskId: Int)

        fun onPatcherFinished(taskId: Int, state: PatchFileState, ret: Boolean, errorCode: Int)
    }

    private fun onPatcherInitialized() {
        executeAllCallbacks(object : CallbackRunnable {
            override fun call(callback: PatcherEventListener) {
                callback.onPatcherInitialized()
            }
        })
    }

    private fun onPatcherUpdateDetails(taskId: Int, details: String) {
        executeAllCallbacks(object : CallbackRunnable {
            override fun call(callback: PatcherEventListener) {
                callback.onPatcherUpdateDetails(taskId, details)
            }
        })
    }

    private fun onPatcherUpdateProgress(taskId: Int, bytes: Long, maxBytes: Long) {
        executeAllCallbacks(object : CallbackRunnable {
            override fun call(callback: PatcherEventListener) {
                callback.onPatcherUpdateProgress(taskId, bytes, maxBytes)
            }
        })
    }

    private fun onPatcherUpdateFilesProgress(taskId: Int, files: Long, maxFiles: Long) {
        executeAllCallbacks(object : CallbackRunnable {
            override fun call(callback: PatcherEventListener) {
                callback.onPatcherUpdateFilesProgress(taskId, files, maxFiles)
            }
        })
    }

    private fun onPatcherStarted(taskId: Int) {
        executeAllCallbacks(object : CallbackRunnable {
            override fun call(callback: PatcherEventListener) {
                callback.onPatcherStarted(taskId)
            }
        })
    }

    private fun onPatcherFinished(taskId: Int, state: PatchFileState, ret: Boolean,
                                  errorCode: Int) {
        executeAllCallbacks(object : CallbackRunnable {
            override fun call(callback: PatcherEventListener) {
                callback.onPatcherFinished(taskId, state, ret, errorCode)
            }
        })
    }

    // Patcher task

    private abstract class BaseTask(service: PatcherService) : Runnable {
        /**
         * This is set in [.onCreate]. The service should never go away while the task is
         * running.
         */
        private var _service: WeakReference<PatcherService?>

        init {
            _service = WeakReference(service)
        }

        var service: PatcherService?
            get() = _service.get()
            set(service) {
                _service = WeakReference(service)
            }
    }

    private class PatchFileTask(
            service: PatcherService,
            /** Task ID  */
            private val taskId: Int,
            /** Patcher ID for creating [.patcher]  */
            internal val patcherId: String,
            /** URI of input file  */
            internal val inputUri: Uri,
            /** URI of output file  */
            internal val outputUri: Uri,
            /** Display name  */
            internal val displayName: String,
            /** Target [Device]  */
            internal var device: Device,
            /** Target ROM ID  */
            internal var romId: String
    ) : BaseTask(service), ProgressListener {
        /** libmbpatcher [PatcherConfig] object  */
        private var pc: PatcherConfig? = null
        /** libmbpatcher [Patcher] object  */
        private var patcher: Patcher? = null
        /** Whether this task has already been executed  */
        private var executed: Boolean = false

        // State information

        /** Patching state  */
        internal var state = AtomicReference(PatchFileState.QUEUED)
        /** Whether the patching was cancelled  */
        internal var cancelled = AtomicBoolean(false)
        /** Details text  */
        internal var details = AtomicReference<String>()
        /** Current bytes processed  */
        internal var bytes = AtomicLong(0)
        /** Maximum bytes  */
        internal var maxBytes = AtomicLong(0)
        /** Current files processed  */
        internal var files = AtomicLong(0)
        /** Maximum files  */
        internal var maxFiles = AtomicLong(0)

        // Completion information

        /** Whether patching was successful  */
        internal var successful = AtomicBoolean(false)
        /** Error code if patching failed  */
        internal var errorCode = AtomicInteger(0)

        fun cancel() {
            // If the file was patching, then it should be considered cancelled
            if (executed) {
                cancelled.set(true)
            }

            synchronized(this) {
                if (patcher != null) {
                    patcher!!.cancelPatching()
                }
            }

            // If the file has not started patching yet, allow it to be patched later
            state.compareAndSet(PatchFileState.PENDING, PatchFileState.QUEUED)
        }

        override fun run() {
            if (executed) {
                throw IllegalStateException("Task $taskId has already been executed!")
            } else {
                executed = true
            }

            if (cancelled.get()) {
                throw IllegalStateException("Task $taskId tried to run, but it has already been " +
                        "cancelled")
            }

            state.set(PatchFileState.IN_PROGRESS)
            service!!.onPatcherStarted(taskId)

            // Create patcher instance
            synchronized(this) {
                pc = PatcherUtils.newPatcherConfig(service!!)
                patcher = pc!!.createPatcher(patcherId)
            }

            val cr = service!!.contentResolver

            val inputName = queryDisplayName(cr, inputUri)
            val outputName = queryDisplayName(cr, outputUri)

            Log.d(TAG, "Android GUI version: ${BuildConfig.VERSION_NAME}")
            Log.d(TAG, "Library version: ${LibMbCommon.version} (${LibMbCommon.gitVersion})")
            Log.d(TAG, "Patching file:")
            Log.d(TAG, "- Patcher ID:  $patcherId")
            Log.d(TAG, "- Input URI:   $inputUri")
            Log.d(TAG, "- Input name:  $inputName")
            Log.d(TAG, "- Output URI:  $outputUri")
            Log.d(TAG, "- Output name: $outputName")
            Log.d(TAG, "- Device:      ${device.id}")
            Log.d(TAG, "- ROM ID:      $romId")

            // Make sure patcher is extracted first
            PatcherUtils.initializePatcher(service!!)

            val fileInfo = FileInfo()
            try {
                (cr.openFileDescriptor(inputUri, "r")
                        ?: throw IOException("Failed to open input URI")).use { pfdIn ->
                    (cr.openFileDescriptor(outputUri, "w")
                            ?: throw IOException("Failed to open output URI")).use { pfdOut ->
                        Log.d(TAG, "Input file descriptor is: ${pfdIn.fd}")
                        Log.d(TAG, "Output file descriptor is: ${pfdOut.fd}")

                        fileInfo.device = device
                        fileInfo.inputPath = "/proc/self/fd/${pfdIn.fd}"
                        fileInfo.outputPath = "/proc/self/fd/${pfdOut.fd}"
                        fileInfo.romId = romId

                        patcher!!.setFileInfo(fileInfo)

                        val ret = patcher!!.patchFile(this)
                        successful.set(ret)
                        errorCode.set(patcher!!.error)

                        val cancelled = cancelled.get()

                        // Set to complete if the task wasn't cancelled
                        val state = if (cancelled) {
                            PatchFileState.CANCELLED
                        } else {
                            PatchFileState.COMPLETED
                        }
                        this.state.set(state)

                        service!!.onPatcherFinished(taskId, state, ret, patcher!!.error)
                    }
                }
            } catch (e: FileNotFoundException) {
                Log.e(TAG, "Failed to open URI", e)
                state.set(PatchFileState.COMPLETED)
                service!!.onPatcherFinished(taskId, PatchFileState.COMPLETED, false, -1)
            } catch (e: IOException) {
                Log.e(TAG, "I/O error", e)
                state.set(PatchFileState.CANCELLED)
                service!!.onPatcherFinished(taskId, PatchFileState.CANCELLED, false, -1)
            } finally {
                // Ensure we destroy allocated objects on the C++ side
                synchronized(this) {
                    pc!!.destroyPatcher(patcher!!)
                    patcher = null
                    pc!!.destroy()
                    pc = null
                }
                fileInfo.destroy()

                // Save log
                LogUtils.dump("patch-file.log")
            }
        }

        private fun queryDisplayName(cr: ContentResolver, uri: Uri?): String? {
            return cr.query(uri!!, null, null, null, null, null)?.use {
                if (it.moveToFirst()) {
                    val nameIndex = it.getColumnIndex(OpenableColumns.DISPLAY_NAME)
                    return it.getString(nameIndex)
                }
                return null
            }
        }

        override fun onProgressUpdated(bytes: Long, maxBytes: Long) {
            this.bytes.set(bytes)
            this.maxBytes.set(maxBytes)
            service!!.onPatcherUpdateProgress(taskId, bytes, maxBytes)
        }

        override fun onFilesUpdated(files: Long, maxFiles: Long) {
            this.files.set(files)
            this.maxFiles.set(maxFiles)
            service!!.onPatcherUpdateFilesProgress(taskId, files, maxFiles)
        }

        override fun onDetailsUpdated(text: String) {
            details.set(text)
            service!!.onPatcherUpdateDetails(taskId, text)
        }
    }

    private class InitializePatcherTask(service: PatcherService) : BaseTask(service) {
        override fun run() {
            PatcherUtils.initializePatcher(service!!)
            service!!.onPatcherInitialized()
        }
    }

    companion object {
        private val TAG = PatcherService::class.java.simpleName

        const val DEFAULT_PATCHING_THREADS = 2

        private const val THREAD_POOL_DEFAULT = "default"
        private const val THREAD_POOL_PATCHING = "patching"
        private const val THREAD_POOL_DEFAULT_THREADS = 2

        // TODO: Won't survive out-of-memory app restart
        private val patcherTasksLock = ReentrantReadWriteLock()
        private val patcherTasks = SparseArray<PatchFileTask>()
        private val patcherNewTaskId = AtomicInteger(0)
    }
}
