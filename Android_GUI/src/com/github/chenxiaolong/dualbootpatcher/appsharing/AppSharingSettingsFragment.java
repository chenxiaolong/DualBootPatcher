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

package com.github.chenxiaolong.dualbootpatcher.appsharing;

import android.app.LoaderManager;
import android.content.AsyncTaskLoader;
import android.content.Context;
import android.content.Intent;
import android.content.Loader;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceFragment;
import android.support.annotation.NonNull;
import android.text.Html;
import android.view.View;
import android.view.animation.AlphaAnimation;

import com.github.chenxiaolong.dualbootpatcher.EventCollector.BaseEvent;
import com.github.chenxiaolong.dualbootpatcher.EventCollector.EventCollectorListener;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomConfig;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.Version;
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppSharingSettingsFragment.NeededInfo;
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppSharingVersionTooOldDialog
        .AppSharingVersionTooOldDialogListener;
import com.github.chenxiaolong.dualbootpatcher.dialogs.IndeterminateProgressDialog;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils.Feature;

public class AppSharingSettingsFragment extends PreferenceFragment implements
        OnPreferenceChangeListener, OnPreferenceClickListener, EventCollectorListener,
        AppSharingVersionTooOldDialogListener,
        LoaderManager.LoaderCallbacks<NeededInfo> {
    public static final String TAG = AppSharingSettingsFragment.class.getSimpleName();

    private static final String EXTRA_SHOWED_VERSION_TOO_OLD_DIALOG =
            "showed_version_too_old_dialog";

    private static final String KEY_SHARE_APPS = "share_apps";
    private static final String KEY_SHARE_PAID_APPS = "share_paid_apps";
    private static final String KEY_SHARE_INDIV_APPS = "share_indiv_apps";
    private static final String KEY_MANAGE_INDIV_APPS = "manage_indiv_apps";

    private AppSharingEventCollector mEventCollector;

    private RomInformation mCurrentRom;
    private RomConfig mConfig;

    private CheckBoxPreference mShareApps;
    private CheckBoxPreference mSharePaidApps;
    private CheckBoxPreference mShareIndivApps;
    private Preference mManageIndivApps;

    private boolean mShowedVersionTooOldDialog;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mEventCollector = AppSharingEventCollector.getInstance(getFragmentManager());

        getPreferenceManager().setSharedPreferencesName("settings");

        addPreferencesFromResource(R.xml.app_sharing_settings);

        mShareApps = (CheckBoxPreference) findPreference(KEY_SHARE_APPS);
        mSharePaidApps = (CheckBoxPreference) findPreference(KEY_SHARE_PAID_APPS);
        mShareIndivApps = (CheckBoxPreference) findPreference(KEY_SHARE_INDIV_APPS);
        mShareApps.setOnPreferenceChangeListener(this);
        mSharePaidApps.setOnPreferenceChangeListener(this);
        mShareIndivApps.setOnPreferenceChangeListener(this);

        mManageIndivApps = findPreference(KEY_MANAGE_INDIV_APPS);
        mManageIndivApps.setOnPreferenceClickListener(this);
    }

    @Override
    public void onSaveInstanceState(@NonNull Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putBoolean(EXTRA_SHOWED_VERSION_TOO_OLD_DIALOG, mShowedVersionTooOldDialog);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        if (getView() != null) {
            getView().setVisibility(View.GONE);
        }

        // Only show progress dialog on initial load
        if (savedInstanceState == null) {
            IndeterminateProgressDialog d = IndeterminateProgressDialog.newInstance();
            d.show(getFragmentManager(), IndeterminateProgressDialog.TAG);
        }

        if (savedInstanceState != null) {
            mShowedVersionTooOldDialog =
                    savedInstanceState.getBoolean(EXTRA_SHOWED_VERSION_TOO_OLD_DIALOG);
        }

        getActivity().getLoaderManager().initLoader(0, null, this);
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
    public void onDestroy() {
        super.onDestroy();

        if (mConfig == null) {
            // Destroyed before onLoadFinished ran
            return;
        }

        // Write changes to config file
        mConfig.apply();
    }

    @Override
    public void onEventReceived(BaseEvent event) {
        if (event instanceof DismissProgressDialogEvent) {
            IndeterminateProgressDialog d = (IndeterminateProgressDialog) getFragmentManager()
                    .findFragmentByTag(IndeterminateProgressDialog.TAG);
            if (d != null) {
                d.dismiss();
            }
        } else if (event instanceof ShowVersionTooOldDialogEvent) {
            if (!mShowedVersionTooOldDialog) {
                mShowedVersionTooOldDialog = true;

                AppSharingVersionTooOldDialog d = AppSharingVersionTooOldDialog.newInstance(this);
                d.show(getFragmentManager(), AppSharingVersionTooOldDialog.TAG);
            }
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object objValue) {
        final String key = preference.getKey();

        if (KEY_SHARE_APPS.equals(key)) {
            mConfig.setGlobalAppSharingEnabled((Boolean) objValue);
            refreshEnabledPrefs();
        } else if (KEY_SHARE_PAID_APPS.equals(key)) {
            mConfig.setGlobalPaidAppSharingEnabled((Boolean) objValue);
            refreshEnabledPrefs();
        } else if (KEY_SHARE_INDIV_APPS.equals(key)) {
            mConfig.setIndivAppSharingEnabled((Boolean) objValue);
            refreshEnabledPrefs();
        }

        return true;
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        final String key = preference.getKey();

        if (KEY_MANAGE_INDIV_APPS.equals(key)) {
            startActivity(new Intent(getActivity(), AppListActivity.class));
        }

        return true;
    }

    private void ensureSaneValues() {
        if (mConfig.isIndivAppSharingEnabled()) {
            mConfig.setGlobalAppSharingEnabled(false);
            mConfig.setGlobalPaidAppSharingEnabled(false);
        }

        // Global app sharing cannot be used in the primary ROM
        if (mCurrentRom.getId().equals(RomUtils.PRIMARY_ID)) {
            mConfig.setGlobalAppSharingEnabled(false);
            mConfig.setGlobalPaidAppSharingEnabled(false);
        }
    }

    private void showAppSharingPrefs() {
        mShareApps.setChecked(mConfig.isGlobalAppSharingEnabled());
        mSharePaidApps.setChecked(mConfig.isGlobalPaidAppSharingEnabled());
        mShareIndivApps.setChecked(mConfig.isIndivAppSharingEnabled());

        if (mCurrentRom.getId().equals(RomUtils.PRIMARY_ID)) {
            mShareApps.setSummary(R.string.a_s_settings_global_incompat_primary);
            mSharePaidApps.setSummary(R.string.a_s_settings_global_incompat_primary);
            mShareApps.setEnabled(false);
            mSharePaidApps.setEnabled(false);
        } else {
            mShareApps.setSummary(R.string.a_s_settings_share_apps_desc);
            mSharePaidApps.setSummary(Html.fromHtml(getString(
                    R.string.a_s_settings_share_paid_apps_desc)));
        }
    }

    private void refreshEnabledPrefs() {
        boolean indivEnabled = mConfig.isIndivAppSharingEnabled();
        boolean globalEnabled = mConfig.isGlobalAppSharingEnabled() ||
                mConfig.isGlobalPaidAppSharingEnabled();

        if (!mCurrentRom.getId().equals(RomUtils.PRIMARY_ID)) {
            mShareApps.setEnabled(!indivEnabled);
            mSharePaidApps.setEnabled(!indivEnabled);
        }
        mShareIndivApps.setEnabled(!globalEnabled);
        mManageIndivApps.setEnabled(!globalEnabled && indivEnabled);
    }

    @Override
    public Loader<NeededInfo> onCreateLoader(int id, Bundle args) {
        return new GetInfo(getActivity());
    }

    @Override
    public void onLoadFinished(Loader<NeededInfo> loader, NeededInfo result) {
        if (result.currentRom == null) {
            getActivity().finish();
            return;
        }

        if (!result.haveRequiredVersion) {
            mEventCollector.postEvent(new ShowVersionTooOldDialogEvent());
            return;
        }

        mCurrentRom = result.currentRom;
        mConfig = result.config;

        ensureSaneValues();
        showAppSharingPrefs();
        refreshEnabledPrefs();

        AlphaAnimation fadeAnimation = new AlphaAnimation(0, 1);
        fadeAnimation.setDuration(100);
        fadeAnimation.setFillAfter(true);

        if (getView() != null) {
            getView().startAnimation(fadeAnimation);
            getView().setVisibility(View.VISIBLE);
        }

        // Can't call dismiss() on the DialogFragment from here because the state may have already
        // been saved. Instead, it'll be closed when we come back.
        mEventCollector.postEvent(new DismissProgressDialogEvent());
    }

    @Override
    public void onLoaderReset(Loader<NeededInfo> loader) {
    }

    @Override
    public void onConfirmVersionTooOld() {
        getActivity().finish();
    }

    protected static class NeededInfo {
        RomInformation currentRom;
        RomConfig config;
        Version mbtoolVersion;
        boolean haveRequiredVersion;
    }

    private static class GetInfo extends AsyncTaskLoader<NeededInfo> {
        private NeededInfo mResult;

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
        public NeededInfo loadInBackground() {
            RomInformation currentRom = RomUtils.getCurrentRom(getContext());
            if (currentRom == null) {
                return null;
            }

            mResult = new NeededInfo();
            mResult.currentRom = currentRom;
            mResult.config = RomConfig.getConfig(currentRom.getConfigPath());
            mResult.mbtoolVersion = MbtoolUtils.getSystemMbtoolVersion(getContext());
            mResult.haveRequiredVersion = mResult.mbtoolVersion.compareTo(
                    MbtoolUtils.getMinimumRequiredVersion(Feature.APP_SHARING)) >= 0;
            return mResult;
        }
    }

    public class DismissProgressDialogEvent extends BaseEvent {
    }

    public class ShowVersionTooOldDialogEvent extends BaseEvent {
    }
}
