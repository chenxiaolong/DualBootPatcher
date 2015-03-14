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

package com.github.chenxiaolong.multibootpatcher.settings;

import android.app.AlertDialog;
import android.app.LoaderManager;
import android.content.AsyncTaskLoader;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.Loader;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.support.annotation.NonNull;

import com.afollestad.materialdialogs.MaterialDialog;
import com.afollestad.materialdialogs.MaterialDialog.ButtonCallback;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;
import com.github.chenxiaolong.multibootpatcher.EventCollector.BaseEvent;
import com.github.chenxiaolong.multibootpatcher.EventCollector.EventCollectorListener;
import com.github.chenxiaolong.multibootpatcher.Version;
import com.github.chenxiaolong.multibootpatcher.settings.RomSettingsEventCollector
        .UpdatedRamdiskEvent;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolUtils;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolUtils.Feature;

public class RomSettingsFragment extends PreferenceFragment implements OnPreferenceClickListener,
        OnDismissListener, EventCollectorListener, LoaderManager.LoaderCallbacks<Version> {
    public static final String TAG = RomSettingsFragment.class.getSimpleName();

    private static final String EXTRA_PROGRESS_DIALOG = "progressDialog";
    private static final String EXTRA_UPDATE_RAMDISK_DIALOG = "updateRamdiskDialog";
    private static final String EXTRA_UPDATE_RAMDISK_DONE_DIALOG = "updateRamdiskDoneDialog";
    private static final String EXTRA_UPDATE_SUCCEEDED = "updateSucceeded";

    private static final String KEY_BOOTING_CATEGORY = "booting_category";
    private static final String KEY_UPDATE_RAMDISK = "update_ramdisk";

    private RomSettingsEventCollector mEventCollector;

    private PreferenceCategory mBootingCategory;

    private MaterialDialog mProgressDialog;
    private MaterialDialog mRamdiskDialog;
    private AlertDialog mUpdateDoneDialog;
    private boolean mUpdateSucceeded;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mEventCollector = RomSettingsEventCollector.getInstance(getFragmentManager());

        getPreferenceManager().setSharedPreferencesName("settings");

        addPreferencesFromResource(R.xml.rom_settings);

        mBootingCategory = (PreferenceCategory) findPreference(KEY_BOOTING_CATEGORY);

        Preference updateRamdisk = findPreference(KEY_UPDATE_RAMDISK);
        updateRamdisk.setOnPreferenceClickListener(this);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        if (savedInstanceState != null) {
            if (savedInstanceState.getBoolean(EXTRA_PROGRESS_DIALOG, false)) {
                buildProgressDialog();
            }

            mUpdateSucceeded = savedInstanceState.getBoolean(EXTRA_UPDATE_SUCCEEDED);

            if (savedInstanceState.getBoolean(EXTRA_UPDATE_RAMDISK_DIALOG, false)) {
                buildUpdateRamdiskDialog();
            }

            if (savedInstanceState.getBoolean(EXTRA_UPDATE_RAMDISK_DONE_DIALOG, false)) {
                buildUpdateRamdiskDoneDialog();
            }
        } else {
            // Only show progress dialog on initial load
            buildProgressDialog();
        }

        getActivity().getLoaderManager().initLoader(0, null, this);
    }

    @Override
    public void onSaveInstanceState(@NonNull Bundle outState) {
        super.onSaveInstanceState(outState);

        if (mProgressDialog != null) {
            outState.putBoolean(EXTRA_PROGRESS_DIALOG, true);
        }

        outState.putBoolean(EXTRA_UPDATE_SUCCEEDED, mUpdateSucceeded);

        if (mRamdiskDialog != null) {
            outState.putBoolean(EXTRA_UPDATE_RAMDISK_DIALOG, true);
        }

        if (mUpdateDoneDialog != null) {
            outState.putBoolean(EXTRA_UPDATE_RAMDISK_DONE_DIALOG, true);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        mEventCollector.attachListener(TAG, this);
    }

    @Override
    public void onPause() {
        super.onPause();
        mEventCollector.detachListener(TAG);
    }

    @Override
    public void onStop() {
        super.onStop();

        if (mProgressDialog != null) {
            mProgressDialog.dismiss();
            mProgressDialog = null;
        }

        if (mRamdiskDialog != null) {
            mRamdiskDialog.dismiss();
            mRamdiskDialog = null;
        }

        if (mUpdateDoneDialog != null) {
            mUpdateDoneDialog.dismiss();
            mUpdateDoneDialog = null;
        }
    }

    private void buildProgressDialog() {
        if (mProgressDialog != null) {
            throw new IllegalStateException("Tried to create progress dialog twice!");
        }

        mProgressDialog = new MaterialDialog.Builder(getActivity())
                .content(R.string.please_wait)
                .progress(true, 0)
                .build();

        mProgressDialog.setOnDismissListener(this);
        mProgressDialog.setCanceledOnTouchOutside(false);
        mProgressDialog.setCancelable(false);
        mProgressDialog.show();
    }

    private void buildUpdateRamdiskDialog() {
        if (mRamdiskDialog != null) {
            throw new IllegalStateException("Tried to create progress dialog twice!");
        }

        mRamdiskDialog = new MaterialDialog.Builder(getActivity())
                .content(R.string.please_wait)
                .progress(true, 0)
                .build();

        mRamdiskDialog.setOnDismissListener(this);
        mRamdiskDialog.setCanceledOnTouchOutside(false);
        mRamdiskDialog.setCancelable(false);
        mRamdiskDialog.show();
    }

    private void buildUpdateRamdiskDoneDialog() {
        if (mUpdateDoneDialog != null) {
            throw new IllegalStateException("Tried to create dialog twice!");
        }

        MaterialDialog.Builder builder = new MaterialDialog.Builder(getActivity());

        if (!mUpdateSucceeded) {
            builder.title(R.string.update_ramdisk_failure_title);
            builder.content(R.string.update_ramdisk_failure_desc);

            builder.negativeText(R.string.ok);
        } else {
            builder.title(R.string.update_ramdisk_success_title);
            builder.content(R.string.update_ramdisk_reboot_desc);

            final Context context = getActivity().getApplicationContext();

            builder.negativeText(R.string.reboot_later);
            builder.positiveText(R.string.reboot_now);

            builder.callback(new ButtonCallback() {
                @Override
                public void onPositive(MaterialDialog dialog) {
                    new Thread() {
                        @Override
                        public void run() {
                            SwitcherUtils.reboot(context);
                        }
                    }.start();
                }
            });
        }

        mUpdateDoneDialog = builder.build();
        mUpdateDoneDialog.setOnDismissListener(this);
        mUpdateDoneDialog.setCanceledOnTouchOutside(false);
        mUpdateDoneDialog.setCancelable(false);
        mUpdateDoneDialog.show();
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        final String key = preference.getKey();

        if (KEY_UPDATE_RAMDISK.equals(key)) {
            buildUpdateRamdiskDialog();
            mEventCollector.updateRamdisk();
        }

        return true;
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        if (mProgressDialog == dialog) {
            mProgressDialog = null;
        }

        if (mRamdiskDialog == dialog) {
            mRamdiskDialog = null;
        }

        if (mUpdateDoneDialog == dialog) {
            mUpdateDoneDialog = null;
        }
    }

    @Override
    public void onEventReceived(BaseEvent event) {
        if (event instanceof UpdatedRamdiskEvent) {
            UpdatedRamdiskEvent e = (UpdatedRamdiskEvent) event;

            mRamdiskDialog.dismiss();
            mRamdiskDialog = null;

            mUpdateSucceeded = e.success;
            buildUpdateRamdiskDoneDialog();
        }
    }

    @Override
    public Loader<Version> onCreateLoader(int i, Bundle bundle) {
        return new GetInfo(getActivity());
    }

    @Override
    public void onLoadFinished(Loader<Version> versionLoader, Version version) {
        if (version.compareTo(MbtoolUtils.getMinimumRequiredVersion(Feature.DAEMON)) >= 0) {
            //bootingCategory.removePreference(updateRamdisk);
            getPreferenceScreen().removePreference(mBootingCategory);
        }

        if (mProgressDialog != null) {
            mProgressDialog.dismiss();
            mProgressDialog = null;
        }
    }

    @Override
    public void onLoaderReset(Loader<Version> versionLoader) {
    }

    private static class GetInfo extends AsyncTaskLoader<Version> {
        private Version mResult;

        public GetInfo(Context context) {
            super(context);
            onContentChanged();
        }

        @Override
        protected void onStartLoading() {
            if (mResult != null) {
                deliverResult(mResult);
            } else if (takeContentChanged()) {
                forceLoad();
            }
        }

        @Override
        public Version loadInBackground() {
            mResult = MbtoolUtils.getSystemMbtoolVersion(getContext());
            return mResult;
        }
    }
}
