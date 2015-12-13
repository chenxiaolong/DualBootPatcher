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
import android.widget.Toast;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomConfig;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.Version;
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppSharingSettingsFragment.NeededInfo;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils.Feature;

public class AppSharingSettingsFragment extends PreferenceFragment implements
        OnPreferenceChangeListener, OnPreferenceClickListener,
        LoaderManager.LoaderCallbacks<NeededInfo> {
    public static final String TAG = AppSharingSettingsFragment.class.getSimpleName();

    private static final String KEY_SHARE_INDIV_APPS = "share_indiv_apps";
    private static final String KEY_MANAGE_INDIV_APPS = "manage_indiv_apps";

    private RomConfig mConfig;

    private CheckBoxPreference mShareIndivApps;
    private Preference mManageIndivApps;

    private boolean mLoading = true;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getPreferenceManager().setSharedPreferencesName("settings");

        addPreferencesFromResource(R.xml.app_sharing_settings);

        mShareIndivApps = (CheckBoxPreference) findPreference(KEY_SHARE_INDIV_APPS);
        mShareIndivApps.setOnPreferenceChangeListener(this);

        mManageIndivApps = findPreference(KEY_MANAGE_INDIV_APPS);
        mManageIndivApps.setOnPreferenceClickListener(this);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        updateState();
        getLoaderManager().initLoader(0, null, this);
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

    private void updateState() {
        int shareResId;
        int manageResId;

        if (mLoading) {
            shareResId = R.string.please_wait;
            manageResId = R.string.please_wait;
        } else {
            shareResId = R.string.a_s_settings_indiv_app_sharing_desc;
            manageResId = R.string.a_s_settings_manage_shared_apps_desc;
        }

        mShareIndivApps.setSummary(shareResId);
        mManageIndivApps.setSummary(manageResId);

        boolean enableShareIndiv = !mLoading;
        boolean enableManageIndiv = !mLoading;
        if (enableManageIndiv && mConfig != null) {
            enableManageIndiv = mConfig.isIndivAppSharingEnabled();
        }
        boolean checkShareIndiv = false;
        if (mConfig != null) {
            checkShareIndiv = mConfig.isIndivAppSharingEnabled();
        }

        mShareIndivApps.setEnabled(enableShareIndiv);
        mManageIndivApps.setEnabled(enableManageIndiv);
        mShareIndivApps.setChecked(checkShareIndiv);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object objValue) {
        final String key = preference.getKey();

        if (KEY_SHARE_INDIV_APPS.equals(key)) {
            mConfig.setIndivAppSharingEnabled((Boolean) objValue);
            updateState();
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

    @Override
    public Loader<NeededInfo> onCreateLoader(int id, Bundle args) {
        return new GetInfo(getActivity());
    }

    @Override
    public void onLoadFinished(Loader<NeededInfo> loader, NeededInfo result) {
        if (result == null) {
            getActivity().finish();
            return;
        }

        if (!result.haveRequiredVersion) {
            Toast.makeText(getActivity(), R.string.a_s_settings_version_too_old, Toast.LENGTH_LONG).show();
            getActivity().finish();
            return;
        }

        mConfig = result.config;

        mLoading = false;
        updateState();
    }

    @Override
    public void onLoaderReset(Loader<NeededInfo> loader) {
    }

    protected static class NeededInfo {
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
            mResult.config = RomConfig.getConfig(currentRom.getConfigPath());
            mResult.mbtoolVersion = MbtoolUtils.getSystemMbtoolVersion(getContext());
            mResult.haveRequiredVersion = mResult.mbtoolVersion.compareTo(
                    MbtoolUtils.getMinimumRequiredVersion(Feature.APP_SHARING)) >= 0;
            return mResult;
        }
    }
}
