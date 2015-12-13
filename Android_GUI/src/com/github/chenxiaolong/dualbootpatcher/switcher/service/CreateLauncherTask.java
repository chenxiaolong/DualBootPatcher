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

package com.github.chenxiaolong.dualbootpatcher.switcher.service;

import android.app.ActivityManager;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.util.Log;

import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.switcher.AutomatedSwitcherActivity;

import java.io.File;

public final class CreateLauncherTask extends BaseServiceTask {
    private static final String TAG = CreateLauncherTask.class.getSimpleName();

    private final RomInformation mRomInfo;
    private final boolean mReboot;
    private final CreateLauncherTaskListener mListener;

    public interface CreateLauncherTaskListener extends BaseServiceTaskListener {
        void onCreatedLauncher(int taskId, RomInformation romInfo);
    }

    public CreateLauncherTask(int taskId, Context context, RomInformation romInfo, boolean reboot,
                              CreateLauncherTaskListener listener) {
        super(taskId, context);
        mRomInfo = romInfo;
        mReboot = reboot;
        mListener = listener;
    }

    @Override
    public void execute() {
        Log.d(TAG, "Creating launcher for " + mRomInfo.getId());

        Intent shortcutIntent = new Intent(getContext(), AutomatedSwitcherActivity.class);
        shortcutIntent.setAction("com.github.chenxiaolong.dualbootpatcher.SWITCH_ROM");
        shortcutIntent.putExtra(AutomatedSwitcherActivity.EXTRA_ROM_ID, mRomInfo.getId());
        shortcutIntent.putExtra(AutomatedSwitcherActivity.EXTRA_REBOOT, mReboot);

        Intent addIntent = new Intent("com.android.launcher.action.INSTALL_SHORTCUT");
        addIntent.putExtra("duplicate", false);
        addIntent.putExtra(Intent.EXTRA_SHORTCUT_INTENT, shortcutIntent);
        addIntent.putExtra(Intent.EXTRA_SHORTCUT_NAME, mRomInfo.getName());

        File file = new File(mRomInfo.getThumbnailPath());
        if (file.exists() && file.canRead()) {
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inPreferredConfig = Bitmap.Config.ARGB_8888;
            Bitmap bitmap = BitmapFactory.decodeFile(file.getAbsolutePath(), options);
            addIntent.putExtra(Intent.EXTRA_SHORTCUT_ICON, createScaledIcon(bitmap));
        } else {
            addIntent.putExtra(Intent.EXTRA_SHORTCUT_ICON_RESOURCE,
                    Intent.ShortcutIconResource.fromContext(
                            getContext(), mRomInfo.getImageResId()));
        }

        getContext().sendBroadcast(addIntent);

        mListener.onCreatedLauncher(getTaskId(), mRomInfo);
    }

    private Bitmap createScaledIcon(Bitmap icon) {
        ActivityManager am = (ActivityManager)
                getContext().getSystemService(Context.ACTIVITY_SERVICE);
        final int iconSize = am.getLauncherLargeIconSize();
        return Bitmap.createScaledBitmap(icon, iconSize, iconSize, true);
    }
}
