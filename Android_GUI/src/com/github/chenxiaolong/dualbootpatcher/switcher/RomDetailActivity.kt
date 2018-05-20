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

package com.github.chenxiaolong.dualbootpatcher.switcher

import android.content.ComponentName
import android.content.Intent
import android.content.ServiceConnection
import android.content.res.ColorStateList
import android.net.Uri
import android.os.Bundle
import android.os.Handler
import android.os.IBinder
import android.os.Looper
import android.support.annotation.StringRes
import android.support.design.widget.CollapsingToolbarLayout
import android.support.design.widget.CoordinatorLayout
import android.support.design.widget.FloatingActionButton
import android.support.design.widget.Snackbar
import android.support.v4.graphics.ColorUtils
import android.support.v7.app.AppCompatActivity
import android.support.v7.graphics.Palette
import android.support.v7.widget.LinearLayoutManager
import android.support.v7.widget.RecyclerView
import android.util.Log
import android.view.Menu
import android.view.MenuItem
import android.widget.ImageView
import android.widget.Toast
import com.github.chenxiaolong.dualbootpatcher.Constants
import com.github.chenxiaolong.dualbootpatcher.FileUtils
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.RomUtils
import com.github.chenxiaolong.dualbootpatcher.RomUtils.CacheWallpaperResult
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation
import com.github.chenxiaolong.dualbootpatcher.SnackbarUtils
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService.ThreadPoolServiceBinder
import com.github.chenxiaolong.dualbootpatcher.Version
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericProgressDialog
import com.github.chenxiaolong.dualbootpatcher.picasso.PaletteGeneratorCallback
import com.github.chenxiaolong.dualbootpatcher.picasso.PaletteGeneratorTransformation
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolErrorActivity
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils.Feature
import com.github.chenxiaolong.dualbootpatcher.socket.exceptions.MbtoolException.Reason
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SetKernelResult
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.SwitchRomResult
import com.github.chenxiaolong.dualbootpatcher.switcher.AddToHomeScreenOptionsDialog.AddToHomeScreenOptionsDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.BackupNameInputDialog.BackupNameInputDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.BackupRestoreTargetsSelectionDialog.BackupRestoreTargetsSelectionDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.CacheRomThumbnailTask.CacheRomThumbnailTaskListener
import com.github.chenxiaolong.dualbootpatcher.switcher.ConfirmChecksumIssueDialog.ConfirmChecksumIssueDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.ConfirmMismatchedSetKernelDialog.ConfirmMismatchedSetKernelDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.RomDetailAdapter.ActionItem
import com.github.chenxiaolong.dualbootpatcher.switcher.RomDetailAdapter.DividerItemDecoration
import com.github.chenxiaolong.dualbootpatcher.switcher.RomDetailAdapter.InfoItem
import com.github.chenxiaolong.dualbootpatcher.switcher.RomDetailAdapter.Item
import com.github.chenxiaolong.dualbootpatcher.switcher.RomDetailAdapter.RomCardItem
import com.github.chenxiaolong.dualbootpatcher.switcher.RomDetailAdapter.RomDetailAdapterListener
import com.github.chenxiaolong.dualbootpatcher.switcher.RomNameInputDialog.RomNameInputDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.SetKernelConfirmDialog.SetKernelConfirmDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.UpdateRamdiskResultDialog.UpdateRamdiskResultDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.WipeTargetsSelectionDialog.WipeTargetsSelectionDialogListener
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.BackupRestoreParams
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.BackupRestoreParams.Action
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.MbtoolAction
import com.github.chenxiaolong.dualbootpatcher.switcher.actions.RomInstallerParams
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CacheWallpaperTask.CacheWallpaperTaskListener
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CreateLauncherTask.CreateLauncherTaskListener
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CreateRamdiskUpdaterTask.CreateRamdiskUpdaterTaskListener
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomDetailsTask.GetRomDetailsTaskListener
import com.github.chenxiaolong.dualbootpatcher.switcher.service.MbtoolTask.MbtoolTaskListener
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SetKernelTask.SetKernelTaskListener
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SwitchRomTask.SwitchRomTaskListener
import com.github.chenxiaolong.dualbootpatcher.switcher.service.WipeRomTask.WipeRomTaskListener
import com.squareup.picasso.Picasso
import mbtool.daemon.v3.MbWipeTarget
import java.io.File
import java.text.SimpleDateFormat
import java.util.ArrayList
import java.util.Date
import java.util.Locale
import kotlin.concurrent.thread

