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
import android.support.annotation.Nullable;
import android.support.v13.app.FragmentCompat;
import android.widget.Toast;

import com.github.chenxiaolong.dualbootpatcher.PermissionUtils;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomConfig;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.Version;
import com.github.chenxiaolong.dualbootpatcher.appsharing.AppSharingSettingsFragment.NeededInfo;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericConfirmDialog
        .GenericConfirmDialogListener;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog;
import com.github.chenxiaolong.dualbootpatcher.dialogs.GenericYesNoDialog
        .GenericYesNoDialogListener;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolConnection;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils;
import com.github.chenxiaolong.dualbootpatcher.socket.MbtoolUtils.Feature;
import com.github.chenxiaolong.dualbootpatcher.socket.interfaces.MbtoolInterface;

import org.apache.commons.io.IOUtils;

public class AppSharingSettingsFragment extends PreferenceFragment implements
        OnPreferenceChangeListener, OnPreferenceClickListener,
        LoaderManager.LoaderCallbacks<NeededInfo>,
        FragmentCompat.OnRequestPermissionsResultCallback,
        GenericYesNoDialogListener, GenericConfirmDialogListener {
    private static final String KEY_SHARE_INDIV_APPS = "share_indiv_apps";
    private static final String KEY_MANAGE_INDIV_APPS = "manage_indiv_apps";

    private static final String YES_NO_DIALOG_PERMISSIONS =
            AppSharingSettingsFragment.class.getCanonicalName() + ".yes_no.permissions";
    private static final String CONFIRM_DIALOG_PERMISSIONS =
            AppSharingSettingsFragment.class.getCanonicalName() + ".confirm.permissions";

    /**
     * Request code for storage permissions request
     * (used in {@link #onRequestPermissionsResult(int, String[], int[])})
     */
    private static final int PERMISSIONS_REQUEST_STORAGE = 1;

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

        if (PermissionUtils.supportsRuntimePermissions()) {
            if (savedInstanceState == null) {
                requestPermissions();
            } else if (PermissionUtils.hasPermissions(
                    getActivity(), PermissionUtils.STORAGE_PERMISSIONS)) {
                onPermissionsGranted();
            }
        } else {
            startLoading();
        }
    }

    @Override
    public void onStop() {
        super.onStop();

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

    private void startLoading() {
        getLoaderManager().initLoader(0, null, this);
    }

    private void requestPermissions() {
        FragmentCompat.requestPermissions(
                this, PermissionUtils.STORAGE_PERMISSIONS, PERMISSIONS_REQUEST_STORAGE);
    }

    private void showPermissionsRationaleDialogYesNo() {
        GenericYesNoDialog dialog = (GenericYesNoDialog)
                getFragmentManager().findFragmentByTag(YES_NO_DIALOG_PERMISSIONS);
        if (dialog == null) {
            GenericYesNoDialog.Builder builder = new GenericYesNoDialog.Builder();
            builder.message(R.string.a_s_settings_storage_permission_required);
            builder.positive(R.string.try_again);
            builder.negative(R.string.cancel);
            dialog = builder.buildFromFragment(YES_NO_DIALOG_PERMISSIONS, this);
            dialog.show(getFragmentManager(), YES_NO_DIALOG_PERMISSIONS);
        }
    }

    private void showPermissionsRationaleDialogConfirm() {
        GenericConfirmDialog dialog = (GenericConfirmDialog)
                getFragmentManager().findFragmentByTag(CONFIRM_DIALOG_PERMISSIONS);
        if (dialog == null) {
            GenericConfirmDialog.Builder builder = new GenericConfirmDialog.Builder();
            builder.message(R.string.a_s_settings_storage_permission_required);
            builder.buttonText(R.string.ok);
            dialog = builder.buildFromFragment(CONFIRM_DIALOG_PERMISSIONS, this);
            dialog.show(getFragmentManager(), CONFIRM_DIALOG_PERMISSIONS);
        }
    }

    private void onPermissionsGranted() {
        startLoading();
    }

    private void onPermissionsDenied() {
        getActivity().finish();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        if (requestCode == PERMISSIONS_REQUEST_STORAGE) {
            if (PermissionUtils.verifyPermissions(grantResults)) {
                onPermissionsGranted();
            } else {
                if (PermissionUtils.shouldShowRationales(this, permissions)) {
                    showPermissionsRationaleDialogYesNo();
                } else {
                    showPermissionsRationaleDialogConfirm();
                }
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }


    @Override
    public void onConfirmYesNo(@Nullable String tag, boolean choice) {
        if (YES_NO_DIALOG_PERMISSIONS.equals(tag)) {
            if (choice) {
                requestPermissions();
            } else {
                onPermissionsDenied();
            }
        }
    }

    @Override
    public void onConfirmOk(@Nullable String tag) {
        if (CONFIRM_DIALOG_PERMISSIONS.equals(tag)) {
            onPermissionsDenied();
        }
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
            Toast.makeText(getActivity(), R.string.a_s_settings_version_too_old,
                    Toast.LENGTH_LONG).show();
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
            RomInformation currentRom;

            MbtoolConnection conn = null;

            try {
                conn = new MbtoolConnection(getContext());
                MbtoolInterface iface = conn.getInterface();

                currentRom = RomUtils.getCurrentRom(getContext(), iface);
            } catch (Exception e) {
                return null;
            } finally {
                IOUtils.closeQuietly(conn);
            }

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
