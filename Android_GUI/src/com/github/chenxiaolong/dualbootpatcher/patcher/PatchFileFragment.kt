/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import android.app.Activity
import android.content.ComponentName
import android.content.ContentResolver
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.net.Uri
import android.os.AsyncTask
import android.os.Bundle
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.support.design.widget.Snackbar
import android.support.v4.app.Fragment
import android.support.v4.content.ContextCompat
import android.support.v7.widget.LinearLayoutManager
import android.support.v7.widget.RecyclerView
import android.support.v7.widget.SimpleItemAnimator
import android.support.v7.widget.helper.ItemTouchHelper
import android.util.Log
import android.util.SparseIntArray
import android.view.LayoutInflater
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager
import android.widget.ProgressBar
import android.widget.TextView
import com.github.chenxiaolong.dualbootpatcher.FileUtils
import com.github.chenxiaolong.dualbootpatcher.FileUtils.UriMetadata
import com.github.chenxiaolong.dualbootpatcher.MenuUtils
import com.github.chenxiaolong.dualbootpatcher.PermissionUtils
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.SnackbarUtils
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog.GenericYesNoDialogListener
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.Device
import com.github.chenxiaolong.dualbootpatcher.patcher.PatchFileItemAdapter.PatchFileItemClickListener
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherOptionsDialog.PatcherOptionsDialogListener
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherService.PatcherEventListener
import com.github.chenxiaolong.dualbootpatcher.views.DragSwipeItemTouchCallback
import com.github.chenxiaolong.dualbootpatcher.views.DragSwipeItemTouchCallback.OnItemMovedOrDismissedListener
import com.github.clans.fab.FloatingActionButton
import com.github.clans.fab.FloatingActionMenu
import java.io.File
import java.util.*