class RomDetailActivity : AppCompatActivity(), RomNameInputDialogListener,
        AddToHomeScreenOptionsDialogListener, CacheRomThumbnailTaskListener,
        RomDetailAdapterListener, ConfirmMismatchedSetKernelDialogListener,
        SetKernelConfirmDialogListener, UpdateRamdiskResultDialogListener,
        WipeTargetsSelectionDialogListener, ConfirmChecksumIssueDialogListener,
        BackupRestoreTargetsSelectionDialogListener, BackupNameInputDialogListener,
        ServiceConnection {
    private lateinit var coordinator: CoordinatorLayout
    private lateinit var collapsing: CollapsingToolbarLayout
    private lateinit var wallpaper: ImageView
    private lateinit var fab: FloatingActionButton
    private lateinit var rv: RecyclerView

    private lateinit var romInfo: RomInformation
    private var bootedRomInfo: RomInformation? = null
    private var activeRomId: String? = null

    private var backupTargets: Array<String>? = null

    private val items = ArrayList<Item>()
    private lateinit var adapter: RomDetailAdapter
    private var updateRamdiskCountdown: Int = 0
    private var updateRamdiskToast: Toast? = null
    private var updateRamdiskResId: Int = 0

    private var taskIdCacheWallpaper = -1
    private var taskIdSwitchRom = -1
    private var taskIdSetKernel = -1
    private var taskIdUpdateRamdisk = -1
    private var taskIdWipeRom = -1
    private var taskIdCreateLauncher = -1
    private var taskIdGetRomDetails = -1

    /** Task IDs to remove  */
    private var taskIdsToRemove: ArrayList<Int>? = ArrayList()

    /** Switcher service  */
    private var service: SwitcherService? = null
    /** Callback for events from the service  */
    private val callback = SwitcherEventCallback()

    /** Handler for processing events from the service on the UI thread  */
    private val handler = Handler(Looper.getMainLooper())

    /** Result intent  */
    private var resultIntent: Intent? = null

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_rom_detail)

        val intent = intent
        romInfo = intent.getParcelableExtra(EXTRA_ROM_INFO)
        bootedRomInfo = intent.getParcelableExtra(EXTRA_BOOTED_ROM_INFO)
        activeRomId = intent.getStringExtra(EXTRA_ACTIVE_ROM_ID)

        if (savedInstanceState != null) {
            taskIdCacheWallpaper = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_CACHE_WALLPAPER)
            taskIdSwitchRom = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_SWITCH_ROM)
            taskIdSetKernel = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_SET_KERNEL)
            taskIdUpdateRamdisk = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_UPDATE_RAMDISK)
            taskIdWipeRom = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_WIPE_ROM)
            taskIdCreateLauncher = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_CREATE_LAUNCHER)
            taskIdGetRomDetails = savedInstanceState.getInt(EXTRA_STATE_TASK_ID_GET_ROM_DETAILS)
            taskIdsToRemove = savedInstanceState.getIntegerArrayList(EXTRA_STATE_TASK_IDS_TO_REMOVE)
            resultIntent = savedInstanceState.getParcelable(EXTRA_STATE_RESULT_INTENT)
            setResult(RESULT_OK, resultIntent)
            backupTargets = savedInstanceState.getStringArray(EXTRA_STATE_BACKUP_TARGETS)
        }

        setSupportActionBar(findViewById(R.id.toolbar))
        supportActionBar!!.setDisplayHomeAsUpEnabled(true)

        coordinator = findViewById(R.id.coordinator)
        collapsing = findViewById(R.id.collapsing_toolbar)
        wallpaper = findViewById(R.id.wallpaper)
        fab = findViewById(R.id.fab)
        rv = findViewById(R.id.main_rv)

        fab.setOnClickListener { switchRom(false) }

        updateTitle()
        updateMbtoolStatus()

        initializeRecyclerView()
        populateRomCardItem()
        populateInfoItems()
        populateActionItems()
    }

    override fun onSaveInstanceState(outState: Bundle?) {
        super.onSaveInstanceState(outState)
        outState!!.putInt(EXTRA_STATE_TASK_ID_CACHE_WALLPAPER, taskIdCacheWallpaper)
        outState.putInt(EXTRA_STATE_TASK_ID_SWITCH_ROM, taskIdSwitchRom)
        outState.putInt(EXTRA_STATE_TASK_ID_SET_KERNEL, taskIdSetKernel)
        outState.putInt(EXTRA_STATE_TASK_ID_UPDATE_RAMDISK, taskIdUpdateRamdisk)
        outState.putInt(EXTRA_STATE_TASK_ID_WIPE_ROM, taskIdWipeRom)
        outState.putInt(EXTRA_STATE_TASK_ID_CREATE_LAUNCHER, taskIdCreateLauncher)
        outState.putInt(EXTRA_STATE_TASK_ID_GET_ROM_DETAILS, taskIdGetRomDetails)
        outState.putIntegerArrayList(EXTRA_STATE_TASK_IDS_TO_REMOVE, taskIdsToRemove)
        outState.putParcelable(EXTRA_STATE_RESULT_INTENT, resultIntent)
        if (backupTargets != null) {
            outState.putStringArray(EXTRA_STATE_BACKUP_TARGETS, backupTargets)
        }
    }

    override fun onStart() {
        super.onStart()

        // Start and bind to the service
        val intent = Intent(this, SwitcherService::class.java)
        bindService(intent, this, BIND_AUTO_CREATE)
        startService(intent)
    }

    override fun onStop() {
        super.onStop()

        if (isFinishing) {
            if (taskIdCacheWallpaper >= 0) {
                removeCachedTaskId(taskIdCacheWallpaper)
                taskIdCacheWallpaper = -1
            }
            if (taskIdGetRomDetails >= 0) {
                removeCachedTaskId(taskIdGetRomDetails)
                taskIdGetRomDetails = -1
            }
        }

        // If we connected to the service and registered the callback, now we unregister it
        if (service != null) {
            if (taskIdCacheWallpaper >= 0) {
                service!!.removeCallback(taskIdCacheWallpaper, callback)
            }
            if (taskIdSwitchRom >= 0) {
                service!!.removeCallback(taskIdSwitchRom, callback)
            }
            if (taskIdSetKernel >= 0) {
                service!!.removeCallback(taskIdSetKernel, callback)
            }
            if (taskIdUpdateRamdisk >= 0) {
                service!!.removeCallback(taskIdUpdateRamdisk, callback)
            }
            if (taskIdWipeRom >= 0) {
                service!!.removeCallback(taskIdWipeRom, callback)
            }
            if (taskIdCreateLauncher >= 0) {
                service!!.removeCallback(taskIdCreateLauncher, callback)
            }
            if (taskIdGetRomDetails >= 0) {
                service!!.removeCallback(taskIdGetRomDetails, callback)
            }
        }

        // Unbind from our service
        unbindService(this)
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
        for (taskId in taskIdsToRemove!!) {
            this.service!!.removeCachedTask(taskId)
        }
        taskIdsToRemove!!.clear()

        if (taskIdCacheWallpaper < 0) {
            taskIdCacheWallpaper = this.service!!.cacheWallpaper(romInfo)
            this.service!!.addCallback(taskIdCacheWallpaper, callback)
            this.service!!.enqueueTaskId(taskIdCacheWallpaper)
        } else {
            this.service!!.addCallback(taskIdCacheWallpaper, callback)
        }

        if (taskIdGetRomDetails < 0) {
            taskIdGetRomDetails = this.service!!.getRomDetails(romInfo)
            this.service!!.addCallback(taskIdGetRomDetails, callback)
            this.service!!.enqueueTaskId(taskIdGetRomDetails)
        } else {
            this.service!!.addCallback(taskIdGetRomDetails, callback)
        }

        if (taskIdCreateLauncher >= 0) {
            this.service!!.addCallback(taskIdCreateLauncher, callback)
        }

        if (taskIdSwitchRom >= 0) {
            this.service!!.addCallback(taskIdSwitchRom, callback)
        }

        if (taskIdSetKernel >= 0) {
            this.service!!.addCallback(taskIdSetKernel, callback)
        }

        if (taskIdUpdateRamdisk >= 0) {
            this.service!!.addCallback(taskIdUpdateRamdisk, callback)
        }

        if (taskIdWipeRom >= 0) {
            this.service!!.addCallback(taskIdWipeRom, callback)
        }
    }

    override fun onServiceDisconnected(name: ComponentName) {
        service = null
    }

    private fun removeCachedTaskId(taskId: Int) {
        if (service != null) {
            service!!.removeCachedTask(taskId)
        } else {
            taskIdsToRemove!!.add(taskId)
        }
    }

    private fun updateTitle() {
        collapsing.title = romInfo.id
    }

    private fun updateMbtoolStatus() {
        // There is currently no way to determine the mbtool version of other ROMs
        if (bootedRomInfo != null && bootedRomInfo!!.id == romInfo.id) {
            val minAppSharing = MbtoolUtils.getMinimumRequiredVersion(Feature.APP_SHARING)
            val minDaemon = MbtoolUtils.getMinimumRequiredVersion(Feature.DAEMON)
            val version = MbtoolUtils.getSystemMbtoolVersion(this)

            if (version >= minAppSharing && version >= minDaemon) {
                updateRamdiskCountdown = FORCE_RAMDISK_UPDATE_TAPS
                updateRamdiskResId = R.string.update_ramdisk_up_to_date_desc
            } else {
                updateRamdiskCountdown = -1
                updateRamdiskResId = if (version == Version.from("0.0.0")) {
                    R.string.update_ramdisk_missing_desc
                } else {
                    R.string.update_ramdisk_outdated_desc
                }
            }
        } else {
            updateRamdiskCountdown = -1
            updateRamdiskResId = R.string.unknown
        }
    }

    private fun initializeRecyclerView() {
        adapter = RomDetailAdapter(items, this)

        rv.setHasFixedSize(true)
        rv.adapter = adapter
        rv.layoutManager = LinearLayoutManager(this)
        rv.addItemDecoration(DividerItemDecoration(this))
    }

    private fun populateRomCardItem() {
        val item = RomCardItem(romInfo)
        items.add(item)
    }

    private fun populateInfoItems() {
        items.add(InfoItem(INFO_SLOT,
                getString(R.string.rom_details_info_slot),
                romInfo.id ?: ""))
        items.add(InfoItem(INFO_VERSION,
                getString(R.string.rom_details_info_version),
                romInfo.version ?: ""))
        items.add(InfoItem(INFO_BUILD,
                getString(R.string.rom_details_info_build),
                romInfo.build ?: ""))
        items.add(InfoItem(INFO_MBTOOL_STATUS,
                getString(R.string.rom_details_info_mbtool_status),
                getString(updateRamdiskResId)))
        items.add(InfoItem(INFO_APPS_COUNTS,
                getString(R.string.rom_details_info_apps_counts),
                getString(R.string.rom_details_info_calculating)))
        items.add(InfoItem(INFO_SYSTEM_PATH,
                getString(R.string.rom_details_info_system_path),
                romInfo.systemPath ?: ""))
        items.add(InfoItem(INFO_CACHE_PATH,
                getString(R.string.rom_details_info_cache_path),
                romInfo.cachePath ?: ""))
        items.add(InfoItem(INFO_DATA_PATH,
                getString(R.string.rom_details_info_data_path),
                romInfo.dataPath ?: ""))
        items.add(InfoItem(INFO_SYSTEM_SIZE,
                getString(R.string.rom_details_info_system_size),
                getString(R.string.rom_details_info_calculating)))
        items.add(InfoItem(INFO_CACHE_SIZE,
                getString(R.string.rom_details_info_cache_size),
                getString(R.string.rom_details_info_calculating)))
        items.add(InfoItem(INFO_DATA_SIZE,
                getString(R.string.rom_details_info_data_size),
                getString(R.string.rom_details_info_calculating)))
    }

    private fun populateActionItems() {
        items.add(ActionItem(ACTION_UPDATE_RAMDISK,
                R.drawable.rom_details_action_update_ramdisk,
                getString(R.string.rom_menu_update_ramdisk)))
        items.add(ActionItem(ACTION_SET_KERNEL,
                R.drawable.rom_details_action_set_kernel,
                getString(R.string.rom_menu_set_kernel)))
        items.add(ActionItem(ACTION_BACKUP,
                R.drawable.rom_details_action_backup,
                getString(R.string.rom_menu_backup_rom)))
        items.add(ActionItem(ACTION_ADD_TO_HOME_SCREEN,
                R.drawable.rom_details_action_home,
                getString(R.string.rom_menu_add_to_home_screen)))
        items.add(ActionItem(ACTION_WIPE_ROM,
                R.drawable.rom_details_action_wipe,
                getString(R.string.rom_menu_wipe_rom)))
    }

    private fun setInfoItem(id: Int, newValue: String) {
        for (i in items.indices) {
            val item = items[i]
            if (item is InfoItem) {
                if (item.id == id) {
                    item.value = newValue
                }
                adapter.notifyItemChanged(i)
            }
        }
    }

    private fun reloadThumbnail() {
        for (i in items.indices) {
            val item = items[i]
            if (item is RomCardItem) {
                adapter.notifyItemChanged(i)
            }
        }
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        menu.add(0, MENU_EDIT_NAME, Menu.NONE, R.string.rom_menu_edit_name)
        menu.add(0, MENU_CHANGE_IMAGE, Menu.NONE, R.string.rom_menu_change_image)
        menu.add(0, MENU_RESET_IMAGE, Menu.NONE, R.string.rom_menu_reset_image)
        return true
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        when (item.itemId) {
            android.R.id.home -> {
                finish()
                return true
            }
            MENU_EDIT_NAME -> {
                onSelectedEditName()
                return true
            }
            MENU_CHANGE_IMAGE -> {
                onSelectedChangeImage()
                return true
            }
            MENU_RESET_IMAGE -> {
                onSelectedResetImage()
                return true
            }
        }
        return super.onOptionsItemSelected(item)
    }

    public override fun onActivityResult(request: Int, result: Int, data: Intent?) {
        when (request) {
            REQUEST_IMAGE -> if (data != null && result == RESULT_OK) {
                CacheRomThumbnailTask(applicationContext, romInfo, data.data!!, this).execute()
            }
            REQUEST_MBTOOL_ERROR -> if (data != null && result == RESULT_OK) {
                finish()
            }
            else -> super.onActivityResult(request, result, data)
        }
    }

    private fun onSelectedEditName() {
        val d = RomNameInputDialog.newInstanceFromActivity(romInfo)
        d.show(supportFragmentManager, CONFIRM_DIALOG_ROM_NAME)
    }

    private fun onSelectedChangeImage() {
        val intent = Intent()
        intent.type = "image/*"
        intent.action = Intent.ACTION_GET_CONTENT
        intent.addCategory(Intent.CATEGORY_OPENABLE)
        startActivityForResult(intent, REQUEST_IMAGE)
    }

    private fun onSelectedResetImage() {
        val f = File(romInfo.thumbnailPath!!)
        if (f.isFile) {
            f.delete()
        }

        reloadThumbnail()
    }

    private fun onSelectedAddToHomeScreen() {
        val d = AddToHomeScreenOptionsDialog.newInstanceFromActivity(romInfo)
        d.show(supportFragmentManager, CONFIRM_DIALOG_ADD_TO_HOME_SCREEN)
    }

    private fun onSelectedUpdateRamdisk() {
        // Act like the Android settings app :)
        if (updateRamdiskCountdown > 0) {
            if (updateRamdiskCountdown == FORCE_RAMDISK_UPDATE_TAPS) {
                createSnackbar(updateRamdiskResId, Snackbar.LENGTH_LONG).show()
            }

            updateRamdiskCountdown--
            if (updateRamdiskCountdown == 0) {
                if (updateRamdiskToast != null) {
                    updateRamdiskToast!!.cancel()
                }
                updateRamdiskToast = Toast.makeText(this, R.string.force_updating_ramdisk,
                        Toast.LENGTH_LONG)
                updateRamdiskToast!!.show()

                createRamdiskUpdater()

                updateRamdiskCountdown = FORCE_RAMDISK_UPDATE_TAPS
            } else if (updateRamdiskCountdown > 0
                    && updateRamdiskCountdown < FORCE_RAMDISK_UPDATE_TAPS - 2) {
                if (updateRamdiskToast != null) {
                    updateRamdiskToast!!.cancel()
                }
                updateRamdiskToast = Toast.makeText(this, resources.getQuantityString(
                        R.plurals.force_update_ramdisk_countdown,
                        updateRamdiskCountdown, updateRamdiskCountdown),
                        Toast.LENGTH_SHORT)
                updateRamdiskToast!!.show()
            }
        } else if (updateRamdiskCountdown < 0) {
            // Already enabled
            if (updateRamdiskToast != null) {
                updateRamdiskToast!!.cancel()
            }

            createRamdiskUpdater()
        }
    }

    private fun onSelectedSetKernel() {
        // Ask for confirmation
        if (activeRomId != null && activeRomId != romInfo.id) {
            val d = ConfirmMismatchedSetKernelDialog.newInstanceFromActivity(
                    activeRomId!!, romInfo.id!!)
            d.show(supportFragmentManager, CONFIRM_DIALOG_MISMATCHED_KERNEL)
        } else {
            val d = SetKernelConfirmDialog.newInstanceFromActivity(romInfo)
            d.show(supportFragmentManager, CONFIRM_DIALOG_SET_KERNEL)
        }
    }

    private fun onSelectedBackupRom() {
        val d = BackupRestoreTargetsSelectionDialog.newInstanceFromActivity(Action.BACKUP)
        d.show(supportFragmentManager, CONFIRM_DIALOG_BACKUP_RESTORE_TARGETS)
    }

    private fun onSelectedWipeRom() {
        if (bootedRomInfo != null && bootedRomInfo!!.id == romInfo.id) {
            createSnackbar(R.string.wipe_rom_no_wipe_current_rom, Snackbar.LENGTH_LONG).show()
        } else {
            val d = WipeTargetsSelectionDialog.newInstanceFromActivity()
            d.show(supportFragmentManager, CONFIRM_DIALOG_WIPE_TARGETS)
        }
    }

    override fun onRomNameChanged(newName: String?) {
        romInfo.name = newName

        thread(start = true) {
            RomUtils.saveConfig(romInfo)
        }

        updateTitle()
    }

    override fun onConfirmAddToHomeScreenOptions(info: RomInformation?, reboot: Boolean) {
        taskIdCreateLauncher = service!!.createLauncher(info!!, reboot)
        service!!.addCallback(taskIdCreateLauncher, callback)
        service!!.enqueueTaskId(taskIdCreateLauncher)
    }

    override fun onConfirmMismatchedSetKernel() {
        setKernel()
    }

    override fun onConfirmChecksumIssue(romId: String) {
        switchRom(true)
    }

    override fun onConfirmSetKernel() {
        setKernel()
    }

    override fun onConfirmUpdatedRamdisk(reboot: Boolean) {
        if (reboot) {
            val builder = GenericProgressDialog.Builder()
            builder.message(R.string.please_wait)
            builder.build().show(supportFragmentManager, PROGRESS_DIALOG_REBOOT)

            thread(start = true) {
                SwitcherUtils.reboot(applicationContext)
            }
        }
    }

    override fun onSelectedBackupRestoreTargets(targets: Array<String>) {
        if (targets.isEmpty()) {
            createSnackbar(R.string.br_no_target_selected, Snackbar.LENGTH_LONG).show()
            return
        }

        backupTargets = targets

        val df = SimpleDateFormat("yyyy.MM.dd-HH.mm.ss", Locale.US)
        val suggestedName = "${df.format(Date())}_${romInfo.id}"

        // Prompt for back up name
        val d = BackupNameInputDialog.newInstanceFromActivity(suggestedName)
        d.show(supportFragmentManager, INPUT_DIALOG_BACKUP_NAME)
    }

    override fun onSelectedBackupName(name: String) {
        val prefs = getSharedPreferences("settings", 0)

        val backupDirUri = Uri.parse(prefs.getString(Constants.Preferences.BACKUP_DIRECTORY_URI,
                Constants.Defaults.BACKUP_DIRECTORY_URI))

        val params = BackupRestoreParams(
                Action.BACKUP, romInfo.id!!, backupTargets!!, name, backupDirUri, false)
        val action = MbtoolAction(params)

        // Start backup
        val intent = Intent(this, MbtoolTaskOutputActivity::class.java)
        intent.putExtra(MbtoolTaskOutputFragment.PARAM_ACTIONS, arrayOf(action))
        startActivity(intent)
    }

    override fun onSelectedWipeTargets(targets: ShortArray) {
        if (targets.isEmpty()) {
            createSnackbar(R.string.wipe_rom_none_selected, Snackbar.LENGTH_LONG).show()
            return
        }

        wipeRom(targets)
    }

    override fun onRomThumbnailCached(info: RomInformation) {
        reloadThumbnail()
    }

    override fun onActionItemSelected(item: ActionItem) {
        when (item.id) {
            ACTION_UPDATE_RAMDISK -> onSelectedUpdateRamdisk()
            ACTION_SET_KERNEL -> onSelectedSetKernel()
            ACTION_BACKUP -> onSelectedBackupRom()
            ACTION_ADD_TO_HOME_SCREEN -> onSelectedAddToHomeScreen()
            ACTION_WIPE_ROM -> onSelectedWipeRom()
            else -> throw IllegalStateException("Invalid action ID: ${item.id}")
        }
    }

    private fun onCachedWallpaper(result: CacheWallpaperResult) {
        val wallpaperFile = File(romInfo.wallpaperPath!!)

        if (result === CacheWallpaperResult.USES_LIVE_WALLPAPER) {
            Toast.makeText(this, "Cannot preview live wallpaper", Toast.LENGTH_SHORT).show()
        }

        val creator = if (result === CacheWallpaperResult.UP_TO_DATE
                || result === CacheWallpaperResult.UPDATED) {
            Picasso.get().load(wallpaperFile).error(R.drawable.material)
        } else {
            Picasso.get().load(R.drawable.material)
        }

        creator
                .transform(PaletteGeneratorTransformation.instance)
                .into(wallpaper, object : PaletteGeneratorCallback(wallpaper) {
                    override fun onObtainedPalette(palette: Palette?) {
                        if (palette == null) {
                            return
                        }

                        var swatch = palette.mutedSwatch
                        if (swatch == null) {
                            swatch = palette.darkVibrantSwatch
                        }
                        if (swatch != null) {
                            // Status bar should be slightly darker
                            val hsl = swatch.hsl
                            hsl[2] -= 0.05f
                            if (hsl[2] < 0) {
                                hsl[2] = 0f
                            }

                            collapsing.setContentScrimColor(swatch.rgb)
                            collapsing.setStatusBarScrimColor(ColorUtils.HSLToColor(hsl))
                            fab.backgroundTintList = ColorStateList.valueOf(swatch.rgb)
                        }
                        //Swatch lightVibrant = palette.getLightVibrantSwatch()
                        //if (lightVibrant != null) {
                        //    collapsingToolbar.setCollapsedTitleTextColor(lightVibrant.getRgb())
                        //    collapsingToolbar.setExpandedTitleColor(lightVibrant.getRgb())
                        //}
                    }
                })
    }

    private fun onSwitchedRom(result: SwitchRomResult) {
        // Remove cached task from service
        removeCachedTaskId(taskIdSwitchRom)
        taskIdSwitchRom = -1

        val d = supportFragmentManager.findFragmentByTag(PROGRESS_DIALOG_SWITCH_ROM)
                as GenericProgressDialog?
        d?.dismiss()

        when (result) {
            SwitchRomResult.SUCCEEDED -> {
                createSnackbar(R.string.choose_rom_success, Snackbar.LENGTH_LONG).show()
                Log.d(TAG, "Prior cached boot partition ROM ID was: $activeRomId")
                activeRomId = romInfo.id
                Log.d(TAG, "Changing cached boot partition ROM ID to: $activeRomId")
            }
            SwitchRomResult.FAILED ->
                createSnackbar(R.string.choose_rom_failure, Snackbar.LENGTH_LONG).show()
            SwitchRomResult.CHECKSUM_INVALID -> {
                createSnackbar(R.string.choose_rom_checksums_invalid, Snackbar.LENGTH_LONG).show()
                showChecksumIssueDialog(ConfirmChecksumIssueDialog.CHECKSUM_INVALID, romInfo.id!!)
            }
            SwitchRomResult.CHECKSUM_NOT_FOUND -> {
                createSnackbar(R.string.choose_rom_checksums_missing, Snackbar.LENGTH_LONG).show()
                showChecksumIssueDialog(ConfirmChecksumIssueDialog.CHECKSUM_MISSING, romInfo.id!!)
            }
            SwitchRomResult.UNKNOWN_BOOT_PARTITION -> showUnknownBootPartitionDialog()
        }
    }

    private fun onSetKernel(result: SetKernelResult) {
        // Remove cached task from service
        removeCachedTaskId(taskIdSetKernel)
        taskIdSetKernel = -1

        val d = supportFragmentManager.findFragmentByTag(PROGRESS_DIALOG_SET_KERNEL)
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

    private fun onCreatedRamdiskUpdater(path: String?) {
        // Remove cached task from service
        removeCachedTaskId(taskIdUpdateRamdisk)
        taskIdUpdateRamdisk = -1

        if (path == null) {
            hideRamdiskUpdateProgressDialog()
            showRamdiskUpdateCompletionDialog(false)
        } else {
            flashRamdiskUpdater(path)
        }
    }

    private fun onFlashedRamdiskUpdater(success: Boolean) {
        // Remove cached task from service
        removeCachedTaskId(taskIdUpdateRamdisk)
        taskIdUpdateRamdisk = -1

        hideRamdiskUpdateProgressDialog()
        showRamdiskUpdateCompletionDialog(success)
    }

    private fun onWipedRom(succeeded: ShortArray?, failed: ShortArray?) {
        // Remove cached task from service
        removeCachedTaskId(taskIdWipeRom)
        taskIdWipeRom = -1

        val d = supportFragmentManager.findFragmentByTag(PROGRESS_DIALOG_WIPE_ROM)
                as GenericProgressDialog?
        d?.dismiss()

        if (succeeded == null || failed == null) {
            createSnackbar(R.string.wipe_rom_initiate_fail, Snackbar.LENGTH_LONG).show()
        } else {
            val sb = StringBuilder()

            if (succeeded.isNotEmpty()) {
                sb.append(String.format(getString(R.string.wipe_rom_successfully_wiped),
                        targetsToString(succeeded)))
                if (failed.isNotEmpty()) {
                    sb.append("\n")
                }
            }
            if (failed.isNotEmpty()) {
                sb.append(String.format(getString(R.string.wipe_rom_failed_to_wipe),
                        targetsToString(failed)))
            }

            createSnackbar(sb.toString(), Snackbar.LENGTH_LONG).show()
        }

        // Alert parent activity
        if (resultIntent == null) {
            resultIntent = Intent()
        }
        resultIntent!!.putExtra(EXTRA_RESULT_WIPED_ROM, true)
        setResult(RESULT_OK, resultIntent)
    }

    private fun onCreatedLauncher() {
        // Remove cached task from service
        removeCachedTaskId(taskIdCreateLauncher)
        taskIdCreateLauncher = -1

        createSnackbar(String.format(getString(R.string.successfully_created_launcher),
                romInfo.name), Snackbar.LENGTH_LONG).show()
    }

    private fun onHaveSystemSize(success: Boolean, size: Long) {
        if (success) {
            setInfoItem(INFO_SYSTEM_SIZE, FileUtils.toHumanReadableSize(this, size, 2))
        } else {
            setInfoItem(INFO_SYSTEM_SIZE, getString(R.string.unknown))
        }
    }

    private fun onHaveCacheSize(success: Boolean, size: Long) {
        if (success) {
            setInfoItem(INFO_CACHE_SIZE, FileUtils.toHumanReadableSize(this, size, 2))
        } else {
            setInfoItem(INFO_CACHE_SIZE, getString(R.string.unknown))
        }
    }

    private fun onHaveDataSize(success: Boolean, size: Long) {
        if (success) {
            setInfoItem(INFO_DATA_SIZE, FileUtils.toHumanReadableSize(this, size, 2))
        } else {
            setInfoItem(INFO_DATA_SIZE, getString(R.string.unknown))
        }
    }

    private fun onHavePackagesCounts(success: Boolean, systemPackages: Int, updatedPackages: Int,
                                     userPackages: Int) {
        if (success) {
            setInfoItem(INFO_APPS_COUNTS, getString(R.string.rom_details_info_apps_counts_value,
                    systemPackages, updatedPackages, userPackages))
        } else {
            setInfoItem(INFO_APPS_COUNTS, getString(R.string.unknown))
        }
    }

    private fun switchRom(forceChecksumsUpdate: Boolean) {
        val builder = GenericProgressDialog.Builder()
        builder.title(R.string.switching_rom)
        builder.message(R.string.please_wait)
        builder.build().show(supportFragmentManager, PROGRESS_DIALOG_SWITCH_ROM)

        taskIdSwitchRom = service!!.switchRom(romInfo.id!!, forceChecksumsUpdate)
        service!!.addCallback(taskIdSwitchRom, callback)
        service!!.enqueueTaskId(taskIdSwitchRom)
    }

    private fun setKernel() {
        val builder = GenericProgressDialog.Builder()
        builder.title(R.string.setting_kernel)
        builder.message(R.string.please_wait)
        builder.build().show(supportFragmentManager, PROGRESS_DIALOG_SET_KERNEL)

        taskIdSetKernel = service!!.setKernel(romInfo.id!!)
        service!!.addCallback(taskIdSetKernel, callback)
        service!!.enqueueTaskId(taskIdSetKernel)
    }

    private fun createRamdiskUpdater() {
        showRamdiskUpdateProgressDialog()

        taskIdUpdateRamdisk = service!!.createRamdiskUpdater(romInfo)
        service!!.addCallback(taskIdUpdateRamdisk, callback)
        service!!.enqueueTaskId(taskIdUpdateRamdisk)
    }

    private fun flashRamdiskUpdater(path: String) {
        val file = File(path)
        val params = RomInstallerParams(Uri.fromFile(file), file.name, romInfo.id!!)
        params.skipMounts = true
        params.allowOverwrite = true

        taskIdUpdateRamdisk = service!!.mbtoolActions(arrayOf(MbtoolAction(params)))
        service!!.addCallback(taskIdUpdateRamdisk, callback)
        service!!.enqueueTaskId(taskIdUpdateRamdisk)
    }

    private fun wipeRom(targets: ShortArray) {
        val builder = GenericProgressDialog.Builder()
        builder.title(R.string.wiping_targets)
        builder.message(R.string.please_wait)
        builder.build().show(supportFragmentManager, PROGRESS_DIALOG_WIPE_ROM)

        taskIdWipeRom = service!!.wipeRom(romInfo.id!!, targets)
        service!!.addCallback(taskIdWipeRom, callback)
        service!!.enqueueTaskId(taskIdWipeRom)
    }

    private fun showChecksumIssueDialog(issue: Int, romId: String) {
        val d = ConfirmChecksumIssueDialog.newInstanceFromActivity(issue, romId)
        d.show(supportFragmentManager, CONFIRM_DIALOG_CHECKSUM_ISSUE)
    }

    private fun showUnknownBootPartitionDialog() {
        val codename = RomUtils.getDeviceCodename(this)
        val message = String.format(getString(R.string.unknown_boot_partition), codename)

        val builder = GenericConfirmDialog.Builder()
        builder.message(message)
        builder.buttonText(R.string.ok)
        builder.build().show(supportFragmentManager, CONFIRM_DIALOG_UNKNOWN_BOOT_PARTITION)
    }

    private fun showRamdiskUpdateProgressDialog() {
        val builder = GenericProgressDialog.Builder()
        builder.message(R.string.please_wait)
        builder.build().show(supportFragmentManager, PROGRESS_DIALOG_UPDATE_RAMDISK)
    }

    private fun hideRamdiskUpdateProgressDialog() {
        val d = supportFragmentManager.findFragmentByTag(PROGRESS_DIALOG_UPDATE_RAMDISK)
                as GenericProgressDialog?
        d?.dismiss()
    }

    private fun showRamdiskUpdateCompletionDialog(success: Boolean) {
        val isCurrentRom = bootedRomInfo != null && bootedRomInfo!!.id == romInfo.id
        val dialog = UpdateRamdiskResultDialog.newInstanceFromActivity(success, isCurrentRom)
        dialog.show(supportFragmentManager, CONFIRM_DIALOG_UPDATED_RAMDISK)
    }

    private fun createSnackbar(text: String, duration: Int): Snackbar {
        return SnackbarUtils.createSnackbar(this, coordinator, text, duration)
    }

    private fun createSnackbar(@StringRes resId: Int, duration: Int): Snackbar {
        return SnackbarUtils.createSnackbar(this, coordinator, resId, duration)
    }

    private fun targetsToString(targets: ShortArray): String {
        val sb = StringBuilder()

        for (i in targets.indices) {
            if (i != 0) {
                sb.append(", ")
            }

            when {
                targets[i] == MbWipeTarget.SYSTEM ->
                    sb.append(getString(R.string.wipe_target_system))
                targets[i] == MbWipeTarget.CACHE ->
                    sb.append(getString(R.string.wipe_target_cache))
                targets[i] == MbWipeTarget.DATA ->
                    sb.append(getString(R.string.wipe_target_data))
                targets[i] == MbWipeTarget.DALVIK_CACHE ->
                    sb.append(getString(R.string.wipe_target_dalvik_cache))
                targets[i] == MbWipeTarget.MULTIBOOT ->
                    sb.append(getString(R.string.wipe_target_multiboot_files))
            }
        }

        return sb.toString()
    }

    private inner class SwitcherEventCallback : CacheWallpaperTaskListener, SwitchRomTaskListener,
            SetKernelTaskListener, CreateRamdiskUpdaterTaskListener, WipeRomTaskListener,
            CreateLauncherTaskListener, GetRomDetailsTaskListener, MbtoolTaskListener {
        override fun onCachedWallpaper(taskId: Int, romInfo: RomInformation,
                                       result: CacheWallpaperResult) {
            if (taskId == taskIdCacheWallpaper) {
                handler.post { this@RomDetailActivity.onCachedWallpaper(result) }
            }
        }

        override fun onSwitchedRom(taskId: Int, romId: String, result: SwitchRomResult) {
            if (taskId == taskIdSwitchRom) {
                handler.post { this@RomDetailActivity.onSwitchedRom(result) }
            }
        }

        override fun onSetKernel(taskId: Int, romId: String, result: SetKernelResult) {
            if (taskId == taskIdSetKernel) {
                handler.post { this@RomDetailActivity.onSetKernel(result) }
            }
        }

        override fun onCreatedRamdiskUpdater(taskId: Int, romInfo: RomInformation, path: String?) {
            if (taskId == taskIdUpdateRamdisk) {
                handler.post { this@RomDetailActivity.onCreatedRamdiskUpdater(path) }
            }
        }

        override fun onWipedRom(taskId: Int, romId: String, succeeded: ShortArray, failed: ShortArray) {
            if (taskId == taskIdWipeRom) {
                handler.post { this@RomDetailActivity.onWipedRom(succeeded, failed) }
            }
        }

        override fun onCreatedLauncher(taskId: Int, romInfo: RomInformation) {
            if (taskId == taskIdCreateLauncher) {
                handler.post { this@RomDetailActivity.onCreatedLauncher() }
            }
        }

        override fun onRomDetailsGotSystemSize(taskId: Int, romInfo: RomInformation,
                                               success: Boolean, size: Long) {
            if (taskId == taskIdGetRomDetails) {
                handler.post { onHaveSystemSize(success, size) }
            }
        }

        override fun onRomDetailsGotCacheSize(taskId: Int, romInfo: RomInformation,
                                              success: Boolean, size: Long) {
            if (taskId == taskIdGetRomDetails) {
                handler.post { onHaveCacheSize(success, size) }
            }
        }

        override fun onRomDetailsGotDataSize(taskId: Int, romInfo: RomInformation,
                                             success: Boolean, size: Long) {
            if (taskId == taskIdGetRomDetails) {
                handler.post { onHaveDataSize(success, size) }
            }
        }

        override fun onRomDetailsGotPackagesCounts(taskId: Int, romInfo: RomInformation,
                                                   success: Boolean, systemPackages: Int,
                                                   updatedPackages: Int, userPackages: Int) {
            if (taskId == taskIdGetRomDetails) {
                handler.post {
                    onHavePackagesCounts(success, systemPackages, updatedPackages, userPackages)
                }
            }
        }

        override fun onRomDetailsFinished(taskId: Int, romInfo: RomInformation) {
            // Ignore
        }

        override fun onMbtoolConnectionFailed(taskId: Int, reason: Reason) {
            handler.post {
                val intent = Intent(this@RomDetailActivity, MbtoolErrorActivity::class.java)
                intent.putExtra(MbtoolErrorActivity.EXTRA_REASON, reason)
                startActivityForResult(intent, REQUEST_MBTOOL_ERROR)
            }
        }

        override fun onMbtoolTasksCompleted(taskId: Int, actions: Array<MbtoolAction>,
                                            attempted: Int, succeeded: Int) {
            if (taskId == taskIdUpdateRamdisk) {
                handler.post { onFlashedRamdiskUpdater(attempted == succeeded) }
            }
        }

        override fun onCommandOutput(taskId: Int, line: String) {
            Log.d(TAG, "[Ramdisk update flasher] $line")
        }
    }

    companion object {
        private val TAG = RomDetailActivity::class.java.simpleName

        private const val MENU_EDIT_NAME = Menu.FIRST
        private const val MENU_CHANGE_IMAGE = Menu.FIRST + 1
        private const val MENU_RESET_IMAGE = Menu.FIRST + 2

        private const val INFO_SLOT = 1
        private const val INFO_VERSION = 2
        private const val INFO_BUILD = 3
        private const val INFO_MBTOOL_STATUS = 4
        private const val INFO_APPS_COUNTS = 5
        private const val INFO_SYSTEM_PATH = 6
        private const val INFO_CACHE_PATH = 7
        private const val INFO_DATA_PATH = 8
        private const val INFO_SYSTEM_SIZE = 9
        private const val INFO_CACHE_SIZE = 10
        private const val INFO_DATA_SIZE = 11

        private const val ACTION_UPDATE_RAMDISK = 1
        private const val ACTION_SET_KERNEL = 2
        private const val ACTION_BACKUP = 3
        private const val ACTION_ADD_TO_HOME_SCREEN = 4
        private const val ACTION_WIPE_ROM = 5

        private val PROGRESS_DIALOG_SWITCH_ROM =
                "${RomDetailActivity::class.java.canonicalName}.progress.switch_rom"
        private val PROGRESS_DIALOG_SET_KERNEL =
                "${RomDetailActivity::class.java.canonicalName}.progress.set_kernel"
        private val PROGRESS_DIALOG_UPDATE_RAMDISK =
                "${RomDetailActivity::class.java.canonicalName}.progress.update_ramdisk"
        private val PROGRESS_DIALOG_WIPE_ROM =
                "${RomDetailActivity::class.java.canonicalName}.progress.wipe_rom"
        private val PROGRESS_DIALOG_REBOOT =
                "${RomDetailActivity::class.java.canonicalName}.progress.reboot"
        private val CONFIRM_DIALOG_UPDATED_RAMDISK =
                "${RomDetailActivity::class.java.canonicalName}.confirm.updated_ramdisk"
        private val CONFIRM_DIALOG_ADD_TO_HOME_SCREEN =
                "${RomDetailActivity::class.java.canonicalName}.confirm.add_to_home_screen"
        private val CONFIRM_DIALOG_ROM_NAME =
                "${RomDetailActivity::class.java.canonicalName}.confirm.rom_name"
        private val CONFIRM_DIALOG_SET_KERNEL =
                "${RomDetailActivity::class.java.canonicalName}.confirm.set_kernel"
        private val CONFIRM_DIALOG_MISMATCHED_KERNEL =
                "${RomDetailActivity::class.java.canonicalName}.confirm.mismatched_kernel"
        private val CONFIRM_DIALOG_BACKUP_RESTORE_TARGETS =
                "${RomDetailActivity::class.java.canonicalName}.confirm.backup_restore_targets"
        private val CONFIRM_DIALOG_WIPE_TARGETS =
                "${RomDetailActivity::class.java.canonicalName}.confirm.wipe_targets"
        private val CONFIRM_DIALOG_CHECKSUM_ISSUE =
                "${RomDetailActivity::class.java.canonicalName}.confirm.checksum_issue"
        private val CONFIRM_DIALOG_UNKNOWN_BOOT_PARTITION =
                "${RomDetailActivity::class.java.canonicalName}.confirm.unknown_boot_partition"
        private val INPUT_DIALOG_BACKUP_NAME =
                "${RomDetailActivity::class.java.canonicalName}.input.backup_name"

        private const val REQUEST_IMAGE = 1234
        private const val REQUEST_MBTOOL_ERROR = 2345

        // Argument extras
        const val EXTRA_ROM_INFO = "rom_info"
        const val EXTRA_BOOTED_ROM_INFO = "booted_rom_info"
        const val EXTRA_ACTIVE_ROM_ID = "active_rom_id"
        // Saved state extras
        private const val EXTRA_STATE_TASK_ID_CACHE_WALLPAPER = "state.cache_wallpaper"
        private const val EXTRA_STATE_TASK_ID_SWITCH_ROM = "state.switch_rom"
        private const val EXTRA_STATE_TASK_ID_SET_KERNEL = "state.set_kernel"
        private const val EXTRA_STATE_TASK_ID_UPDATE_RAMDISK = "state.update_ramdisk"
        private const val EXTRA_STATE_TASK_ID_WIPE_ROM = "state.wipe_rom"
        private const val EXTRA_STATE_TASK_ID_CREATE_LAUNCHER = "state.create_launcher"
        private const val EXTRA_STATE_TASK_ID_GET_ROM_DETAILS = "state.get_rom_details"
        private const val EXTRA_STATE_TASK_IDS_TO_REMOVE = "state.task_ids_to_remove"
        private const val EXTRA_STATE_RESULT_INTENT = "state.result_intent"
        private const val EXTRA_STATE_BACKUP_TARGETS = "state.backup_targets"
        // Result extras
        const val EXTRA_RESULT_WIPED_ROM = "result.wiped_rom"

        private const val FORCE_RAMDISK_UPDATE_TAPS = 7
    }
}