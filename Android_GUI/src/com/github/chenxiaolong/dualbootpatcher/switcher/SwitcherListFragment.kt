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

package com.github.chenxiaolong.dualbootpatcher.switcher

import android.app.Activity
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.Bundle
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.support.annotation.StringRes
import android.support.design.widget.Snackbar
import android.support.v4.app.Fragment
import android.support.v7.widget.CardView
import android.support.v7.widget.LinearLayoutManager
import android.support.v7.widget.RecyclerView
import android.util.Log
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.view.WindowManager
import com.github.chenxiaolong.dualbootpatcher.PermissionUtils
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.RomUtils
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.SnackbarUtils
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog.GenericConfirmDialogListener
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog.GenericYesNoDialogListener
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolErrorActivity
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SetKernelResult
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SwitchRomResult
import com.github.chenxiaolong.dualbootpatcher.switcher.ConfirmChecksumIssueDialog.ConfirmChecksumIssueDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.RomCardAdapter.RomCardActionListener
import com.github.chenxiaolong.dualbootpatcher.switcher.SetKernelNeededDialog.SetKernelNeededDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.KernelStatus
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomsStateTask.GetRomsStateTaskListener
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SetKernelTask.SetKernelTaskListener
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SwitchRomTask.SwitchRomTaskListener
import com.github.chenxiaolong.dualbootpatcher.views.SwipeRefreshLayoutWorkaround
import com.github.clans.fab.FloatingActionButton
import java.util.*

