/*
 * Copyright (C) 2014-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.switcher

import android.app.Activity
import android.content.ComponentName
import android.content.ContentResolver
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.content.SharedPreferences
import android.net.Uri
import android.os.AsyncTask
import android.os.Bundle
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ProgressBar
import android.widget.TextView
import android.widget.Toast
import androidx.cardview.widget.CardView
import androidx.core.content.edit
import androidx.fragment.app.Fragment
import androidx.loader.app.LoaderManager
import androidx.loader.app.LoaderManager.LoaderCallbacks
import androidx.loader.content.AsyncTaskLoader
import androidx.loader.content.Loader
import androidx.recyclerview.widget.ItemTouchHelper
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import androidx.recyclerview.widget.RecyclerView.ViewHolder
import com.github.chenxiaolong.dualbootpatcher.Constants
import com.github.chenxiaolong.dualbootpatcher.FileUtils
import com.github.chenxiaolong.dualbootpatcher.FileUtils.UriMetadata
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.RomUtils
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder
import com.github.chenxiaolong.dualbootpatcher.dialogs.FirstUseDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.FirstUseDialog.FirstUseDialogListener
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericSingleChoiceDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericSingleChoiceDialog.GenericSingleChoiceDialogListener
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils.Feature
import com.github.chenxiaolong.dualbootpatcher.switcher.BackupRestoreTargetsSelectionDialog.BackupRestoreTargetsSelectionDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.ChangeInstallLocationDialog.ChangeInstallLocationDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.InAppFlashingFragment.LoaderResult
import com.github.chenxiaolong.dualbootpatcher.switcher.InstallLocationSelectionDialog.RomIdSelectionDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.NamedSlotIdInputDialog.NamedSlotIdInputDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.VerificationResult
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.BackupRestoreParams
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.BackupRestoreParams.Action
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.MbtoolAction
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.MbtoolAction.Type
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.RomInstallerParams
import com.github.chenxiaolong.dualbootpatcher.switcher.service.VerifyZipTask.VerifyZipTaskListener
import com.github.chenxiaolong.dualbootpatcher.views.DragSwipeItemTouchCallback
import com.github.chenxiaolong.dualbootpatcher.views.DragSwipeItemTouchCallback.OnItemMovedOrDismissedListener
import com.github.clans.fab.FloatingActionButton
import com.github.clans.fab.FloatingActionMenu
import java.util.*

class InAppFlashingFragment : Fragment(), FirstUseDialogListener, RomIdSelectionDialogListener,
        NamedSlotIdInputDialogListener, ChangeInstallLocationDialogListener,
        GenericSingleChoiceDialogListener, BackupRestoreTargetsSelectionDialogListener,
        LoaderCallbacks<LoaderResult>, ServiceConnection, OnItemMovedOrDismissedListener {
    private lateinit var activityCallback: OnReadyStateChangedListener

    private lateinit var prefs: SharedPreferences

    private var selectedUri: Uri? = null
    private var selectedUriFileName: String? = null
    private var selectedBackupDirUri: Uri? = null
    private var selectedBackupName: String? = null
    private var selectedBackupTargets: Array<String>? = null
    private var selectedRomId: String? = null
    private var currentRomId: String? = null
    private var zipRomId: String? = null
    private var addType: Type? = null

    private lateinit var progressBar: ProgressBar

    private val pendingActions = ArrayList<MbtoolAction>()
    private lateinit var adapter: PendingActionCardAdapter

    private var verifyZipOnServiceConnected: Boolean = false

    private var taskIdVerifyZip = -1

    /** Task IDs to remove  */
    private val taskIdsToRemove = ArrayList<Int>()

    /** Switcher service  */
    private var service: SwitcherService? = null
    /** Callback for events from the service  */
    private val callback = SwitcherEventCallback()

    /** Handler for processing events from the service on the UI thread  */
    private val handler = Handler(Looper.getMainLooper())

    /** Whether we're querying the URI metadata  */
    private var queryingMetadata: Boolean = false
    /** Task for querying the metadata of URIs  */
    private var queryMetadataTask: GetUriMetadataTask? = null

    interface OnReadyStateChangedListener {
        fun onReady(ready: Boolean)

        fun onFinished()
    }

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?,
                              savedInstanceState: Bundle?): View? {
        return inflater.inflate(R.layout.fragment_in_app_flashing, container, false)
    }

    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)

        if (savedInstanceState != null) {
            val savedActions = savedInstanceState.getParcelableArrayList<MbtoolAction>(
                    EXTRA_PENDING_ACTIONS)!!
            pendingActions.addAll(savedActions)
        }

        progressBar = activity!!.findViewById(R.id.card_list_loading)
        val cardListView = activity!!.findViewById<RecyclerView>(R.id.card_list)

        adapter = PendingActionCardAdapter(activity!!, pendingActions)
        cardListView.setHasFixedSize(true)
        cardListView.adapter = adapter

        val itemTouchCallback = DragSwipeItemTouchCallback(this)
        val itemTouchHelper = ItemTouchHelper(itemTouchCallback)
        itemTouchHelper.attachToRecyclerView(cardListView)

        val llm = LinearLayoutManager(activity)
        llm.orientation = RecyclerView.VERTICAL
        cardListView.layoutManager = llm

        val fabMenu = activity!!.findViewById<FloatingActionMenu>(R.id.fab_add_item_menu)
        val fabAddPatchedFile = activity!!.findViewById<FloatingActionButton>(R.id.fab_add_patched_file)
        val fabAddBackup = activity!!.findViewById<FloatingActionButton>(R.id.fab_add_backup)

        fabAddPatchedFile.setOnClickListener {
            addPatchedFile()
            fabMenu.close(true)
        }
        fabAddBackup.setOnClickListener {
            addBackup()
            fabMenu.close(true)
        }

        if (savedInstanceState != null) {
            selectedUri = savedInstanceState.getParcelable(EXTRA_SELECTED_URI)
            selectedUriFileName = savedInstanceState.getString(EXTRA_SELECTED_URI_FILE_NAME)
            selectedBackupDirUri = savedInstanceState.getParcelable(EXTRA_SELECTED_BACKUP_DIR_URI)
            selectedBackupName = savedInstanceState.getString(EXTRA_SELECTED_BACKUP_NAME)
            selectedBackupTargets = savedInstanceState.getStringArray(EXTRA_SELECTED_BACKUP_TARGETS)
            selectedRomId = savedInstanceState.getString(EXTRA_SELECTED_ROM_ID)
            zipRomId = savedInstanceState.getString(EXTRA_ZIP_ROM_ID)
            addType = savedInstanceState.getSerializable(EXTRA_ADD_TYPE) as Type?
            taskIdVerifyZip = savedInstanceState.getInt(EXTRA_TASK_ID_VERIFY_ZIP)
            queryingMetadata = savedInstanceState.getBoolean(EXTRA_QUERYING_METADATA)
        }

        activityCallback = activity as OnReadyStateChangedListener
        activityCallback.onReady(!pendingActions.isEmpty())

        prefs = activity!!.getSharedPreferences("settings", 0)

        if (savedInstanceState == null) {
            val shouldShow = prefs.getBoolean(PREF_SHOW_FIRST_USE_DIALOG, true)
            if (shouldShow) {
                val d = FirstUseDialog.newInstance(
                        this, R.string.in_app_flashing_title,
                        R.string.in_app_flashing_dialog_first_use)
                d.show(fragmentManager!!, CONFIRM_DIALOG_FIRST_USE)
            }
        }

        LoaderManager.getInstance(this).initLoader(0, null, this)
    }

    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)

        outState.putParcelableArrayList(EXTRA_PENDING_ACTIONS, pendingActions)

        outState.putParcelable(EXTRA_SELECTED_URI, selectedUri)
        outState.putString(EXTRA_SELECTED_URI_FILE_NAME, selectedUriFileName)
        outState.putParcelable(EXTRA_SELECTED_BACKUP_DIR_URI, selectedBackupDirUri)
        outState.putString(EXTRA_SELECTED_BACKUP_NAME, selectedBackupName)
        outState.putStringArray(EXTRA_SELECTED_BACKUP_TARGETS, selectedBackupTargets)
        outState.putString(EXTRA_SELECTED_ROM_ID, selectedRomId)
        outState.putString(EXTRA_ZIP_ROM_ID, zipRomId)
        outState.putSerializable(EXTRA_ADD_TYPE, addType)
        outState.putInt(EXTRA_TASK_ID_VERIFY_ZIP, taskIdVerifyZip)
        outState.putBoolean(EXTRA_QUERYING_METADATA, queryingMetadata)
    }

    override fun onStart() {
        super.onStart()

        // Start and bind to the service
        val intent = Intent(activity, SwitcherService::class.java)
        activity!!.bindService(intent, this, Context.BIND_AUTO_CREATE)
        activity!!.startService(intent)
    }

    override fun onStop() {
        super.onStop()

        // Cancel metadata query task
        cancelQueryUriMetadata()

        // If we connected to the service and registered the callback, now we unregister it
        if (service != null) {
            if (taskIdVerifyZip >= 0) {
                service!!.removeCallback(taskIdVerifyZip, callback)
            }
        }

        // Unbind from our service
        activity!!.unbindService(this)
        service = null

        // At this point, the callback will not get called anymore by the service. Now we just need
        // to remove all pending Runnables that were posted to handler.
        handler.removeCallbacksAndMessages(null)
    }

    override fun onServiceConnected(name: ComponentName, service: IBinder) {
        // Save a reference to the service so we can interact with it
        val binder = service as ThreadPoolServiceBinder
        this.service = binder.service as SwitcherService

        // Remove old task IDs
        for (taskId in taskIdsToRemove) {
            this.service!!.removeCachedTask(taskId)
        }
        taskIdsToRemove.clear()

        if (taskIdVerifyZip >= 0) {
            this.service!!.addCallback(taskIdVerifyZip, callback)
        }

        if (verifyZipOnServiceConnected) {
            verifyZipOnServiceConnected = false
            verifyZip()
        }

        if (queryingMetadata) {
            queryUriMetadata()
        }
    }

    override fun onServiceDisconnected(name: ComponentName) {
        service = null
    }

    private fun removeCachedTaskId(taskId: Int) {
        if (service != null) {
            service!!.removeCachedTask(taskId)
        } else {
            taskIdsToRemove.add(taskId)
        }
    }

    override fun onCreateLoader(id: Int, args: Bundle?): Loader<LoaderResult> {
        return BuiltinRomsLoader(activity!!)
    }

    override fun onLoadFinished(loader: Loader<LoaderResult>, result: LoaderResult) {
        if (result.currentRom != null) {
            currentRomId = result.currentRom!!.id
        }

        progressBar.visibility = View.GONE
    }

    override fun onLoaderReset(loader: Loader<LoaderResult>) {}

    private fun addPatchedFile() {
        addType = Type.ROM_INSTALLER

        // Show file chooser
        val intent = FileUtils.getFileOpenIntent(activity!!, "*/*")
        startActivityForResult(intent, ACTIVITY_REQUEST_FILE)
    }

    private fun addBackup() {
        selectedBackupDirUri = Uri.parse(prefs.getString(
                Constants.Preferences.BACKUP_DIRECTORY_URI,
                Constants.Defaults.BACKUP_DIRECTORY_URI))

        val backupNames = getDirectories(activity, selectedBackupDirUri)

        if (backupNames == null || backupNames.isEmpty()) {
            Toast.makeText(activity, R.string.in_app_flashing_no_backups_available,
                    Toast.LENGTH_LONG).show()
        } else {
            addType = Type.BACKUP_RESTORE

            val builder = GenericSingleChoiceDialog.Builder()
            builder.message(R.string.in_app_flashing_select_backup_dialog_desc)
            builder.positive(R.string.ok)
            builder.negative(R.string.cancel)
            builder.choices(*backupNames)
            builder.buildFromFragment(CONFIRM_DIALOG_SELECT_BACKUP, this).show(
                    fragmentManager!!, CONFIRM_DIALOG_SELECT_BACKUP)
        }
    }

    private fun onVerifiedZip(romId: String?, result: VerificationResult) {
        removeCachedTaskId(taskIdVerifyZip)
        taskIdVerifyZip = -1

        val dialog = fragmentManager!!.findFragmentByTag(PROGRESS_DIALOG_VERIFY_ZIP)
                as GenericProgressDialog?
        dialog?.dismiss()

        zipRomId = romId

        if (result === VerificationResult.NO_ERROR) {
            if (romId != null) {
                val cild = ChangeInstallLocationDialog.newInstance(this, romId)
                cild.show(fragmentManager!!, CONFIRM_DIALOG_INSTALL_LOCATION)
            } else {
                showRomIdSelectionDialog()
            }
        } else {
            val error = when (result) {
                SwitcherUtils.VerificationResult.ERROR_ZIP_NOT_FOUND ->
                    getString(R.string.in_app_flashing_error_zip_not_found)
                SwitcherUtils.VerificationResult.ERROR_ZIP_READ_FAIL ->
                    getString(R.string.in_app_flashing_error_zip_read_fail)
                SwitcherUtils.VerificationResult.ERROR_NOT_MULTIBOOT ->
                    getString(R.string.in_app_flashing_error_not_multiboot)
                SwitcherUtils.VerificationResult.ERROR_VERSION_TOO_OLD -> String.format(
                        getString(R.string.in_app_flashing_error_version_too_old),
                        MbtoolUtils.getMinimumRequiredVersion(Feature.IN_APP_INSTALLATION))
                else -> throw IllegalStateException("Invalid verification result ID")
            }

            val builder = GenericConfirmDialog.Builder()
            builder.message(error)
            builder.buttonText(R.string.ok)
            builder.build().show(fragmentManager!!, CONFIRM_DIALOG_ERROR)
        }
    }

    /**
     * Called after the input URI's metadata has been retrieved
     *
     * @param metadata URI metadata
     *
     * @see .queryUriMetadata
     */
    private fun onQueriedMetadata(metadata: UriMetadata) {
        val dialog = fragmentManager!!.findFragmentByTag(PROGRESS_DIALOG_QUERYING_METADATA)
                as GenericProgressDialog?
        dialog?.dismiss()

        selectedUriFileName = metadata.displayName

        if (service != null) {
            verifyZip()
        } else {
            verifyZipOnServiceConnected = true
        }
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        when (requestCode) {
            PERFORM_ACTIONS -> activityCallback.onFinished()
            ACTIVITY_REQUEST_FILE -> if (data != null && resultCode == Activity.RESULT_OK) {
                selectedUri = data.data

                val builder = GenericProgressDialog.Builder()
                builder.title(R.string.in_app_flashing_dialog_verifying_zip)
                builder.message(R.string.please_wait)
                builder.build().show(fragmentManager!!, PROGRESS_DIALOG_VERIFY_ZIP)

                queryUriMetadata()
            }
            else -> super.onActivityResult(requestCode, resultCode, data)
        }
    }

    private fun verifyZip() {
        taskIdVerifyZip = service!!.verifyZip(selectedUri!!)
        service!!.addCallback(taskIdVerifyZip, callback)
        service!!.enqueueTaskId(taskIdVerifyZip)
    }

    /**
     * Query the metadata for the input file
     *
     * After the user selects an input file, this function is called to start the task of retrieving
     * the file's name and size. Once the information has been retrieved,
     * [.onQueriedMetadata] is called.
     *
     * @see .onQueriedMetadata
     */
    private fun queryUriMetadata() {
        if (queryMetadataTask != null) {
            throw IllegalStateException("Already querying metadata!")
        }
        queryMetadataTask = GetUriMetadataTask()
        queryMetadataTask!!.execute(selectedUri)

        // Show progress dialog. Dialog may already exist if a configuration change occurred during
        // the query (and thus, this function is called again in onReady()).
        var dialog = fragmentManager!!.findFragmentByTag(PROGRESS_DIALOG_QUERYING_METADATA)
                as GenericProgressDialog?
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
     * @see .onStop
     */
    private fun cancelQueryUriMetadata() {
        if (queryMetadataTask != null) {
            queryMetadataTask!!.cancel(true)
            queryMetadataTask = null
        }
    }

    override fun onConfirmFirstUse(dontShowAgain: Boolean) {
        prefs.edit {
            putBoolean(PREF_SHOW_FIRST_USE_DIALOG, !dontShowAgain)
        }
    }

    override fun onConfirmSingleChoice(tag: String, index: Int, text: String?) {
        selectedBackupName = text

        // Adk for restore targets
        val d = BackupRestoreTargetsSelectionDialog.newInstanceFromFragment(this, Action.RESTORE)
        d.show(fragmentManager!!, CONFIRM_DIALOG_SELECT_TARGETS)
    }

    override fun onSelectedBackupRestoreTargets(targets: Array<String>) {
        selectedBackupTargets = targets

        // Ask for ROM ID
        showRomIdSelectionDialog()
    }

    override fun onSelectedInstallLocation(romId: String) {
        selectedRomId = romId
        onHaveRomId()
    }

    override fun onSelectedTemplateLocation(prefix: String) {
        val d = NamedSlotIdInputDialog.newInstance(this, prefix)
        d.show(fragmentManager!!, CONFIRM_DIALOG_NAMED_SLOT_ID)
    }

    override fun onSelectedNamedSlotRomId(romId: String) {
        selectedRomId = romId
        onHaveRomId()
    }

    override fun onChangeInstallLocationClicked(changeInstallLocation: Boolean) {
        if (changeInstallLocation) {
            showRomIdSelectionDialog()
        } else {
            selectedRomId = zipRomId
            onHaveRomId()
        }
    }

    override fun onItemMoved(fromPosition: Int, toPosition: Int) {
        Collections.swap(pendingActions, fromPosition, toPosition)
        adapter.notifyItemMoved(fromPosition, toPosition)
    }

    override fun onItemDismissed(position: Int) {
        pendingActions.removeAt(position)
        adapter.notifyItemRemoved(position)

        if (pendingActions.isEmpty()) {
            activityCallback.onReady(false)
        }
    }

    private fun onHaveRomId() {
        if (selectedRomId == currentRomId) {
            val builder = GenericConfirmDialog.Builder()
            builder.message(R.string.in_app_flashing_error_no_overwrite_rom)
            builder.buttonText(R.string.ok)
            builder.build().show(fragmentManager!!, CONFIRM_DIALOG_ERROR)
        } else {
            activityCallback.onReady(true)

            var action: MbtoolAction? = null

            if (addType == Type.ROM_INSTALLER) {
                val params = RomInstallerParams(
                        selectedUri!!, selectedUriFileName!!, selectedRomId!!)
                action = MbtoolAction(params)
            } else if (addType == Type.BACKUP_RESTORE) {
                val params = BackupRestoreParams(Action.RESTORE, selectedRomId!!,
                        selectedBackupTargets!!, selectedBackupName!!, selectedBackupDirUri!!,
                        false)
                action = MbtoolAction(params)
            }

            if (action != null) {
                pendingActions.add(action)
                adapter.notifyItemInserted(pendingActions.size - 1)
            }
        }
    }

    private fun showRomIdSelectionDialog() {
        val dialog = InstallLocationSelectionDialog.newInstance(this)
        dialog.show(fragmentManager!!, CONFIRM_DIALOG_ROM_ID)
    }

    fun onActionBarCheckItemClicked() {
        val intent = Intent(activity, MbtoolTaskOutputActivity::class.java)
        intent.putExtra(MbtoolTaskOutputFragment.PARAM_ACTIONS, pendingActions.toTypedArray())
        startActivityForResult(intent, PERFORM_ACTIONS)
    }

    private class BuiltinRomsLoader(context: Context) : AsyncTaskLoader<LoaderResult>(context) {
        private var result: LoaderResult? = null

        init {
            onContentChanged()
        }

        override fun onStartLoading() {
            if (result != null) {
                deliverResult(result)
            } else if (takeContentChanged()) {
                forceLoad()
            }
        }

        override fun loadInBackground(): LoaderResult? {
            var currentRom: RomInformation? = null

            try {
                MbtoolConnection(context).use { conn ->
                    val iface = conn.`interface`
                    currentRom = RomUtils.getCurrentRom(context, iface!!)
                }
            } catch (e: Exception) {
                // Ignore
            }

            result = LoaderResult(currentRom)
            return result
        }
    }

    class LoaderResult(
        internal var currentRom: RomInformation?
    )

    private class PendingActionViewHolder(itemView: View) : ViewHolder(itemView) {
        internal var vCard: CardView = itemView as CardView
        internal var vTitle: TextView = vCard.findViewById(R.id.action_title)
        internal var vSubtitle1: TextView = vCard.findViewById(R.id.action_subtitle1)
        internal var vSubtitle2: TextView = vCard.findViewById(R.id.action_subtitle2)
        internal var vSubtitle3: TextView = vCard.findViewById(R.id.action_subtitle3)
    }

    private class PendingActionCardAdapter(
            private val context: Context,
            private val items: List<MbtoolAction>
    ) : RecyclerView.Adapter<PendingActionViewHolder>() {
        override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): PendingActionViewHolder {
            val view = LayoutInflater
                    .from(parent.context)
                    .inflate(R.layout.card_v7_pending_action, parent, false)
            return PendingActionViewHolder(view)
        }

        override fun onBindViewHolder(holder: PendingActionViewHolder, position: Int) {
            val pa = items[position]

            when (pa.type) {
                MbtoolAction.Type.ROM_INSTALLER -> {
                    val params = pa.romInstallerParams

                    holder.vTitle.setText(R.string.in_app_flashing_action_flash_file)
                    holder.vSubtitle1.text = context.getString(
                            R.string.in_app_flashing_filename, params!!.displayName)
                    holder.vSubtitle2.text = context.getString(
                            R.string.in_app_flashing_location, params.romId)
                    holder.vSubtitle3.visibility = View.GONE
                }
                MbtoolAction.Type.BACKUP_RESTORE -> {
                    val params = pa.backupRestoreParams

                    holder.vTitle.setText(R.string.in_app_flashing_action_restore_backup)
                    holder.vSubtitle1.text = context.getString(
                            R.string.in_app_flashing_backup_name, params!!.backupName)
                    holder.vSubtitle2.text = context.getString(
                            R.string.in_app_flashing_restore_targets,
                            Arrays.toString(params.targets))
                    holder.vSubtitle3.text = context.getString(
                            R.string.in_app_flashing_location, params.romId)
                    holder.vSubtitle3.visibility = View.VISIBLE
                }
                else -> {}
            }
        }

        override fun getItemCount(): Int {
            return items.size
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

        override fun onPostExecute(metadatas: Array<UriMetadata>) {
            cr = null

            if (isAdded) {
                queryingMetadata = false
                onQueriedMetadata(metadatas[0])
            }
        }
    }

    private inner class SwitcherEventCallback : VerifyZipTaskListener {
        override fun onVerifiedZip(taskId: Int, uri: Uri, result: VerificationResult,
                                   romId: String?) {
            if (taskId == taskIdVerifyZip) {
                handler.post { this@InAppFlashingFragment.onVerifiedZip(romId, result) }
            }
        }
    }

    companion object {
        private const val PERFORM_ACTIONS = 1234

        private const val EXTRA_PENDING_ACTIONS = "pending_actions"
        private const val EXTRA_SELECTED_URI = "selected_uri"
        private const val EXTRA_SELECTED_URI_FILE_NAME = "selected_uri_file_name"
        private const val EXTRA_SELECTED_BACKUP_DIR_URI = "selected_backup_uri"
        private const val EXTRA_SELECTED_BACKUP_NAME = "selected_backup_name"
        private const val EXTRA_SELECTED_BACKUP_TARGETS = "selected_backup_targets"
        private const val EXTRA_SELECTED_ROM_ID = "selected_rom_id"
        private const val EXTRA_ZIP_ROM_ID = "zip_rom_id"
        private const val EXTRA_ADD_TYPE = "add_type"
        private const val EXTRA_TASK_ID_VERIFY_ZIP = "task_id_verify_zip"
        private const val EXTRA_QUERYING_METADATA = "querying_metadata"

        private const val PREF_SHOW_FIRST_USE_DIALOG = "zip_flashing_first_use_show_dialog"

        /** Request code for file picker (used in [.onActivityResult]) */
        private const val ACTIVITY_REQUEST_FILE = 1000

        private val PROGRESS_DIALOG_VERIFY_ZIP =
                "${InAppFlashingFragment::class.java.name}.progress.verify_zip"
        private val PROGRESS_DIALOG_QUERYING_METADATA =
                "${InAppFlashingFragment::class.java.name}.progress.querying_metadata"
        private val CONFIRM_DIALOG_FIRST_USE =
                "${InAppFlashingFragment::class.java.name}.confirm.first_use"
        private val CONFIRM_DIALOG_INSTALL_LOCATION =
                "${InAppFlashingFragment::class.java.name}.confirm.install_location"
        private val CONFIRM_DIALOG_NAMED_SLOT_ID =
                "${InAppFlashingFragment::class.java.name}.confirm.named_slot_id"
        private val CONFIRM_DIALOG_ROM_ID =
                "${InAppFlashingFragment::class.java.name}.confirm.rom_id"
        private val CONFIRM_DIALOG_ERROR =
                "${InAppFlashingFragment::class.java.name}.confirm.error"
        private val CONFIRM_DIALOG_SELECT_BACKUP =
                "${InAppFlashingFragment::class.java.name}.confirm.select_backup"
        private val CONFIRM_DIALOG_SELECT_TARGETS =
                "${InAppFlashingFragment::class.java.name}.confirm.select_targets"

        private fun getDirectories(context: Context?, uri: Uri?): Array<String>? {
            val directory = FileUtils.getDocumentFile(context!!, uri!!)
            val files = directory.listFiles()

            val filenames = files
                    .filter { it.isDirectory }
                    .map { it.name!! }
                    .sorted()

            return filenames.toTypedArray()
        }
    }
}
