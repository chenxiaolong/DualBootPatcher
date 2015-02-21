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
import android.app.ProgressDialog;
import android.content.AsyncTaskLoader;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnDismissListener;
import android.content.Loader;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.support.annotation.NonNull;

import com.afollestad.materialdialogs.MaterialDialog;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;
import com.github.chenxiaolong.multibootpatcher.EventCollector.BaseEvent;
import com.github.chenxiaolong.multibootpatcher.EventCollector.EventCollectorListener;
import com.github.chenxiaolong.multibootpatcher.Version;
import com.github.chenxiaolong.multibootpatcher.settings.RomSettingsEventCollector
        .UpdatedRamdiskEvent;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolUtils;

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
    private ProgressDialog mRamdiskDialog;
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
        }

        // Only show progress dialog on initial load
        if (savedInstanceState == null) {
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
        if (mRamdiskDialog == null) {
            mRamdiskDialog = new ProgressDialog(getActivity());
            mRamdiskDialog.setMessage(getString(R.string.please_wait));
            mRamdiskDialog.setCancelable(false);
            mRamdiskDialog.setIndeterminate(true);
            mRamdiskDialog.show();
            mRamdiskDialog.setOnDismissListener(this);
        }
    }

    private void buildUpdateRamdiskDoneDialog() {
        if (mUpdateDoneDialog == null) {
            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());

            if (mUpdateSucceeded) {
                builder.setTitle(R.string.update_ramdisk_failure_title);
                builder.setMessage(R.string.update_ramdisk_failure_desc);

                builder.setNegativeButton(R.string.ok, null);
            } else {
                builder.setTitle(R.string.update_ramdisk_success_title);
                builder.setMessage(R.string.update_ramdisk_reboot_desc);

                final Context context = getActivity().getApplicationContext();

                builder.setNegativeButton(R.string.reboot_later, null);
                builder.setPositiveButton(R.string.reboot_now, new OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        new Thread() {
                            @Override
                            public void run() {
                                SwitcherUtils.reboot(context);
                            }
                        }.start();
                    }
                });
            }

            mUpdateDoneDialog = builder.show();
            mUpdateDoneDialog.setOnDismissListener(this);
            mUpdateDoneDialog.setCanceledOnTouchOutside(false);
        }
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
        if (mProgressDialog != null && mProgressDialog == dialog) {
            mProgressDialog = null;
        }

        if (mRamdiskDialog != null && mRamdiskDialog == dialog) {
            mRamdiskDialog = null;
        }

        if (mUpdateDoneDialog != null && mUpdateDoneDialog == dialog) {
            mUpdateDoneDialog = null;
        }
    }

    @Override
    public void onEventReceived(BaseEvent event) {
        if (event instanceof UpdatedRamdiskEvent) {
            UpdatedRamdiskEvent e = (UpdatedRamdiskEvent) event;

            if (mRamdiskDialog != null) {
                mRamdiskDialog.dismiss();
            }

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
        if (version.compareTo(MbtoolUtils.getMinimumRequiredVersion()) >= 0) {
            //bootingCategory.removePreference(updateRamdisk);
            getPreferenceScreen().removePreference(mBootingCategory);
        }

        if (mProgressDialog != null) {
            mProgressDialog.dismiss();
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
