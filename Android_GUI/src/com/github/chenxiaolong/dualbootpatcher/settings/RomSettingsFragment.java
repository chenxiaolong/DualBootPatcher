/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceFragment;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.dualbootpatcher.MainActivity;
import com.github.chenxiaolong.dualbootpatcher.MiscUtils;
import com.github.chenxiaolong.dualbootpatcher.R;

public class RomSettingsFragment extends PreferenceFragment implements
        OnPreferenceChangeListener {
    public static final String TAG = "rom_settings";

    private static final String KEY_SHARE_APPS = "share_apps";
    private static final String KEY_SHARE_PAID_APPS = "share_paid_apps";
    private static final String KEY_SHOW_REBOOT = "show_reboot";
    private static final String KEY_SHOW_EXIT = "show_exit";

    private DisableableCheckBoxPreference mShareApps;
    private DisableableCheckBoxPreference mSharePaidApps;
    private CheckBoxPreference mShowReboot;
    private CheckBoxPreference mShowExit;

    private boolean mAttemptedRoot;
    private boolean mHaveRootAccess;

    public static RomSettingsFragment newInstance() {
        RomSettingsFragment f = new RomSettingsFragment();

        return f;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getPreferenceManager().setSharedPreferencesName("settings");

        addPreferencesFromResource(R.xml.rom_settings);

        mShareApps = (DisableableCheckBoxPreference) findPreference(KEY_SHARE_APPS);
        mSharePaidApps = (DisableableCheckBoxPreference) findPreference(KEY_SHARE_PAID_APPS);

        mShareApps.setOnPreferenceChangeListener(this);
        mSharePaidApps.setOnPreferenceChangeListener(this);

        mShowReboot = (CheckBoxPreference) findPreference(KEY_SHOW_REBOOT);
        mShowExit = (CheckBoxPreference) findPreference(KEY_SHOW_EXIT);

        mShowReboot.setOnPreferenceChangeListener(this);
        mShowExit.setOnPreferenceChangeListener(this);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        if (savedInstanceState != null) {
            mAttemptedRoot = savedInstanceState.getBoolean("attemptedRoot");
            mHaveRootAccess = savedInstanceState.getBoolean("haveRootAccess");
        }

        // Get root access
        if (!mAttemptedRoot) {
            mAttemptedRoot = true;
            mHaveRootAccess = CommandUtils.requestRootAccess();
        }

        String version = MiscUtils.getPatchedByVersion();

        if (!mHaveRootAccess) {
            mShareApps.setSummary(R.string.rom_settings_noroot_desc);
            mSharePaidApps.setSummary(R.string.rom_settings_noroot_desc);
            mShareApps.setVisuallyEnabled(false);
            mSharePaidApps.setVisuallyEnabled(false);
        } else if (MiscUtils.compareVersions(version, "8.0.0") < 0) {
            mShareApps.setSummary(String.format(
                    getActivity().getString(R.string.rom_settings_too_old),
                    "8.0.0"));
            mSharePaidApps.setSummary(String.format(
                    getActivity().getString(R.string.rom_settings_too_old),
                    "8.0.0"));
            mShareApps.setEnabled(false);
            mSharePaidApps.setEnabled(false);

            mShareApps.setChecked(SettingsUtils.isShareAppsEnabled());
            mSharePaidApps.setChecked(SettingsUtils.isSharePaidAppsEnabled());
        } else {
            mShareApps.setSummary(R.string.rom_settings_share_apps_desc);
            mSharePaidApps
                    .setSummary(R.string.rom_settings_share_paid_apps_desc);
            mShareApps.setVisuallyEnabled(true);
            mSharePaidApps.setVisuallyEnabled(true);

            mShareApps.setChecked(SettingsUtils.isShareAppsEnabled());
            mSharePaidApps.setChecked(SettingsUtils.isSharePaidAppsEnabled());
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putBoolean("attemptedRoot", mAttemptedRoot);
        outState.putBoolean("haveRootAccess", mHaveRootAccess);
    }

    private void reloadFragment() {
        FragmentManager fm = getFragmentManager();
        FragmentTransaction ft = fm.beginTransaction();
        ft.detach(this);
        ft.attach(this);
        ft.commit();
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object objValue) {
        final String key = preference.getKey();

        if (KEY_SHARE_APPS.equals(key)) {
            if (((DisableableCheckBoxPreference) preference)
                    .isVisuallyEnabled()) {
                // Shouldn't be doing this on the UI thread, but this command
                // returns very quickly
                return SettingsUtils.setShareAppsEnabled((Boolean) objValue);
            } else {
                mAttemptedRoot = false;
                reloadFragment();
                return false;
            }
        } else if (KEY_SHARE_PAID_APPS.equals(key)) {
            if (((DisableableCheckBoxPreference) preference)
                    .isVisuallyEnabled()) {
                // Shouldn't be doing this on the UI thread, but this command
                // returns very quickly
                return SettingsUtils
                        .setSharePaidAppsEnabled((Boolean) objValue);
            } else {
                mAttemptedRoot = false;
                reloadFragment();
                return false;
            }
        } else if (KEY_SHOW_REBOOT.equals(key)) {
            ((MainActivity) getActivity()).showReboot((Boolean) objValue);
        } else if (KEY_SHOW_EXIT.equals(key)) {
            ((MainActivity) getActivity()).showExit((Boolean) objValue);
        }

        return true;
    }
}
