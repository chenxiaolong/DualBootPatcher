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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.content.Intent;
import android.support.v4.content.LocalBroadcastManager;

import com.github.chenxiaolong.dualbootpatcher.RomUtils.CacheWallpaperResult;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.ThreadPoolService;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SetKernelResult;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolSocket.SwitchRomResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.KernelStatus;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils.VerificationResult;
import com.github.chenxiaolong.dualbootpatcher.switcher.ZipFlashingFragment.PendingAction;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BaseServiceTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.BaseServiceTask.TaskState;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CacheWallpaperTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CacheWallpaperTask
        .CacheWallpaperTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CreateLauncherTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.CreateLauncherTask
        .CreateLauncherTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.FlashZipsTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.FlashZipsTask.FlashZipsTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomDetailsTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomDetailsTask
        .GetRomDetailsTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomsStateTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.GetRomsStateTask
        .GetRomsStateTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SetKernelTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SetKernelTask.SetKernelTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SwitchRomTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.SwitchRomTask.SwitchRomTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.VerifyZipTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.VerifyZipTask.VerifyZipTaskListener;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.WipeRomTask;
import com.github.chenxiaolong.dualbootpatcher.switcher.service.WipeRomTask.WipeRomTaskListener;

import java.util.HashMap;
import java.util.concurrent.atomic.AtomicInteger;

public class SwitcherService extends ThreadPoolService {
    private static final String ACTION_BASE = SwitcherService.class.getCanonicalName();

    private static final String THREAD_POOL_DEFAULT = "default";
    private static final int THREAD_POOL_DEFAULT_THREADS = 2;

    /** Task ID. Sent as part of every broadcast */
    public static final String RESULT_TASK_ID = "task_id";

    private static final AtomicInteger sNewTaskId = new AtomicInteger(0);
    private static final HashMap<Integer, BaseServiceTask> sTaskCache = new HashMap<>();

    private void addTask(int taskId, BaseServiceTask task) {
        synchronized (sTaskCache) {
            sTaskCache.put(taskId, task);
        }
    }

    private BaseServiceTask getTask(int taskId) {
        synchronized (sTaskCache) {
            return sTaskCache.get(taskId);
        }
    }

    private BaseServiceTask removeTask(int taskId) {
        synchronized (sTaskCache) {
            return sTaskCache.remove(taskId);
        }
    }

    private void enqueueTask(BaseServiceTask task) {
        addTask(task.getTaskId(), task);
        enqueueOperation(THREAD_POOL_DEFAULT, task);
    }

    public TaskState getCachedTaskState(int taskId) {
        BaseServiceTask task = getTask(taskId);
        return task.getState();
    }

    public boolean removeCachedTask(int taskId) {
        BaseServiceTask task = removeTask(taskId);
        return task != null;
    }

