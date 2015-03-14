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

package com.github.chenxiaolong.dualbootpatcher.settings;

import android.app.LoaderManager;
import android.content.AsyncTaskLoader;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
import android.content.Loader;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.support.annotation.NonNull;
import android.text.Html;
import android.view.View;
import android.view.animation.AlphaAnimation;

import com.afollestad.materialdialogs.MaterialDialog;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.settings.AppSharingSettingsFragment.NeededInfo;
import com.github.chenxiaolong.multibootpatcher.Version;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolUtils;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolUtils.Feature;

public class AppSharingSettingsFragment extends PreferenceFragment implements
        OnPreferenceChangeListener, OnPreferenceClickListener, OnDismissListener,
        LoaderManager.LoaderCallbacks<NeededInfo> {
    public static final String TAG = AppSharingSettingsFragment.class.getSimpleName();

    private static final String EXTRA_PROGRESS_DIALOG = "progressDialog";

    private static final String KEY_APP_SHARING_CATEGORY = "app_sharing_category";
    private static final String KEY_SHARE_APPS = "share_apps";
    private static final String KEY_SHARE_PAID_APPS = "share_paid_apps";
    private static final String KEY_SHARE_INDIV_APPS = "share_indiv_apps";

    private MaterialDialog mProgressDialog;

    private RomInformation mCurrentRom;
    private boolean mHaveRequiredVersion;

    private PreferenceCategory mAppSharingCategory;

    private CheckBoxPreference mShareApps;
    private CheckBoxPreference mSharePaidApps;
    private Preference mShareIndivApps;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getPreferenceManager().setSharedPreferencesName("settings");

        addPreferencesFromResource(R.xml.app_sharing_settings);

        mAppSharingCategory = (PreferenceCategory) findPreference(KEY_APP_SHARING_CATEGORY);

        mShareApps = (CheckBoxPreference) findPreference(KEY_SHARE_APPS);
        mSharePaidApps = (CheckBoxPreference) findPreference(KEY_SHARE_PAID_APPS);
        mShareApps.setOnPreferenceChangeListener(this);
        mSharePaidApps.setOnPreferenceChangeListener(this);

        mShareIndivApps = findPreference(KEY_SHARE_INDIV_APPS);
        mShareIndivApps.setOnPreferenceClickListener(this);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        getView().setVisibility(View.GONE);

        if (savedInstanceState != null) {
            if (savedInstanceState.getBoolean(EXTRA_PROGRESS_DIALOG)) {
                buildProgressDialog();
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
    }

    @Override
    public void onStop() {
        super.onStop();

        if (mProgressDialog != null) {
            mProgressDialog.dismiss();
            mProgressDialog = null;
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

    @Override
    public boolean onPreferenceChange(Preference preference, Object objValue) {
        final String key = preference.getKey();

        if (KEY_SHARE_APPS.equals(key)) {
            // Shouldn't be doing this on the UI thread, but this command
            // returns very quickly
            boolean ret = AppSharingUtils.setShareAppsEnabled((Boolean) objValue);
            setupIndivAppSharePrefs(mSharePaidApps.isChecked() || (Boolean) objValue);
            return ret;
        } else if (KEY_SHARE_PAID_APPS.equals(key)) {
            // Shouldn't be doing this on the UI thread, but this command
            // returns very quickly
            boolean ret = AppSharingUtils.setSharePaidAppsEnabled((Boolean) objValue);
            setupIndivAppSharePrefs(mShareApps.isChecked() || (Boolean) objValue);
            return ret;
        }

        return true;
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        final String key = preference.getKey();

        if (KEY_SHARE_INDIV_APPS.equals(key)) {
            startActivity(new Intent(getActivity(), AppListActivity.class));
        }

        return true;
    }

    private void showAppSharingPrefs() {
        mShareApps.setChecked(AppSharingUtils.isShareAppsEnabled());
        mSharePaidApps.setChecked(AppSharingUtils.isSharePaidAppsEnabled());
    }

    private void setupGlobalAppSharePrefs() {
        boolean isPrimary = mCurrentRom != null && mCurrentRom.getId().equals(RomUtils.PRIMARY_ID);

        if (isPrimary) {
            mAppSharingCategory.removePreference(mShareApps);
            mAppSharingCategory.removePreference(mSharePaidApps);
            return;
        }

        mSharePaidApps.setSummary(Html.fromHtml(getActivity().getString(
                R.string.a_s_settings_share_paid_apps_desc)));

        if (!mHaveRequiredVersion) {
            mShareApps.setEnabled(false);
            mSharePaidApps.setEnabled(false);
        }
    }

    private void setupIndivAppSharePrefs(boolean globalShared) {
        if (globalShared) {
            // If global app sharing is enabled, then don't allow individual app sharing
            mShareIndivApps.setSummary(R.string.indiv_app_sharing_incompat_global);
            mShareIndivApps.setEnabled(false);
        } else {
            mShareIndivApps.setSummary(R.string.a_s_settings_indiv_app_sharing_desc);
            mShareIndivApps.setEnabled(mHaveRequiredVersion);
        }
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        if (mProgressDialog != null && mProgressDialog == dialog) {
            mProgressDialog = null;
        }
    }

    @Override
    public Loader<NeededInfo> onCreateLoader(int id, Bundle args) {
        return new GetInfo(getActivity());
    }

    @Override
    public void onLoadFinished(Loader<NeededInfo> loader, NeededInfo result) {
        mCurrentRom = result.currentRom;
        mHaveRequiredVersion = result.haveRequiredVersion;

        showAppSharingPrefs();

        setupGlobalAppSharePrefs();
        setupIndivAppSharePrefs(mShareApps.isChecked() || mSharePaidApps.isChecked());

        AlphaAnimation fadeAnimation = new AlphaAnimation(0, 1);
        fadeAnimation.setDuration(100);
        fadeAnimation.setFillAfter(true);

        getView().startAnimation(fadeAnimation);
        getView().setVisibility(View.VISIBLE);

        if (mProgressDialog != null) {
            mProgressDialog.dismiss();
        }
    }

    @Override
    public void onLoaderReset(Loader<NeededInfo> loader) {
    }

    protected static class NeededInfo {
        RomInformation currentRom;
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
            mResult = new NeededInfo();
            mResult.currentRom = RomUtils.getCurrentRom(getContext());
            mResult.mbtoolVersion = MbtoolUtils.getSystemMbtoolVersion(getContext());
            mResult.haveRequiredVersion = mResult.mbtoolVersion.compareTo(
                    MbtoolUtils.getMinimumRequiredVersion(Feature.APP_SHARING)) >= 0;
            return mResult;
        }
    }
}