class PatchFileFragment : Fragment(), ServiceConnection, PatcherOptionsDialogListener,
        OnItemMovedOrDismissedListener, PatchFileItemClickListener, GenericYesNoDialogListener {
    /** Whether we should show the progress bar (true by default for obvious reasons)  */
    private var showingProgress = true

    /** Main files list  */
    private lateinit var recycler: RecyclerView
    /** FAB  */
    private lateinit var fab: FloatingActionMenu
    private lateinit var fabAddZip: FloatingActionButton
    private lateinit var fabAddOdin: FloatingActionButton
    /** Loading progress spinner  */
    private lateinit var progressBar: ProgressBar
    /** Add zip message  */
    private lateinit var addZipMessage: TextView

    /** Check icon in the toolbar  */
    private var checkItem: MenuItem? = null
    /** Cancel icon in the toolbar  */
    private var cancelItem: MenuItem? = null

    /** Adapter for the list of files to patch  */
    private lateinit var adapter: PatchFileItemAdapter

    /** Our patcher service  */
    private var service: PatcherService? = null
    /** Callback for events from the service  */
    private val callback = PatcherEventCallback()

    /** Handler for processing events from the service on the UI thread  */
    private val handler = Handler(Looper.getMainLooper())

    /** [Runnable]s to process once the service has been connected  */
    private val execOnConnect = ArrayList<Runnable>()

    /** Whether we're initialized  */
    private var initialized: Boolean = false

    /** List of patcher items (pending, in progress, or complete)  */
    private val items = ArrayList<PatchFileItem>()
    /** Map task IDs to item indexes  */
    private val itemsMap = SparseIntArray()

    /** Item touch callback for dragging and swiping  */
    private lateinit var itemTouchCallback: DragSwipeItemTouchCallback

    /** Selected patcher ID  */
    private var selectedPatcherId: String? = null
    /** Selected input file  */
    private var selectedInputUri: Uri? = null
    /** Selected output file  */
    private var selectedOutputUri: Uri? = null
    /** Selected input file's name  */
    private var selectedInputFileName: String? = null
    /** Selected input file's size  */
    private var selectedInputFileSize: Long = 0
    /** Task ID of selected patcher item  */
    private var selectedTaskId: Int = 0
    /** Target device  */
    private var selectedDevice: Device? = null
    /** Target ROM ID  */
    private var selectedRomId: String? = null
    /** Whether we're querying the URI metadata  */
    private var queryingMetadata: Boolean = false
    /** Task for querying the metadata of URIs  */
    private var queryMetadataTask: GetUriMetadataTask? = null

    /**
     * {@inheritDoc}
     */
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        setHasOptionsMenu(true)
    }

    /**
     * {@inheritDoc}
     */
    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)

        if (savedInstanceState != null) {
            selectedPatcherId = savedInstanceState.getString(EXTRA_SELECTED_PATCHER_ID)
            selectedInputUri = savedInstanceState.getParcelable(EXTRA_SELECTED_INPUT_URI)
            selectedOutputUri = savedInstanceState.getParcelable(EXTRA_SELECTED_OUTPUT_URI)
            selectedInputFileName = savedInstanceState.getString(EXTRA_SELECTED_INPUT_FILE_NAME)
            selectedInputFileSize = savedInstanceState.getLong(EXTRA_SELECTED_INPUT_FILE_SIZE)
            selectedTaskId = savedInstanceState.getInt(EXTRA_SELECTED_TASK_ID)
            selectedDevice = savedInstanceState.getParcelable(EXTRA_SELECTED_DEVICE)
            selectedRomId = savedInstanceState.getString(EXTRA_SELECTED_ROM_ID)
            queryingMetadata = savedInstanceState.getBoolean(EXTRA_QUERYING_METADATA)
        }

        // Initialize UI elements
        recycler = activity!!.findViewById(R.id.files_list)
        fab = activity!!.findViewById(R.id.fab)
        fabAddZip = activity!!.findViewById(R.id.fab_add_flashable_zip)
        fabAddOdin = activity!!.findViewById(R.id.fab_add_odin_image)
        progressBar = activity!!.findViewById(R.id.loading)
        addZipMessage = activity!!.findViewById(R.id.add_zip_message)

        itemTouchCallback = DragSwipeItemTouchCallback(this)
        val itemTouchHelper = ItemTouchHelper(itemTouchCallback)
        itemTouchHelper.attachToRecyclerView(recycler)

        // Disable change animation since we frequently update the progress, which makes the
        // animation very ugly
        val animator = recycler.itemAnimator
        if (animator is SimpleItemAnimator) {
            animator.supportsChangeAnimations = false
        }

        // Set up listener for the FAB
        fabAddZip.setOnClickListener {
            startFileSelection(PatcherUtils.PATCHER_ID_ZIPPATCHER)
            fab.close(true)
        }
        fabAddOdin.setOnClickListener {
            startFileSelection(PatcherUtils.PATCHER_ID_ODINPATCHER)
            fab.close(true)
        }

        // Set up adapter for the files list
        adapter = PatchFileItemAdapter(activity!!, items, this)
        recycler.setHasFixedSize(true)
        recycler.adapter = adapter

        val llm = LinearLayoutManager(activity)
        llm.orientation = LinearLayoutManager.VERTICAL
        recycler.layoutManager = llm

        // Hide FAB initially
        fab.hideMenuButton(false)

        // Show loading progress bar
        updateLoadingStatus()

        // Initialize the patcher once the service is connected
        executeNeedsService(Runnable { service!!.initializePatcher() })

        // NOTE: No further loading should be done here. All initialization should be done in
        // onPatcherLoaded(), which is called once the patcher's data files have been extracted and
        // loaded.
    }

    /**
     * {@inheritDoc}
     */
    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)

        outState.putString(EXTRA_SELECTED_PATCHER_ID, selectedPatcherId)
        outState.putParcelable(EXTRA_SELECTED_INPUT_URI, selectedInputUri)
        outState.putParcelable(EXTRA_SELECTED_OUTPUT_URI, selectedOutputUri)
        outState.putString(EXTRA_SELECTED_INPUT_FILE_NAME, selectedInputFileName)
        outState.putLong(EXTRA_SELECTED_INPUT_FILE_SIZE, selectedInputFileSize)
        outState.putInt(EXTRA_SELECTED_TASK_ID, selectedTaskId)
        outState.putParcelable(EXTRA_SELECTED_DEVICE, selectedDevice)
        outState.putString(EXTRA_SELECTED_ROM_ID, selectedRomId)
        outState.putBoolean(EXTRA_QUERYING_METADATA, queryingMetadata)
    }

    /**
     * {@inheritDoc}
     */
    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?,
                              savedInstanceState: Bundle?): View? {
        return inflater.inflate(R.layout.fragment_patcher, container, false)
    }

    /**
     * {@inheritDoc}
     */
    override fun onCreateOptionsMenu(menu: Menu, inflater: MenuInflater?) {
        inflater!!.inflate(R.menu.actionbar_check_cancel, menu)

        // NOTE: May crash on some versions of Android due to a bug where getActivity() returns null
        // after onAttach() has been called, but before onDetach() has been called. It's similar to
        // this bug report, except it happens with android.app.Fragment:
        // https://code.google.com/p/android/issues/detail?id=67519
        val primary = ContextCompat.getColor(activity!!, R.color.text_color_primary)
        MenuUtils.tintAllMenuIcons(menu, primary)

        checkItem = menu.findItem(R.id.check_item)
        cancelItem = menu.findItem(R.id.cancel_item)

        updateToolbarIcons()

        super.onCreateOptionsMenu(menu, inflater)
    }

    /**
     * {@inheritDoc}
     */
    override fun onOptionsItemSelected(item: MenuItem?): Boolean {
        when (item!!.itemId) {
            R.id.check_item -> {
                executeNeedsService(Runnable {
                    for ((index, patchItem) in items.withIndex()) {
                        if (patchItem.state === PatchFileState.QUEUED) {
                            patchItem.state = PatchFileState.PENDING
                            service!!.startPatching(patchItem.taskId)
                            adapter.notifyItemChanged(index)
                        }
                    }
                })
                return true
            }
            R.id.cancel_item -> {
                executeNeedsService(Runnable {
                    // Cancel the tasks in reverse order since there's a chance that the next task
                    // will start when the previous one is cancelled
                    for ((index, patchItem) in items.reversed().withIndex()) {
                        if (patchItem.state === PatchFileState.IN_PROGRESS
                                || patchItem.state === PatchFileState.PENDING) {
                            service!!.cancelPatching(patchItem.taskId)
                            if (patchItem.state === PatchFileState.PENDING) {
                                patchItem.state = PatchFileState.QUEUED
                                adapter.notifyItemChanged(index)
                            }
                        }
                    }
                })
                return true
            }
        }
        return super.onOptionsItemSelected(item)
    }

    /**
     * {@inheritDoc}
     */
    override fun onStart() {
        super.onStart()

        // Bind to our service. We start the service so it doesn't get killed when all the clients
        // unbind from the service. The service will automatically stop once all clients have
        // unbound and all tasks have completed.
        val intent = Intent(activity, PatcherService::class.java)
        activity!!.bindService(intent, this, Context.BIND_AUTO_CREATE)
        activity!!.startService(intent)
    }

    /**
     * {@inheritDoc}
     */
    override fun onStop() {
        super.onStop()

        // Cancel metadata query task
        cancelQueryUriMetadata()

        // If we connected to the service and registered the callback, now we unregister it
        if (service != null) {
            service!!.unregisterCallback(callback)
        }

        // Unbind from our service
        activity!!.unbindService(this)
        service = null

        // At this point, the callback will not get called anymore by the service. Now we just need
        // to remove all pending Runnables that were posted to handler.
        handler.removeCallbacksAndMessages(null)
    }

    /**
     * {@inheritDoc}
     */
    override fun onServiceConnected(name: ComponentName, service: IBinder) {
        // Save a reference to the service so we can interact with it
        val binder = service as ThreadPoolServiceBinder
        this.service = binder.service as PatcherService

        // Register callback
        this.service!!.registerCallback(callback)

        for (runnable in execOnConnect) {
            runnable.run()
        }
        execOnConnect.clear()
    }

    /**
     * {@inheritDoc}
     */
    override fun onServiceDisconnected(componentName: ComponentName) {
        service = null
    }

    /**
     * {@inheritDoc}
     */
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        when (requestCode) {
            ACTIVITY_REQUEST_INPUT_FILE -> if (data != null && resultCode == Activity.RESULT_OK) {
                onSelectedInputUri(data.data!!)
            }
            ACTIVITY_REQUEST_OUTPUT_FILE -> if (data != null && resultCode == Activity.RESULT_OK) {
                onSelectedOutputUri(data.data!!)
            }
            else -> super.onActivityResult(requestCode, resultCode, data)
        }
    }

    /**
     * {@inheritDoc}
     */
    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String>,
                                            grantResults: IntArray) {
        when (requestCode) {
            PERMISSIONS_REQUEST_STORAGE -> if (PermissionUtils.verifyPermissions(grantResults)) {
                selectInputUri()
            } else {
                if (PermissionUtils.shouldShowRationales(this, permissions)) {
                    showPermissionsRationaleDialogYesNo()
                } else {
                    showPermissionsRationaleDialogConfirm()
                }
            }
            else -> super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        }
    }

    /**
     * {@inheritDoc}
     */
    override fun onItemMoved(fromPosition: Int, toPosition: Int) {
        // Update index map
        val fromItem = items[fromPosition]
        val toItem = items[toPosition]
        itemsMap.put(fromItem.taskId, toPosition)
        itemsMap.put(toItem.taskId, fromPosition)

        Collections.swap(items, fromPosition, toPosition)
        adapter.notifyItemMoved(fromPosition, toPosition)
    }

    /**
     * {@inheritDoc}
     */
    override fun onItemDismissed(position: Int) {
        val item = items[position]
        itemsMap.delete(item.taskId)

        items.removeAt(position)
        adapter.notifyItemRemoved(position)

        updateAddZipMessage()
        updateToolbarIcons()

        executeNeedsService(Runnable { service!!.removePatchFileTask(item.taskId) })
    }

    /**
     * {@inheritDoc}
     */
    override fun onPatchFileItemClicked(item: PatchFileItem) {
        showPatcherOptionsDialog(item.taskId)
    }

    /**
     * Toggle main UI and progress bar visibility depending on [.showingProgress]
     */
    private fun updateLoadingStatus() {
        if (showingProgress) {
            recycler.visibility = View.GONE
            fab.hideMenuButton(true)
            addZipMessage.visibility = View.GONE
            progressBar.visibility = View.VISIBLE
        } else {
            recycler.visibility = View.VISIBLE
            fab.showMenuButton(true)
            addZipMessage.visibility = View.VISIBLE
            progressBar.visibility = View.GONE
        }
    }

    /**
     * Called when the patcher data and libmbpatcher have been initialized
     *
     * This method is guaranteed to be called only once during between onStart() and onStop()
     */
    private fun onPatcherInitialized() {
        Log.d(TAG, "Patcher has been initialized")
        onReady()
    }

    /**
     * Called when everything has been initialized and we have the necessary permissions
     */
    private fun onReady() {
        // Load patch file items from the service
        val taskIds = service!!.patchFileTaskIds
        Arrays.sort(taskIds)
        for (taskId in taskIds) {
            val item = PatchFileItem(
                    service!!.getPatcherId(taskId),
                    service!!.getInputUri(taskId),
                    service!!.getOutputUri(taskId),
                    service!!.getDisplayName(taskId),
                    service!!.getDevice(taskId)!!,
                    service!!.getRomId(taskId)!!
            )
            item.taskId = taskId
            item.state = service!!.getState(taskId)
            item.details = service!!.getDetails(taskId)
            item.bytes = service!!.getCurrentBytes(taskId)
            item.maxBytes = service!!.getMaximumBytes(taskId)
            item.files = service!!.getCurrentFiles(taskId)
            item.maxFiles = service!!.getMaximumFiles(taskId)
            item.successful = service!!.isSuccessful(taskId)
            item.errorCode = service!!.getErrorCode(taskId)

            items.add(item)
            itemsMap.put(taskId, items.size - 1)
        }
        adapter.notifyDataSetChanged()

        // We are now fully initialized. Hide the loading spinner
        showingProgress = false
        updateLoadingStatus()

        // Hide add zip message if we've already added a zip
        updateAddZipMessage()

        updateToolbarIcons()
        updateModifiability()
        updateScreenOnState()

        // Resume URI metadata query if it was interrupted
        if (queryingMetadata) {
            queryUriMetadata()
        }
    }

    private fun updateAddZipMessage() {
        addZipMessage.visibility = if (items.isEmpty()) View.VISIBLE else View.GONE
    }

    private fun updateToolbarIcons() {
        if (checkItem != null && cancelItem != null) {
            var checkVisible = false
            var cancelVisible = false
            for (item in items) {
                if (item.state === PatchFileState.QUEUED) {
                    checkVisible = true
                } else if (item.state === PatchFileState.IN_PROGRESS) {
                    checkVisible = false
                    cancelVisible = true
                    break
                }
            }
            checkItem!!.isVisible = checkVisible
            cancelItem!!.isVisible = cancelVisible
        }
    }

    private fun updateModifiability() {
        val canModify = items.none {
            it.state === PatchFileState.PENDING || it.state === PatchFileState.IN_PROGRESS
        }
        itemTouchCallback.isLongPressDragEnabled = canModify
        itemTouchCallback.isItemViewSwipeEnabled = canModify
    }

    private fun updateScreenOnState() {
        val keepScreenOn = items.any {
            it.state === PatchFileState.PENDING || it.state === PatchFileState.IN_PROGRESS
        }
        if (keepScreenOn) {
            activity!!.window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        } else {
            activity!!.window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
        }
    }

    private fun showPatcherOptionsDialog(taskId: Int) {
        var preselectedDeviceId: String? = null
        var preselectedRomId: String? = null

        if (taskId >= 0) {
            val item = items[itemsMap[taskId]]
            preselectedDeviceId = item.device.id
            preselectedRomId = item.romId
        }

        val dialog = PatcherOptionsDialog.newInstanceFromFragment(
                this, taskId, preselectedDeviceId, preselectedRomId)
        dialog.show(fragmentManager!!, DIALOG_PATCHER_OPTIONS)
    }

    private fun startFileSelection(patcherId: String) {
        selectedPatcherId = patcherId

        if (PermissionUtils.supportsRuntimePermissions()) {
            requestPermissions()
        } else {
            selectInputUri()
        }
    }

    private fun requestPermissions() {
        requestPermissions(PermissionUtils.STORAGE_PERMISSIONS, PERMISSIONS_REQUEST_STORAGE)
    }

    private fun showPermissionsRationaleDialogYesNo() {
        var dialog = fragmentManager!!.findFragmentByTag(YES_NO_DIALOG_PERMISSIONS) as GenericYesNoDialog?
        if (dialog == null) {
            val builder = GenericYesNoDialog.Builder()
            builder.message(R.string.patcher_storage_permission_required)
            builder.positive(R.string.try_again)
            builder.negative(R.string.cancel)
            dialog = builder.buildFromFragment(YES_NO_DIALOG_PERMISSIONS, this)
            dialog.show(fragmentManager!!, YES_NO_DIALOG_PERMISSIONS)
        }
    }

    private fun showPermissionsRationaleDialogConfirm() {
        var dialog = fragmentManager!!.findFragmentByTag(CONFIRM_DIALOG_PERMISSIONS) as GenericConfirmDialog?
        if (dialog == null) {
            val builder = GenericConfirmDialog.Builder()
            builder.message(R.string.patcher_storage_permission_required)
            builder.buttonText(R.string.ok)
            dialog = builder.build()
            dialog.show(fragmentManager!!, CONFIRM_DIALOG_PERMISSIONS)
        }
    }

    /**
     * Show activity for selecting an input file
     *
     * After the user selects a file, [.onSelectedInputUri] will be called. If the user
     * cancels the selection, nothing will happen.
     *
     * @see {@link .onSelectedInputUri
     */
    private fun selectInputUri() {
        val intent = FileUtils.getFileOpenIntent(activity!!, "*/*")
        startActivityForResult(intent, ACTIVITY_REQUEST_INPUT_FILE)
    }

    /**
     * Show activity for selecting an output file
     *
     * After the user confirms the patcher options, this function will be called. Once the user
     * selects the output filename, [.onSelectedOutputUri] will be called. If the user
     * leaves the activity without selecting a file, nothing will happen.
     *
     * The specified ROM ID is added to the suggested filename before the file extension. For
     * example, if the original input filename was "SuperDuperROM-1.0.zip" and the ROM ID is "dual",
     * then the suggested filename is "SuperDuperROM-1.0_dual.zip".
     *
     * @see [.onSelectedOutputUri]
     */
    private fun selectOutputFile() {
        val baseName: String
        val extension: String
        if (selectedPatcherId == PatcherUtils.PATCHER_ID_ODINPATCHER) {
            baseName = selectedInputFileName!!.replace(
                    "(\\.tar\\.md5(\\.gz|\\.xz)?|\\.zip)$".toRegex(), "")
            extension = "zip"
        } else {
            baseName = File(selectedInputFileName).nameWithoutExtension
            extension = File(selectedInputFileName).extension
        }
        val sb = StringBuilder()
        sb.append(baseName)
        sb.append('_')
        sb.append(selectedRomId)
        if (extension.isNotEmpty() || selectedInputFileName!!.endsWith('.')) {
            sb.append('.')
        }
        sb.append(extension)
        val desiredName = sb.toString()
        val intent = FileUtils.getFileSaveIntent(activity!!, "*/*", desiredName)
        startActivityForResult(intent, ACTIVITY_REQUEST_OUTPUT_FILE)
    }

    /**
     * Query the metadata for the input file
     *
     * After the user selects an input file, this function is called to start the task of retrieving
     * the file's name and size. Once the information has been retrieved,
     * [.onQueriedMetadata] is called.
     *
     * @see {@link .onQueriedMetadata
     */
    private fun queryUriMetadata() {
        if (queryMetadataTask != null) {
            throw IllegalStateException("Already querying metadata!")
        }
        queryMetadataTask = GetUriMetadataTask()
        queryMetadataTask!!.execute(selectedInputUri)

        // Show progress dialog. Dialog may already exist if a configuration change occurred during
        // the query (and thus, this function is called again in onReady()).
        var dialog = fragmentManager!!.findFragmentByTag(PROGRESS_DIALOG_QUERYING_METADATA) as GenericProgressDialog?
        if (dialog == null) {
            val builder = GenericProgressDialog.Builder()
            builder.message(R.string.please_wait)
            dialog = builder.build()
            dialog.show(fragmentManager!!, PROGRESS_DIALOG_QUERYING_METADATA)
        }
    }

    /**
     * Cancel task for querying the input URI metadata
     *
     * This function is a no-op if there is no such task.
     *
     * @see {@link .onStop
     */
    private fun cancelQueryUriMetadata() {
        if (queryMetadataTask != null) {
            queryMetadataTask!!.cancel(true)
            queryMetadataTask = null
        }
    }

    /**
     * Called after the user tapped the FAB and selected a file
     *
     * This function will call [.queryUriMetadata] to start querying various metadata of the
     * input URI.
     *
     * @see {@link .queryUriMetadata
     * @param uri Input file's URI
     */
    private fun onSelectedInputUri(uri: Uri) {
        selectedInputUri = uri

        queryUriMetadata()
    }

    /**
     * Called after the user selects the output file
     *
     * This function will call [.addOrEditItem] to either add the new patcher item or edit
     * the selected item.
     *
     * @see {@link .addOrEditItem
     * @param uri Output file's URI
     */
    private fun onSelectedOutputUri(uri: Uri) {
        selectedOutputUri = uri

        addOrEditItem()
    }

    /**
     * Called after the input URI's metadata has been retrieved
     *
     * This function will open the patcher options dialog.
     *
     * @param metadata URI metadata
     *
     * @see {@link .queryUriMetadata
     * @see {@link .showPatcherOptionsDialog
     */
    private fun onQueriedMetadata(metadata: UriMetadata) {
        val dialog = fragmentManager!!.findFragmentByTag(PROGRESS_DIALOG_QUERYING_METADATA) as GenericProgressDialog?
        dialog?.dismiss()

        selectedInputFileName = metadata.displayName
        selectedInputFileSize = metadata.size

        // Open patcher options
        showPatcherOptionsDialog(-1)
    }

    /**
     * Called after the user confirms the patcher options
     *
     * This function will call [.selectOutputFile] for choosing an output file
     *
     * @see {@link .selectOutputFile
     * @param id Task ID (-1 if new item)
     * @param device Target device
     * @param romId Target ROM ID
     */
    override fun onConfirmedOptions(id: Int, device: Device, romId: String) {
        selectedTaskId = id
        selectedDevice = device
        selectedRomId = romId

        selectOutputFile()
    }

    override fun onConfirmYesNo(tag: String, choice: Boolean) {
        if (YES_NO_DIALOG_PERMISSIONS == tag) {
            if (choice) {
                requestPermissions()
            }
        }
    }

    private fun addOrEditItem() {
        if (selectedTaskId >= 0) {
            // Edit existing task
            executeNeedsService(Runnable {
                val index = itemsMap[selectedTaskId]
                val item = items[index]
                item.device = selectedDevice!!
                item.romId = selectedRomId!!
                adapter.notifyItemChanged(index)

                service!!.setDevice(selectedTaskId, selectedDevice!!)
                service!!.setRomId(selectedTaskId, selectedRomId!!)
            })
            return
        }

        // Do not allow two patching operations with the same output file. This is not completely
        // foolproof since two URIs can refer to the same target path, but it's the best we can do.
        for (item in items) {
            if (item.outputUri == selectedOutputUri) {
                SnackbarUtils.createSnackbar(activity!!, fab,
                        R.string.patcher_cannot_add_same_item,
                        Snackbar.LENGTH_LONG).show()
                return
            }
        }

        executeNeedsService(Runnable {
            val pf = PatchFileItem(
                    selectedPatcherId!!,
                    selectedInputUri!!,
                    selectedOutputUri!!,
                    selectedInputFileName!!,
                    selectedDevice!!,
                    selectedRomId!!
            )
            pf.state = PatchFileState.QUEUED

            val taskId = service!!.addPatchFileTask(pf.patcherId, pf.inputUri, pf.outputUri,
                    pf.displayName, pf.device, pf.romId)
            pf.taskId = taskId

            items.add(pf)
            itemsMap.put(taskId, items.size - 1)
            adapter.notifyItemInserted(items.size - 1)
            updateAddZipMessage()
            updateToolbarIcons()
        })
    }

    /**
     * Execute a [Runnable] that requires the service to be connected
     *
     * NOTE: If the service is disconnect before this method is called and does not reconnect before
     * this fragment is destroyed, then the runnable will NOT be executed.
     *
     * @param runnable Runnable that requires access to the service
     */
    private fun executeNeedsService(runnable: Runnable) {
        if (service != null) {
            runnable.run()
        } else {
            execOnConnect.add(runnable)
        }
    }

    private inner class PatcherEventCallback : PatcherEventListener {
        override fun onPatcherInitialized() {
            // Make sure we don't initialize more than once. This event could be sent more than
            // once if, eg., service.initializePatcher() is called and the device is rotated
            // before this event is received. Then service.initializePatcher() will be called
            // again leading to a duplicate event.
            if (!initialized) {
                initialized = true
                handler.post { this@PatchFileFragment.onPatcherInitialized() }
            }
        }

        override fun onPatcherUpdateDetails(taskId: Int, details: String) {
            handler.post {
                if (itemsMap.indexOfKey(taskId) >= 0) {
                    val itemIndex = itemsMap[taskId]
                    val item = items[itemIndex]
                    item.details = details
                    adapter.notifyItemChanged(itemIndex)
                }
            }
        }

        override fun onPatcherUpdateProgress(taskId: Int, bytes: Long, maxBytes: Long) {
            handler.post {
                if (itemsMap.indexOfKey(taskId) >= 0) {
                    val itemIndex = itemsMap[taskId]
                    val item = items[itemIndex]
                    item.bytes = bytes
                    item.maxBytes = maxBytes
                    adapter.notifyItemChanged(itemIndex)
                }
            }
        }

        override fun onPatcherUpdateFilesProgress(taskId: Int, files: Long, maxFiles: Long) {
            handler.post {
                if (itemsMap.indexOfKey(taskId) >= 0) {
                    val itemIndex = itemsMap[taskId]
                    val item = items[itemIndex]
                    item.files = files
                    item.maxFiles = maxFiles
                    adapter.notifyItemChanged(itemIndex)
                }
            }
        }

        override fun onPatcherStarted(taskId: Int) {
            handler.post {
                if (itemsMap.indexOfKey(taskId) >= 0) {
                    val itemIndex = itemsMap[taskId]
                    val item = items[itemIndex]
                    item.state = PatchFileState.IN_PROGRESS
                    updateToolbarIcons()
                    updateModifiability()
                    updateScreenOnState()
                    adapter.notifyItemChanged(itemIndex)
                }
            }
        }

        override fun onPatcherFinished(taskId: Int, state: PatchFileState, ret: Boolean,
                                       errorCode: Int) {
            handler.post {
                if (itemsMap.indexOfKey(taskId) >= 0) {
                    val itemIndex = itemsMap[taskId]
                    val item = items[itemIndex]

                    item.state = state
                    item.details = getString(R.string.details_done)
                    item.successful = ret
                    item.errorCode = errorCode

                    updateToolbarIcons()
                    updateModifiability()
                    updateScreenOnState()

                    adapter.notifyItemChanged(itemIndex)

                    //return Result(if (ret) RESULT_PATCHING_SUCCEEDED else RESULT_PATCHING_FAILED,
                    //        "See ${LogUtils.getPath("patch-file.log")} for details", newPath)
                }
            }
        }
    }

    /**
     * Task to query the display name, size, and MIME type of a list of openable URIs.
     */
    private inner class GetUriMetadataTask : AsyncTask<Uri, Void, Array<UriMetadata>>() {
        private var cr: ContentResolver? = null

        override fun onPreExecute() {
            cr = activity!!.contentResolver
            queryingMetadata = true
        }

        override fun doInBackground(vararg params: Uri): Array<UriMetadata> {
            return FileUtils.queryUriMetadata(cr!!, *params)
        }

        override fun onPostExecute(metadataList: Array<UriMetadata>) {
            cr = null

            if (isAdded) {
                queryingMetadata = false
                onQueriedMetadata(metadataList[0])
            }
        }
    }

    companion object {
        val FRAGMENT_TAG: String = PatchFileFragment::class.java.canonicalName
        private val TAG = PatchFileFragment::class.java.simpleName

        private val DIALOG_PATCHER_OPTIONS =
                "${PatchFileFragment::class.java.canonicalName}.patcher_options"
        private val YES_NO_DIALOG_PERMISSIONS =
                "${PatchFileFragment::class.java.canonicalName}.yes_no.permissions"
        private val CONFIRM_DIALOG_PERMISSIONS =
                "${PatchFileFragment::class.java.canonicalName}.confirm.permissions"
        private val PROGRESS_DIALOG_QUERYING_METADATA =
                "${PatchFileFragment::class.java.canonicalName}.progress.querying_metadata"

        private const val EXTRA_SELECTED_PATCHER_ID = "selected_patcher_id"
        private const val EXTRA_SELECTED_INPUT_URI = "selected_input_file"
        private const val EXTRA_SELECTED_OUTPUT_URI = "selected_output_file"
        private const val EXTRA_SELECTED_INPUT_FILE_NAME = "selected_input_file_name"
        private const val EXTRA_SELECTED_INPUT_FILE_SIZE = "selected_input_file_size"
        private const val EXTRA_SELECTED_TASK_ID = "selected_task_id"
        private const val EXTRA_SELECTED_DEVICE = "selected_device"
        private const val EXTRA_SELECTED_ROM_ID = "selected_rom_id"
        private const val EXTRA_QUERYING_METADATA = "querying_metadata"

        /** Request code for choosing input file  */
        private const val ACTIVITY_REQUEST_INPUT_FILE = 1000
        /** Request code for choosing output file  */
        private const val ACTIVITY_REQUEST_OUTPUT_FILE = 1001
        /**
         * Request code for storage permissions request
         * (used in [.onRequestPermissionsResult])
         */
        private const val PERMISSIONS_REQUEST_STORAGE = 1

        fun newInstance(): PatchFileFragment {
            return PatchFileFragment()
        }
    }
}