    public void enforceFinishedState(BaseServiceTask task) {
        if (task.getState() != TaskState.FINISHED) {
            throw new IllegalStateException("Tried to get result for task " + task.getTaskId() +
                    " in state " + task.getState().name());
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void onCreate() {
        super.onCreate();

        addThreadPool(THREAD_POOL_DEFAULT, THREAD_POOL_DEFAULT_THREADS);
    }

    // Get ROMs state

    public static final String ACTION_GOT_ROMS_STATE = ACTION_BASE + ".got_roms_list";
    public static final String RESULT_GOT_ROMS_STATE_ROMS_LIST = "roms_list";
    public static final String RESULT_GOT_ROMS_STATE_CURRENT_ROM = "current_rom";
    public static final String RESULT_GOT_ROMS_STATE_ACTIVE_ROM_ID = "active_rom_id";
    public static final String RESULT_GOT_ROMS_STATE_KERNEL_STATUS = "kernel_status";

    public int getRomsState() {
        int taskId = sNewTaskId.getAndIncrement();
        GetRomsStateTask task = new GetRomsStateTask(taskId, this, mGetRomsStateTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final GetRomsStateTaskListener mGetRomsStateTaskListener =
            new GetRomsStateTaskListener() {
        @Override
        public void onGotRomsState(int taskId, RomInformation[] roms, RomInformation currentRom,
                                   String activeRomId, KernelStatus kernelStatus) {
            Intent intent = new Intent(ACTION_GOT_ROMS_STATE);
            intent.putExtra(RESULT_TASK_ID, taskId);
            intent.putExtra(RESULT_GOT_ROMS_STATE_ROMS_LIST, roms);
            intent.putExtra(RESULT_GOT_ROMS_STATE_CURRENT_ROM, currentRom);
            intent.putExtra(RESULT_GOT_ROMS_STATE_ACTIVE_ROM_ID, activeRomId);
            intent.putExtra(RESULT_GOT_ROMS_STATE_KERNEL_STATUS, kernelStatus);
            LocalBroadcastManager.getInstance(SwitcherService.this).sendBroadcast(intent);
        }
    };

    public RomInformation[] getResultRomsStateRomsList(int taskId) {
        GetRomsStateTask task = (GetRomsStateTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mRoms;
    }

    public RomInformation getResultRomsStateCurrentRom(int taskId) {
        GetRomsStateTask task = (GetRomsStateTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mCurrentRom;
    }

    public String getResultRomsStateActiveRomId(int taskId) {
        GetRomsStateTask task = (GetRomsStateTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mActiveRomId;
    }

    public KernelStatus getResultRomsStateKernelStatus(int taskId) {
        GetRomsStateTask task = (GetRomsStateTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mKernelStatus;
    }

    // Switch ROM

    public static final String ACTION_SWITCHED_ROM = ACTION_BASE + ".switched_rom";
    public static final String RESULT_SWITCHED_ROM_ROM_ID = "rom_id";
    public static final String RESULT_SWITCHED_ROM_RESULT = "result";

    public int switchRom(String romId, boolean forceChecksumsUpdate) {
        int taskId = sNewTaskId.getAndIncrement();
        SwitchRomTask task = new SwitchRomTask(
                taskId, this, romId, forceChecksumsUpdate, mSwitchRomTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final SwitchRomTaskListener mSwitchRomTaskListener = new SwitchRomTaskListener() {
        @Override
        public void onSwitchedRom(int taskid, String romId, SwitchRomResult result) {
            Intent intent = new Intent(ACTION_SWITCHED_ROM);
            intent.putExtra(RESULT_TASK_ID, taskid);
            intent.putExtra(RESULT_SWITCHED_ROM_ROM_ID, romId);
            intent.putExtra(RESULT_SWITCHED_ROM_RESULT, result);
            LocalBroadcastManager.getInstance(SwitcherService.this).sendBroadcast(intent);
        }
    };

    public String getResultSwitchedRomRomId(int taskId) {
        SwitchRomTask task = (SwitchRomTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mRomId;
    }

    public SwitchRomResult getResultSwitchedRomResult(int taskId) {
        SwitchRomTask task = (SwitchRomTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mResult;
    }

    // Set kernel

    public static final String ACTION_SET_KERNEL = ACTION_BASE + ".set_kernel";
    public static final String RESULT_SET_KERNEL_ROM_ID = "rom_id";
    public static final String RESULT_SET_KERNEL_RESULT = "result";

    public int setKernel(String romId) {
        int taskId = sNewTaskId.getAndIncrement();
        SetKernelTask task = new SetKernelTask(taskId, this, romId, mSetKernelTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final SetKernelTaskListener mSetKernelTaskListener = new SetKernelTaskListener() {
        @Override
        public void onSetKernel(int taskId, String romId, SetKernelResult result) {
            Intent intent = new Intent(ACTION_SET_KERNEL);
            intent.putExtra(RESULT_TASK_ID, taskId);
            intent.putExtra(RESULT_SET_KERNEL_ROM_ID, romId);
            intent.putExtra(RESULT_SET_KERNEL_RESULT, result);
            LocalBroadcastManager.getInstance(SwitcherService.this).sendBroadcast(intent);
        }
    };

    public String getResultSetKernelRomId(int taskId) {
        SetKernelTask task = (SetKernelTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mRomId;
    }

    public SetKernelResult getResultSetKernelResult(int taskId) {
        SetKernelTask task = (SetKernelTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mResult;
    }

    // Create launcher

    public static final String ACTION_CREATED_LAUNCHER = ACTION_BASE + ".created_launcher";
    public static final String RESULT_CREATED_LAUNCHER_ROM = "rom";

    public int createLauncher(RomInformation romInfo, boolean reboot) {
        int taskId = sNewTaskId.getAndIncrement();
        CreateLauncherTask task = new CreateLauncherTask(
                taskId, this, romInfo, reboot, mCreateLauncherTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final CreateLauncherTaskListener mCreateLauncherTaskListener =
            new CreateLauncherTaskListener() {
        @Override
        public void onCreatedLauncher(int taskId, RomInformation romInfo) {
            Intent intent = new Intent(ACTION_CREATED_LAUNCHER);
            intent.putExtra(RESULT_TASK_ID, taskId);
            intent.putExtra(RESULT_CREATED_LAUNCHER_ROM, romInfo);
            LocalBroadcastManager.getInstance(SwitcherService.this).sendBroadcast(intent);
        }
    };

    // Verify zip

    public static final String ACTION_VERIFIED_ZIP = ACTION_BASE + ".verified_zip";
    public static final String RESULT_VERIFIED_ZIP_RESULT = "result";
    public static final String RESULT_VERIFIED_ZIP_ROM_ID = "rom_id";

    public int verifyZip(String path) {
        int taskId = sNewTaskId.getAndIncrement();
        VerifyZipTask task = new VerifyZipTask(taskId, this, path, mVerifyZipTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final VerifyZipTaskListener mVerifyZipTaskListener = new VerifyZipTaskListener() {
        @Override
        public void onVerifiedZip(int taskId, String path, VerificationResult result,
                                  String romId) {
            Intent intent = new Intent(ACTION_VERIFIED_ZIP);
            intent.putExtra(RESULT_TASK_ID, taskId);
            intent.putExtra(RESULT_VERIFIED_ZIP_RESULT, result);
            intent.putExtra(RESULT_VERIFIED_ZIP_ROM_ID, romId);
            LocalBroadcastManager.getInstance(SwitcherService.this).sendBroadcast(intent);
        }
    };

    public String getResultVerifiedZipPath(int taskId) {
        VerifyZipTask task = (VerifyZipTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mPath;
    }

    public VerificationResult getResultVerifiedZipResult(int taskId) {
        VerifyZipTask task = (VerifyZipTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mResult;
    }

    public String getResultVerifiedZipRomId(int taskId) {
        VerifyZipTask task = (VerifyZipTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mRomId;
    }

    // In-app flashing

    public static final String ACTION_FLASHED_ZIPS = ACTION_BASE + ".flashed_zips";
    public static final String ACTION_NEW_OUTPUT_LINE = ACTION_BASE + ".new_output_line";
    public static final String RESULT_FLASHED_ZIPS_TOTAL_ACTIONS = "total_actions";
    public static final String RESULT_FLASHED_ZIPS_FAILED_ACTIONS = "failed_actions";
    public static final String RESULT_FLASHED_ZIPS_OUTPUT_LINE = "output_line";

    public int flashZips(PendingAction[] actions) {
        int taskId = sNewTaskId.getAndIncrement();
        FlashZipsTask task = new FlashZipsTask(taskId, this, actions, mFlashZipsTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final FlashZipsTaskListener mFlashZipsTaskListener = new FlashZipsTaskListener() {
        @Override
        public void onFlashedZips(int taskId, int totalActions, int failedActions) {
            Intent intent = new Intent(ACTION_FLASHED_ZIPS);
            intent.putExtra(RESULT_TASK_ID, taskId);
            intent.putExtra(RESULT_FLASHED_ZIPS_TOTAL_ACTIONS, totalActions);
            intent.putExtra(RESULT_FLASHED_ZIPS_FAILED_ACTIONS, failedActions);
            LocalBroadcastManager.getInstance(SwitcherService.this).sendBroadcast(intent);
        }

        @Override
        public void onCommandOutput(int taskId, String line) {
            Intent intent = new Intent(ACTION_NEW_OUTPUT_LINE);
            intent.putExtra(RESULT_TASK_ID, taskId);
            intent.putExtra(RESULT_FLASHED_ZIPS_OUTPUT_LINE, line);
            LocalBroadcastManager.getInstance(SwitcherService.this).sendBroadcast(intent);
        }
    };

    public int getResultFlashedZipsTotalActions(int taskId) {
        FlashZipsTask task = (FlashZipsTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mTotal;
    }

    public int getResultFlashedZipsFailedActions(int taskId) {
        FlashZipsTask task = (FlashZipsTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mFailed;
    }

    public String[] getResultFlashedZipsOutputLines(int taskId) {
        FlashZipsTask task = (FlashZipsTask) getTask(taskId);
        return task.getLines();
    }

    // Wipe ROM

    public static final String ACTION_WIPED_ROM = ACTION_BASE + ".wiped_rom";
    public static final String RESULT_WIPED_ROM_ROM_ID = "rom_id";
    public static final String RESULT_WIPED_ROM_SUCCEEDED = "succeeded";
    public static final String RESULT_WIPED_ROM_FAILED = "failed";

    public int wipeRom(String romId, short[] targets) {
        int taskId = sNewTaskId.getAndIncrement();
        WipeRomTask task = new WipeRomTask(taskId, this, romId, targets, mWipeRomTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final WipeRomTaskListener mWipeRomTaskListener = new WipeRomTaskListener() {
        @Override
        public void onWipedRom(int taskId, String romId, short[] succeeded, short[] failed) {
            Intent intent = new Intent(ACTION_WIPED_ROM);
            intent.putExtra(RESULT_TASK_ID, taskId);
            intent.putExtra(RESULT_WIPED_ROM_ROM_ID, romId);
            intent.putExtra(RESULT_WIPED_ROM_SUCCEEDED, succeeded);
            intent.putExtra(RESULT_WIPED_ROM_FAILED, failed);
            LocalBroadcastManager.getInstance(SwitcherService.this).sendBroadcast(intent);
        }
    };

    public String getResultWipedRomRom(int taskId) {
        WipeRomTask task = (WipeRomTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mRomId;
    }

    public short[] getResultWipedRomTargetsSucceeded(int taskId) {
        WipeRomTask task = (WipeRomTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mTargetsSucceeded;
    }

    public short[] getResultWipedRomTargetsFailed(int taskId) {
        WipeRomTask task = (WipeRomTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mTargetsFailed;
    }

    // Cache wallpaper

    public static final String ACTION_CACHED_WALLPAPER = ACTION_BASE + ".cached_wallpaper";
    public static final String RESULT_CACHED_WALLPAPER_ROM = "rom";
    public static final String RESULT_CACHED_WALLPAPER_RESULT = "result";

    public int cacheWallpaper(RomInformation info) {
        int taskId = sNewTaskId.getAndIncrement();
        CacheWallpaperTask task = new CacheWallpaperTask(
                taskId, this, info, mCacheWallpaperTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final CacheWallpaperTaskListener mCacheWallpaperTaskListener =
            new CacheWallpaperTaskListener() {
        @Override
        public void onCachedWallpaper(int taskId, RomInformation romInfo,
                                      CacheWallpaperResult result) {
            Intent intent = new Intent(ACTION_CACHED_WALLPAPER);
            intent.putExtra(RESULT_TASK_ID, taskId);
            intent.putExtra(RESULT_CACHED_WALLPAPER_ROM, romInfo);
            intent.putExtra(RESULT_CACHED_WALLPAPER_RESULT, result);
            LocalBroadcastManager.getInstance(SwitcherService.this).sendBroadcast(intent);
        }
    };

    public RomInformation getResultCachedWallpaperRom(int taskId) {
        CacheWallpaperTask task = (CacheWallpaperTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mRomInfo;
    }

    public CacheWallpaperResult getResultCachedWallpaperResult(int taskId) {
        CacheWallpaperTask task = (CacheWallpaperTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mResult;
    }

    // Get ROM details

    public static final String ACTION_ROM_DETAILS_FINISHED =
            ACTION_BASE + ".rom_details.finished";
    public static final String ACTION_ROM_DETAILS_GOT_SYSTEM_SIZE =
            ACTION_BASE + ".rom_details.got_system_size";
    public static final String ACTION_ROM_DETAILS_GOT_CACHE_SIZE =
            ACTION_BASE + ".rom_details.got_cache_size";
    public static final String ACTION_ROM_DETAILS_GOT_DATA_SIZE =
            ACTION_BASE + ".rom_details.got_data_size";
    public static final String ACTION_ROM_DETAILS_GOT_PACKAGES_COUNTS =
            ACTION_BASE + ".rom_details.got_packages_counts";
    public static final String RESULT_ROM_DETAILS_ROM = "rom";
    public static final String RESULT_ROM_DETAILS_SIZE = "size";
    public static final String RESULT_ROM_DETAILS_SUCCESS = "success";
    public static final String RESULT_ROM_DETAILS_PACKAGES_COUNT_SYSTEM = "packages_system";
    public static final String RESULT_ROM_DETAILS_PACKAGES_COUNT_UPDATED = "packages_updated";
    public static final String RESULT_ROM_DETAILS_PACKAGES_COUNT_USER = "packages_user";

    public int getRomDetails(RomInformation romInfo) {
        int taskId = sNewTaskId.getAndIncrement();
        GetRomDetailsTask task = new GetRomDetailsTask(
                taskId, this, romInfo, mGetRomDetailsTaskListener);
        enqueueTask(task);
        return taskId;
    }

    private final GetRomDetailsTaskListener mGetRomDetailsTaskListener =
            new GetRomDetailsTaskListener() {
        @Override
        public void onRomDetailsGotSystemSize(int taskId, RomInformation romInfo,
                                              boolean success, long size) {
            Intent intent = new Intent(ACTION_ROM_DETAILS_GOT_SYSTEM_SIZE);
            intent.putExtra(RESULT_TASK_ID, taskId);
            intent.putExtra(RESULT_ROM_DETAILS_ROM, romInfo);
            intent.putExtra(RESULT_ROM_DETAILS_SUCCESS, success);
            intent.putExtra(RESULT_ROM_DETAILS_SIZE, size);
            LocalBroadcastManager.getInstance(SwitcherService.this).sendBroadcast(intent);
        }

        @Override
        public void onRomDetailsGotCacheSize(int taskId, RomInformation romInfo,
                                             boolean success, long size) {
            Intent intent = new Intent(ACTION_ROM_DETAILS_GOT_CACHE_SIZE);
            intent.putExtra(RESULT_TASK_ID, taskId);
            intent.putExtra(RESULT_ROM_DETAILS_ROM, romInfo);
            intent.putExtra(RESULT_ROM_DETAILS_SUCCESS, success);
            intent.putExtra(RESULT_ROM_DETAILS_SIZE, size);
            LocalBroadcastManager.getInstance(SwitcherService.this).sendBroadcast(intent);
        }

        @Override
        public void onRomDetailsGotDataSize(int taskId, RomInformation romInfo,
                                            boolean success, long size) {
            Intent intent = new Intent(ACTION_ROM_DETAILS_GOT_DATA_SIZE);
            intent.putExtra(RESULT_TASK_ID, taskId);
            intent.putExtra(RESULT_ROM_DETAILS_ROM, romInfo);
            intent.putExtra(RESULT_ROM_DETAILS_SUCCESS, success);
            intent.putExtra(RESULT_ROM_DETAILS_SIZE, size);
            LocalBroadcastManager.getInstance(SwitcherService.this).sendBroadcast(intent);
        }

        @Override
        public void onRomDetailsGotPackagesCounts(int taskId, RomInformation romInfo,
                                                  boolean success, int systemPackages,
                                                  int updatedPackages, int userPackages) {
            Intent intent = new Intent(ACTION_ROM_DETAILS_GOT_PACKAGES_COUNTS);
            intent.putExtra(RESULT_TASK_ID, taskId);
            intent.putExtra(RESULT_ROM_DETAILS_ROM, romInfo);
            intent.putExtra(RESULT_ROM_DETAILS_SUCCESS, success);
            intent.putExtra(RESULT_ROM_DETAILS_PACKAGES_COUNT_SYSTEM, systemPackages);
            intent.putExtra(RESULT_ROM_DETAILS_PACKAGES_COUNT_UPDATED, updatedPackages);
            intent.putExtra(RESULT_ROM_DETAILS_PACKAGES_COUNT_USER, userPackages);
            LocalBroadcastManager.getInstance(SwitcherService.this).sendBroadcast(intent);
        }

        @Override
        public void onRomDetailsFinished(int taskId, RomInformation romInfo) {
            Intent intent = new Intent(ACTION_ROM_DETAILS_FINISHED);
            intent.putExtra(RESULT_TASK_ID, taskId);
            intent.putExtra(RESULT_ROM_DETAILS_ROM, romInfo);
            LocalBroadcastManager.getInstance(SwitcherService.this).sendBroadcast(intent);
        }
    };

    public RomInformation getResultRomDetailsRom(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        enforceFinishedState(task);
        return task.mRomInfo;
    }

    public boolean hasResultRomDetailsSystemSize(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        return task.mHaveSystemSize.get();
    }

    public boolean hasResultRomDetailsCacheSize(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        return task.mHaveCacheSize.get();
    }

    public boolean hasResultRomDetailsDataSize(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        return task.mHaveDataSize.get();
    }

    public boolean hasResultRomDetailsPackagesCounts(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        return task.mHavePackagesCounts.get();
    }

    public boolean getResultRomDetailsSystemSizeSuccess(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHaveSystemSize.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting system size");
        }
        return task.mSystemSizeSuccess;
    }

    public long getResultRomDetailsSystemSize(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHaveSystemSize.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting system size");
        }
        return task.mSystemSize;
    }

    public boolean getResultRomDetailsCacheSizeSuccess(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHaveCacheSize.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting cache size");
        }
        return task.mCacheSizeSuccess;
    }

    public long getResultRomDetailsCacheSize(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHaveCacheSize.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting cache size");
        }
        return task.mCacheSize;
    }

    public boolean getResultRomDetailsDataSizeSuccess(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHaveDataSize.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting data size");
        }
        return task.mDataSizeSuccess;
    }

    public long getResultRomDetailsDataSize(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHaveDataSize.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting data size");
        }
        return task.mDataSize;
    }

    public boolean getResultRomDetailsPackagesCountsSuccess(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHavePackagesCounts.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting packages counts");
        }
        return task.mPackagesCountsSuccess;
    }

    public int getResultRomDetailsSystemPackages(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHavePackagesCounts.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting packages counts");
        }
        return task.mSystemPackages;
    }

    public int getResultRomDetailsUpdatedPackages(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHavePackagesCounts.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting packages counts");
        }
        return task.mSystemPackages;
    }

    public int getResultRomDetailsUserPackages(int taskId) {
        GetRomDetailsTask task = (GetRomDetailsTask) getTask(taskId);
        if (!task.mHavePackagesCounts.get()) {
            throw new IllegalStateException(
                    "Task " + taskId + " has not finished getting packages counts");
        }
        return task.mSystemPackages;
    }
}