class SwitcherListFragment : Fragment(), RomCardActionListener, SetKernelNeededDialogListener,
        ConfirmChecksumIssueDialogListener, GenericYesNoDialogListener,
        GenericConfirmDialogListener, ServiceConnection {
    private var havePermissionsResult = false

    /** Service task ID to get the state of installed ROMs  */
    private var taskIdGetRomsState = -1
    /** Service task ID for switching the ROM  */
    private var taskIdSwitchRom = -1
    /** Service task ID for setting the kernel  */
    private var taskIdSetKernel = -1

    /** Task IDs to remove  */
    private var taskIdsToRemove: ArrayList<Int>? = ArrayList()

    /** Switcher service  */
    private var service: SwitcherService? = null
    /** Callback for events from the service  */
    private val callback = SwitcherEventCallback()

    /** Handler for processing events from the service on the UI thread  */
    private val handler = Handler(Looper.getMainLooper())

    /** [Runnable]s to process once the service has been connected  */
    private val execOnConnect = ArrayList<Runnable>()

    private var performingAction: Boolean = false

    private lateinit var errorCardView: CardView
    private lateinit var romCardAdapter: RomCardAdapter
    private lateinit var cardListView: RecyclerView
    private lateinit var fabFlashZip: FloatingActionButton
    private lateinit var swipeRefresh: SwipeRefreshLayoutWorkaround

    private var roms: ArrayList<RomInformation>? = null
    private var currentRom: RomInformation? = null
    private var selectedRom: RomInformation? = null
    private var activeRomId: String? = null

    private var showedSetKernelWarning: Boolean = false

    private var isInitialStart: Boolean = false
    private var isLoading: Boolean = false

    /**
     * {@inheritDoc}
     */
    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)

        if (savedInstanceState != null) {
            taskIdGetRomsState = savedInstanceState.getInt(EXTRA_TASK_ID_GET_ROMS_STATE)
            taskIdSwitchRom = savedInstanceState.getInt(EXTRA_TASK_ID_SWITCH_ROM)
            taskIdSetKernel = savedInstanceState.getInt(EXTRA_TASK_ID_SET_KERNEL)
            taskIdsToRemove = savedInstanceState.getIntegerArrayList(EXTRA_TASK_IDS_TO_REMOVE)

            performingAction = savedInstanceState.getBoolean(EXTRA_PERFORMING_ACTION)

            selectedRom = savedInstanceState.getParcelable(EXTRA_SELECTED_ROM)
            activeRomId = savedInstanceState.getString(EXTRA_ACTIVE_ROM_ID)

            showedSetKernelWarning = savedInstanceState.getBoolean(EXTRA_SHOWED_SET_KERNEL_WARNING)
            havePermissionsResult = savedInstanceState.getBoolean(EXTRA_HAVE_PERMISSIONS_RESULT)

            isLoading = savedInstanceState.getBoolean(EXTRA_IS_LOADING)
        }

        fabFlashZip = activity!!.findViewById(R.id.fab_flash_zip)
        fabFlashZip.setOnClickListener {
            val intent = Intent(activity, InAppFlashingActivity::class.java)
            startActivityForResult(intent, REQUEST_FLASH_ZIP)
        }

        swipeRefresh = activity!!.findViewById(R.id.swiperefreshroms)
        swipeRefresh.setOnRefreshListener { reloadRomsState() }

        swipeRefresh.setColorSchemeResources(
                R.color.swipe_refresh_color1,
                R.color.swipe_refresh_color2,
                R.color.swipe_refresh_color3,
                R.color.swipe_refresh_color4
        )

        initErrorCard()
        initCardList()
        setErrorVisibility(false)

        isInitialStart = savedInstanceState == null
    }

    /**
     * {@inheritDoc}
     */
    override fun onStart() {
        super.onStart()

        // Start and bind to the service
        val intent = Intent(activity, SwitcherService::class.java)
        activity!!.bindService(intent, this, Context.BIND_AUTO_CREATE)
        activity!!.startService(intent)

        // Permissions
        if (PermissionUtils.supportsRuntimePermissions()) {
            if (isInitialStart) {
                requestPermissions()
            } else if (havePermissionsResult) {
                if (PermissionUtils.hasPermissions(
                        activity!!, PermissionUtils.STORAGE_PERMISSIONS)) {
                    onPermissionsGranted()
                } else {
                    onPermissionsDenied()
                }
            }
        } else {
            startWhenServiceConnected()
        }
    }

    /**
     * {@inheritDoc}
     */
    override fun onStop() {
        super.onStop()

        // If we connected to the service and registered the callback, now we unregister it
        if (service != null) {
            if (taskIdGetRomsState >= 0) {
                service!!.removeCallback(taskIdGetRomsState, callback)
            }
            if (taskIdSwitchRom >= 0) {
                service!!.removeCallback(taskIdSwitchRom, callback)
            }
            if (taskIdSetKernel >= 0) {
                service!!.removeCallback(taskIdSetKernel, callback)
            }
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
        this.service = binder.service as SwitcherService

        for (runnable in execOnConnect) {
            runnable.run()
        }
        execOnConnect.clear()
    }

    /**
     * {@inheritDoc}
     */
    override fun onServiceDisconnected(name: ComponentName) {
        service = null
    }

    override fun onRequestPermissionsResult(requestCode: Int, permissions: Array<String>,
                                            grantResults: IntArray) {
        when (requestCode) {
            PERMISSIONS_REQUEST_STORAGE -> if (PermissionUtils.verifyPermissions(grantResults)) {
                onPermissionsGranted()
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

    private fun requestPermissions() {
        requestPermissions(PermissionUtils.STORAGE_PERMISSIONS, PERMISSIONS_REQUEST_STORAGE)
    }

    private fun onPermissionsGranted() {
        havePermissionsResult = true
        startWhenServiceConnected()
    }

    private fun onPermissionsDenied() {
        havePermissionsResult = true
        startWhenServiceConnected()
    }

    private fun startWhenServiceConnected() {
        executeNeedsService(Runnable { onReady() })
    }

    private fun showPermissionsRationaleDialogYesNo() {
        var dialog = fragmentManager!!.findFragmentByTag(YES_NO_DIALOG_PERMISSIONS)
                as GenericYesNoDialog?
        if (dialog == null) {
            val builder = GenericYesNoDialog.Builder()
            builder.message(R.string.switcher_storage_permission_required)
            builder.positive(R.string.try_again)
            builder.negative(R.string.cancel)
            dialog = builder.buildFromFragment(YES_NO_DIALOG_PERMISSIONS, this)
            dialog.show(fragmentManager!!, YES_NO_DIALOG_PERMISSIONS)
        }
    }

    private fun showPermissionsRationaleDialogConfirm() {
        var dialog = fragmentManager!!.findFragmentByTag(CONFIRM_DIALOG_PERMISSIONS)
                as GenericConfirmDialog?
        if (dialog == null) {
            val builder = GenericConfirmDialog.Builder()
            builder.message(R.string.switcher_storage_permission_required)
            builder.buttonText(R.string.ok)
            dialog = builder.buildFromFragment(CONFIRM_DIALOG_PERMISSIONS, this)
            dialog.show(fragmentManager!!, CONFIRM_DIALOG_PERMISSIONS)
        }
    }

    override fun onConfirmYesNo(tag: String, choice: Boolean) {
        if (YES_NO_DIALOG_PERMISSIONS == tag) {
            if (choice) {
                requestPermissions()
            } else {
                onPermissionsDenied()
            }
        }
    }

    override fun onConfirmOk(tag: String) {
        if (CONFIRM_DIALOG_PERMISSIONS == tag) {
            onPermissionsDenied()
        }
    }

    private fun onReady() {
        // Remove old task IDs
        for (taskId in taskIdsToRemove!!) {
            service!!.removeCachedTask(taskId)
        }
        taskIdsToRemove!!.clear()

        if (taskIdGetRomsState < 0) {
            setLoadingSpinnerVisibility(true)

            // Load ROMs state asynchronously
            taskIdGetRomsState = service!!.getRomsState()
            service!!.addCallback(taskIdGetRomsState, callback)
            service!!.enqueueTaskId(taskIdGetRomsState)
        } else {
            setLoadingSpinnerVisibility(isLoading)

            service!!.addCallback(taskIdGetRomsState, callback)
        }

        if (taskIdSwitchRom >= 0) {
            service!!.addCallback(taskIdSwitchRom, callback)
        }

        if (taskIdSetKernel >= 0) {
            service!!.addCallback(taskIdSetKernel, callback)
        }
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

    /**
     * Reload ROMs state
     *
     * This will be behave as if the entire fragment has been restarted. The following actions will
     * be performed:
     * - The active ROM ID will be cleared
     * - Various UI elements will be reset to their loading state
     * - The cached task will be removed from the service
     * - The Service will reload the ROMs state
     */
    private fun reloadRomsState() {
        activeRomId = null
        setErrorVisibility(false)
        setRomListVisibility(false)
        setFabVisibility(false, true)
        setLoadingSpinnerVisibility(true)

        removeCachedTaskId(taskIdGetRomsState)
        // onActivityResult() gets called before onStart() (apparently), so we'll just set the task
        // ID to -1 and wait for the natural reload in onServiceConnected().
        if (service != null) {
            taskIdGetRomsState = service!!.getRomsState()
            service!!.addCallback(taskIdGetRomsState, callback)
            service!!.enqueueTaskId(taskIdGetRomsState)
        } else {
            taskIdGetRomsState = -1
        }
    }

    /**
     * {@inheritDoc}
     */
    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?,
                              savedInstanceState: Bundle?): View? {
        return inflater.inflate(R.layout.fragment_switcher, container, false)
    }

    /**
     * {@inheritDoc}
     */
    override fun onSaveInstanceState(outState: Bundle) {
        super.onSaveInstanceState(outState)

        outState.putInt(EXTRA_TASK_ID_GET_ROMS_STATE, taskIdGetRomsState)
        outState.putInt(EXTRA_TASK_ID_SWITCH_ROM, taskIdSwitchRom)
        outState.putInt(EXTRA_TASK_ID_SET_KERNEL, taskIdSetKernel)
        outState.putIntegerArrayList(EXTRA_TASK_IDS_TO_REMOVE, taskIdsToRemove)

        outState.putBoolean(EXTRA_PERFORMING_ACTION, performingAction)
        if (selectedRom != null) {
            outState.putParcelable(EXTRA_SELECTED_ROM, selectedRom)
        }
        outState.putString(EXTRA_ACTIVE_ROM_ID, activeRomId)
        outState.putBoolean(EXTRA_SHOWED_SET_KERNEL_WARNING, showedSetKernelWarning)
        outState.putBoolean(EXTRA_HAVE_PERMISSIONS_RESULT, havePermissionsResult)

        outState.putBoolean(EXTRA_IS_LOADING, isLoading)
    }

    /**
     * Create error card on fragment startup
     */
    private fun initErrorCard() {
        errorCardView = activity!!.findViewById(R.id.card_error)
        errorCardView.isClickable = true
        errorCardView.setOnClickListener { reloadRomsState() }
    }

    /**
     * Create CardListView on fragment startup
     */
    private fun initCardList() {
        roms = ArrayList()
        romCardAdapter = RomCardAdapter(roms!!, this)

        cardListView = activity!!.findViewById(R.id.card_list)
        cardListView.setHasFixedSize(true)
        cardListView.adapter = romCardAdapter

        val llm = LinearLayoutManager(activity)
        llm.orientation = LinearLayoutManager.VERTICAL
        cardListView.layoutManager = llm

        setRomListVisibility(false)
        setFabVisibility(false, false)
    }

    /**
     * Set visibility of loading progress spinner
     *
     * @param visible Visibility
     */
    private fun setLoadingSpinnerVisibility(visible: Boolean) {
        isLoading = visible
        swipeRefresh.isRefreshing = visible
    }

    /**
     * Set visibility of the ROMs list
     *
     * @param visible Visibility
     */
    private fun setRomListVisibility(visible: Boolean) {
        cardListView.visibility = if (visible) View.VISIBLE else View.GONE
    }

    /**
     * Set visibility of the mbtool connection error message
     *
     * @param visible Visibility
     */
    private fun setErrorVisibility(visible: Boolean) {
        errorCardView.visibility = if (visible) View.VISIBLE else View.GONE
    }

    /**
     * Set visibility of the FAB for in-app flashing
     *
     * @param visible Visibility
     */
    private fun setFabVisibility(visible: Boolean, animate: Boolean) {
        if (visible) {
            fabFlashZip.show(animate)
        } else {
            fabFlashZip.hide(animate)
        }
    }

    private fun removeCachedTaskId(taskId: Int) {
        if (service != null) {
            service!!.removeCachedTask(taskId)
        } else {
            taskIdsToRemove!!.add(taskId)
        }
    }

    private fun updateKeepScreenOnStatus() {
        // NOTE: May crash on some versions of Android due to a bug where getActivity() returns null
        // after onAttach() has been called, but before onDetach() has been called. It's similar to
        // this bug report, except it happens with android.app.Fragment:
        // https://code.google.com/p/android/issues/detail?id=67519
        if (isAdded) {
            if (performingAction) {
                // Keep screen on
                activity!!.window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
            } else {
                // Don't keep screen on
                activity!!.window.clearFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
            }
        }
    }

    /**
     * {@inheritDoc}
     */
    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        when (requestCode) {
            REQUEST_FLASH_ZIP -> reloadRomsState()
            REQUEST_ROM_DETAILS -> if (data != null && resultCode == Activity.RESULT_OK) {
                val wipedRom = data.getBooleanExtra(
                        RomDetailActivity.EXTRA_RESULT_WIPED_ROM, false)
                if (wipedRom) {
                    reloadRomsState()
                }
            }
            REQUEST_MBTOOL_ERROR -> if (data != null && resultCode == Activity.RESULT_OK) {
                val canRetry = data.getBooleanExtra(
                        MbtoolErrorActivity.EXTRA_RESULT_CAN_RETRY, false)
                if (canRetry) {
                    reloadRomsState()
                }
            }
            else -> super.onActivityResult(requestCode, resultCode, data)
        }
    }

    private fun onGotRomsState(roms: Array<RomInformation>?, currentRom: RomInformation?,
                               activeRomId: String?, kernelStatus: KernelStatus) {
        this.roms!!.clear()

        if (roms != null && roms.isNotEmpty()) {
            Collections.addAll(this.roms!!, *roms)
            setFabVisibility(true, true)
        } else {
            setErrorVisibility(true)
            setFabVisibility(false, true)
        }

        this.currentRom = currentRom

        // Only set activeRomId if it isn't already set (eg. restored from savedInstanceState).
        // reloadRomsState() will set activeRomId back to null so it is refreshed.
        if (this.activeRomId == null) {
            this.activeRomId = activeRomId
        }

        romCardAdapter.activeRomId = this.activeRomId
        romCardAdapter.notifyDataSetChanged()
        updateKeepScreenOnStatus()

        setLoadingSpinnerVisibility(false)
        setRomListVisibility(true)

        if (this.currentRom != null && this.activeRomId != null
                && this.currentRom!!.id == this.activeRomId) {
            // Only show if the current boot image ROM ID matches the booted ROM. Otherwise, the
            // user can switch to another ROM, exit the app, and open it again to trigger the dialog
            if (!showedSetKernelWarning) {
                showedSetKernelWarning = true

                if (kernelStatus === KernelStatus.DIFFERENT) {
                    val dialog = SetKernelNeededDialog.newInstance(
                            this, R.string.kernel_different_from_saved_desc)
                    dialog.show(fragmentManager!!, CONFIRM_DIALOG_SET_KERNEL_NEEDED)
                } else if (kernelStatus === KernelStatus.UNSET) {
                    val dialog = SetKernelNeededDialog.newInstance(
                            this, R.string.kernel_not_set_desc)
                    dialog.show(fragmentManager!!, CONFIRM_DIALOG_SET_KERNEL_NEEDED)
                }
            }
        }
    }

    private fun onSwitchedRom(romId: String, result: SwitchRomResult) {
        // Remove cached task from service
        removeCachedTaskId(taskIdSwitchRom)
        taskIdSwitchRom = -1

        performingAction = false
        updateKeepScreenOnStatus()

        val d = fragmentManager!!.findFragmentByTag(PROGRESS_DIALOG_SWITCH_ROM)
                as GenericProgressDialog?
        d?.dismiss()

        when (result) {
            SwitchRomResult.SUCCEEDED -> {
                createSnackbar(R.string.choose_rom_success, Snackbar.LENGTH_LONG).show()
                Log.d(TAG, "Prior cached boot partition ROM ID was: $activeRomId")
                activeRomId = romId
                Log.d(TAG, "Changing cached boot partition ROM ID to: $activeRomId")
                romCardAdapter.activeRomId = activeRomId
                romCardAdapter.notifyDataSetChanged()
            }
            SwitchRomResult.FAILED ->
                createSnackbar(R.string.choose_rom_failure, Snackbar.LENGTH_LONG).show()
            SwitchRomResult.CHECKSUM_INVALID -> {
                createSnackbar(R.string.choose_rom_checksums_invalid, Snackbar.LENGTH_LONG).show()
                showChecksumIssueDialog(ConfirmChecksumIssueDialog.CHECKSUM_INVALID, romId)
            }
            SwitchRomResult.CHECKSUM_NOT_FOUND -> {
                createSnackbar(R.string.choose_rom_checksums_missing, Snackbar.LENGTH_LONG).show()
                showChecksumIssueDialog(ConfirmChecksumIssueDialog.CHECKSUM_MISSING, romId)
            }
            SwitchRomResult.UNKNOWN_BOOT_PARTITION -> showUnknownBootPartitionDialog()
        }
    }

    private fun onSetKernel(result: SetKernelResult) {
        // Remove cached task from service
        removeCachedTaskId(taskIdSetKernel)
        taskIdSetKernel = -1

        performingAction = false
        updateKeepScreenOnStatus()

        val d = fragmentManager!!.findFragmentByTag(PROGRESS_DIALOG_SET_KERNEL)
                as GenericProgressDialog?
        d?.dismiss()

        when (result) {
            SetKernelResult.SUCCEEDED ->
                createSnackbar(R.string.set_kernel_success, Snackbar.LENGTH_LONG).show()
            SetKernelResult.FAILED ->
                createSnackbar(R.string.set_kernel_failure, Snackbar.LENGTH_LONG).show()
            SetKernelResult.UNKNOWN_BOOT_PARTITION -> showUnknownBootPartitionDialog()
        }
    }

    /**
     * Wrapper function to show a [Snackbar]
     *
     * @param resId String resource
     * @param duration [Snackbar] duration
     *
     * @return [Snackbar] to display
     */
    private fun createSnackbar(@StringRes resId: Int, duration: Int): Snackbar {
        return SnackbarUtils.createSnackbar(activity!!, fabFlashZip, resId, duration)
    }

    private fun showChecksumIssueDialog(issue: Int, romId: String) {
        val d = ConfirmChecksumIssueDialog.newInstanceFromFragment(this, issue, romId)
        d.show(fragmentManager!!, CONFIRM_DIALOG_CHECKSUM_ISSUE)
    }

    private fun showUnknownBootPartitionDialog() {
        val codename = RomUtils.getDeviceCodename(activity!!)
        val message = String.format(getString(R.string.unknown_boot_partition), codename)

        val builder = GenericConfirmDialog.Builder()
        builder.message(message)
        builder.buttonText(R.string.ok)
        builder.build().show(fragmentManager!!, CONFIRM_DIALOG_UNKNOWN_BOOT_PARTITION)
    }

    override fun onSelectedRom(info: RomInformation) {
        selectedRom = info

        switchRom(info.id, false)
    }

    override fun onSelectedRomMenu(info: RomInformation) {
        selectedRom = info

        val intent = Intent(activity, RomDetailActivity::class.java)
        intent.putExtra(RomDetailActivity.EXTRA_ROM_INFO, selectedRom)
        intent.putExtra(RomDetailActivity.EXTRA_BOOTED_ROM_INFO, currentRom)
        intent.putExtra(RomDetailActivity.EXTRA_ACTIVE_ROM_ID, activeRomId)
        startActivityForResult(intent, REQUEST_ROM_DETAILS)
    }

    private fun switchRom(romId: String?, forceChecksumsUpdate: Boolean) {
        performingAction = true
        updateKeepScreenOnStatus()

        val builder = GenericProgressDialog.Builder()
        builder.title(R.string.switching_rom)
        builder.message(R.string.please_wait)
        builder.build().show(fragmentManager!!, PROGRESS_DIALOG_SWITCH_ROM)

        taskIdSwitchRom = service!!.switchRom(romId!!, forceChecksumsUpdate)
        service!!.addCallback(taskIdSwitchRom, callback)
        service!!.enqueueTaskId(taskIdSwitchRom)
    }

    private fun setKernel(info: RomInformation?) {
        performingAction = true
        updateKeepScreenOnStatus()

        val builder = GenericProgressDialog.Builder()
        builder.title(R.string.setting_kernel)
        builder.message(R.string.please_wait)
        builder.build().show(fragmentManager!!, PROGRESS_DIALOG_SET_KERNEL)

        taskIdSetKernel = service!!.setKernel(info!!.id!!)
        service!!.addCallback(taskIdSetKernel, callback)
        service!!.enqueueTaskId(taskIdSetKernel)
    }

    override fun onConfirmSetKernelNeeded() {
        setKernel(currentRom)
    }

    override fun onConfirmChecksumIssue(romId: String) {
        switchRom(romId, true)
    }

    private inner class SwitcherEventCallback : GetRomsStateTaskListener, SwitchRomTaskListener,
            SetKernelTaskListener {
        override fun onGotRomsState(taskId: Int, roms: Array<RomInformation>?,
                                    currentRom: RomInformation?, activeRomId: String?,
                                    kernelStatus: KernelStatus) {
            if (taskId == taskIdGetRomsState) {
                handler.post {
                    this@SwitcherListFragment.onGotRomsState(
                            roms, currentRom, activeRomId, kernelStatus)
                }
            }
        }

        override fun onSwitchedRom(taskId: Int, romId: String, result: SwitchRomResult) {
            if (taskId == taskIdSwitchRom) {
                handler.post { this@SwitcherListFragment.onSwitchedRom(romId, result) }
            }
        }

        override fun onSetKernel(taskId: Int, romId: String, result: SetKernelResult) {
            if (taskId == taskIdSetKernel) {
                handler.post { this@SwitcherListFragment.onSetKernel(result) }
            }
        }

        override fun onMbtoolConnectionFailed(taskId: Int, reason: Reason) {
            handler.post {
                val intent = Intent(activity, MbtoolErrorActivity::class.java)
                intent.putExtra(MbtoolErrorActivity.EXTRA_REASON, reason)
                startActivityForResult(intent, REQUEST_MBTOOL_ERROR)
            }
        }
    }

    companion object {
        val FRAGMENT_TAG: String = SwitcherListFragment::class.java.canonicalName
        private val TAG = SwitcherListFragment::class.java.simpleName

        private const val EXTRA_TASK_ID_GET_ROMS_STATE = "task_id_get_roms_state"
        private const val EXTRA_TASK_ID_SWITCH_ROM = "task_id_switch_rom"
        private const val EXTRA_TASK_ID_SET_KERNEL = "task_id_set_kernel"
        private const val EXTRA_TASK_IDS_TO_REMOVE = "task_ids_to_remove"

        private const val EXTRA_PERFORMING_ACTION = "performing_action"
        private const val EXTRA_SELECTED_ROM = "selected_rom"
        private const val EXTRA_ACTIVE_ROM_ID = "active_rom_id"
        private const val EXTRA_SHOWED_SET_KERNEL_WARNING = "showed_set_kernel_warning"
        private const val EXTRA_HAVE_PERMISSIONS_RESULT = "have_permissions_result"
        private const val EXTRA_IS_LOADING = "is_loading"

        /** Fragment tag for progress dialog for switching ROMS  */
        private val PROGRESS_DIALOG_SWITCH_ROM =
                "${SwitcherListFragment::class.java.canonicalName}.progress.switch_rom"
        /** Fragment tag for progress dialog for setting the kernel  */
        private val PROGRESS_DIALOG_SET_KERNEL =
                "${SwitcherListFragment::class.java.canonicalName}.progress.set_kernel"
        /** Fragment tag for confirmation dialog saying that setting the kernel is needed  */
        private val CONFIRM_DIALOG_SET_KERNEL_NEEDED =
                "${SwitcherListFragment::class.java.canonicalName}.confirm.set_kernel_needed"
        private val CONFIRM_DIALOG_CHECKSUM_ISSUE =
                "${SwitcherListFragment::class.java.canonicalName}.confirm.checksum_issue"
        private val CONFIRM_DIALOG_UNKNOWN_BOOT_PARTITION =
                "${SwitcherListFragment::class.java.canonicalName}.confirm.unknown_boot_partition"
        private val CONFIRM_DIALOG_PERMISSIONS =
                "${SwitcherListFragment::class.java.canonicalName}.confirm.permissions"
        private val YES_NO_DIALOG_PERMISSIONS =
                "${SwitcherListFragment::class.java.canonicalName}.yes_no.permissions"

        /**
         * Request code for storage permissions request
         * (used in [.onRequestPermissionsResult])
         */
        private const val PERMISSIONS_REQUEST_STORAGE = 1

        private const val REQUEST_FLASH_ZIP = 2345
        private const val REQUEST_ROM_DETAILS = 3456
        private const val REQUEST_MBTOOL_ERROR = 4567

        fun newInstance(): SwitcherListFragment {
            return SwitcherListFragment()
        }
    }
}
