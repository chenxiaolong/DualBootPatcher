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
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;
import android.text.Html;
import android.view.View;
import android.view.animation.AlphaAnimation;

import com.github.chenxiaolong.dualbootpatcher.MiscUtils;
import com.github.chenxiaolong.dualbootpatcher.MiscUtils.Version;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.RootCheckerFragment;
import com.github.chenxiaolong.dualbootpatcher.RootCheckerFragment.RootCheckerListener;

public class RomSettingsFragment extends PreferenceFragment implements
        OnPreferenceChangeListener, OnPreferenceClickListener, RootCheckerListener,
        OnDismissListener {
    public static final String TAG = RomSettingsFragment.class.getSimpleName();

    private static final String KEY_APP_SHARING_CATEGORY = "app_sharing_category";
    private static final String KEY_NO_ROOT = "no_root";
    private static final String KEY_SHARE_APPS = "share_apps";
    private static final String KEY_SHARE_PAID_APPS = "share_paid_apps";
    private static final String KEY_SHARE_INDIV_APPS = "share_indiv_apps";

    private RootCheckerFragment mRootCheckerFragment;
    private ProgressDialog mProgressDialog;
    private boolean mAttemptedRoot;

    private PreferenceCategory mAppSharingCategory;

    private Preference mNoRoot;
    private CheckBoxPreference mShareApps;
    private CheckBoxPreference mSharePaidApps;
    private Preference mShareIndivApps;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getPreferenceManager().setSharedPreferencesName("settings");

        addPreferencesFromResource(R.xml.rom_settings);

        mAppSharingCategory = (PreferenceCategory) findPreference(KEY_APP_SHARING_CATEGORY);

        mNoRoot = findPreference(KEY_NO_ROOT);
        mNoRoot.setOnPreferenceClickListener(this);

        mShareApps = (CheckBoxPreference) findPreference(KEY_SHARE_APPS);
        mSharePaidApps = (CheckBoxPreference) findPreference(KEY_SHARE_PAID_APPS);
        mShareApps.setOnPreferenceChangeListener(this);
        mSharePaidApps.setOnPreferenceChangeListener(this);

        mShareIndivApps = findPreference(KEY_SHARE_INDIV_APPS);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        getView().setVisibility(View.GONE);

        if (savedInstanceState != null) {
            mAttemptedRoot = savedInstanceState.getBoolean("attemptedRoot");

            if (!mAttemptedRoot && savedInstanceState.getBoolean("haveDialog")) {
                buildProgressDialog();
            }
        }

        mRootCheckerFragment = RootCheckerFragment.getInstance(getFragmentManager());

        if (!mAttemptedRoot) {
            mAttemptedRoot = true;
            buildProgressDialog();
            mRootCheckerFragment.requestRoot();
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putBoolean("attemptedRoot", mAttemptedRoot);

        if (mProgressDialog != null) {
            outState.putBoolean("haveDialog", true);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        mRootCheckerFragment.attachListenerAndResendEvents(this);
    }

    @Override
    public void onPause() {
        super.onPause();
        mRootCheckerFragment.detachListener(this);
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
        if (mProgressDialog == null) {
            mProgressDialog = new ProgressDialog(getActivity());
            mProgressDialog.setMessage(getString(R.string.waiting_for_root));
            mProgressDialog.setCancelable(false);
            mProgressDialog.setIndeterminate(true);
            mProgressDialog.show();
            mProgressDialog.setOnDismissListener(this);
        }
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
            // Shouldn't be doing this on the UI thread, but this command
            // returns very quickly
            return AppSharingUtils.setShareAppsEnabled((Boolean) objValue);
        } else if (KEY_SHARE_PAID_APPS.equals(key)) {
            // Shouldn't be doing this on the UI thread, but this command
            // returns very quickly
            return AppSharingUtils.setSharePaidAppsEnabled((Boolean) objValue);
        }

        return true;
    }

    @Override
    public boolean onPreferenceClick(Preference preference) {
        final String key = preference.getKey();

        if (KEY_NO_ROOT.equals(key)) {
            mAttemptedRoot = false;
            reloadFragment();
            return false;
        }

        return true;
    }

    private void showAppSharingPrefs(boolean haveRootAccess) {
        if (!haveRootAccess) {
            mAppSharingCategory.removePreference(mShareApps);
            mAppSharingCategory.removePreference(mSharePaidApps);
            mAppSharingCategory.removePreference(mShareIndivApps);
        } else {
            mAppSharingCategory.removePreference(mNoRoot);
            mAppSharingCategory.addPreference(mShareApps);
            mAppSharingCategory.addPreference(mSharePaidApps);
            mAppSharingCategory.addPreference(mShareIndivApps);

            mShareApps.setChecked(AppSharingUtils.isShareAppsEnabled());
            mSharePaidApps.setChecked(AppSharingUtils.isSharePaidAppsEnabled());
        }
    }

    @Override
    public void rootRequestAcknowledged(boolean allowed) {
        Version version = new Version(MiscUtils.getPatchedByVersion());
        Version verGlobalShareApps = MiscUtils.getMinimumVersionFor(
                MiscUtils.FEATURE_GLOBAL_APP_SHARING | MiscUtils.FEATURE_GLOBAL_PAID_APP_SHARING);
        Version verIndivAppSync = MiscUtils.getMinimumVersionFor(
                MiscUtils.FEATURE_INDIV_APP_SYNCING);

        showAppSharingPrefs(allowed);

        if (allowed) {
            RomInformation curRom = RomUtils.getCurrentRom();
            boolean isPrimary = curRom != null && curRom.id.equals(RomUtils.PRIMARY_ID);

            if (isPrimary) {
                mAppSharingCategory.removePreference(mShareApps);
                mAppSharingCategory.removePreference(mSharePaidApps);
            }

            if (version.compareTo(verGlobalShareApps) < 0) {
                mShareApps.setSummary(String.format(getActivity().getString(
                        R.string.rom_settings_too_old), verGlobalShareApps));

                mSharePaidApps.setSummary(String.format(getActivity().getString(
                        R.string.rom_settings_too_old), verGlobalShareApps));
                mShareApps.setEnabled(false);
                mSharePaidApps.setEnabled(false);
            } else {
                mSharePaidApps.setSummary(Html.fromHtml(getActivity().getString(
                        R.string.rom_settings_share_paid_apps_desc)));
            }

            // Show warning if we're not booted in primary and the ramdisk does not have syncdaemon
            if (!isPrimary && version.compareTo(verIndivAppSync) < 0) {
                mShareIndivApps.setSummary(String.format(getActivity().getString(
                        R.string.rom_settings_too_old), verIndivAppSync));
                mShareIndivApps.setEnabled(false);
            }
        }

        AlphaAnimation fadeOutAnimation = new AlphaAnimation(0, 1);
        fadeOutAnimation.setDuration(100);
        fadeOutAnimation.setFillAfter(true);

        getView().startAnimation(fadeOutAnimation);
        getView().setVisibility(View.VISIBLE);
        if (mProgressDialog != null) {
            mProgressDialog.dismiss();
        }
    }

    @Override
    public void onDismiss(DialogInterface dialogInterface) {
        if (mProgressDialog != null) {
            mProgressDialog = null;
        }
    }
}
