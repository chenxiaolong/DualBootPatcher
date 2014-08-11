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

import android.app.AlertDialog;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
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

import com.github.chenxiaolong.dualbootpatcher.EventCollector.BaseEvent;
import com.github.chenxiaolong.dualbootpatcher.EventCollector.EventCollectorListener;
import com.github.chenxiaolong.dualbootpatcher.MiscUtils.Version;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.github.chenxiaolong.dualbootpatcher.RootCheckerEventCollector;
import com.github.chenxiaolong.dualbootpatcher.RootCheckerEventCollector.RootAcknowledgedEvent;
import com.github.chenxiaolong.dualbootpatcher.settings.AppSharingEventCollector.SettingsInfoEvent;
import com.github.chenxiaolong.dualbootpatcher.settings.AppSharingEventCollector
        .UpdatedRamdiskEvent;
import com.github.chenxiaolong.dualbootpatcher.switcher.SwitcherUtils;

public class RomSettingsFragment extends PreferenceFragment implements
        OnPreferenceChangeListener, OnPreferenceClickListener, OnDismissListener,
        EventCollectorListener {
    public static final String TAG = RomSettingsFragment.class.getSimpleName();

    private static final String MINIMUM_VERSION = "8.0.0.r83";

    private static final String EXTRA_ROOT_CHECK_DIALOG = "rootCheckDialog";
    private static final String EXTRA_UPDATE_RAMDISK_DIALOG = "updateRamdiskDialog";
    private static final String EXTRA_UPDATE_RAMDISK_DONE_DIALOG = "updateRamdiskDoneDialog";
    private static final String EXTRA_UPDATE_FAILED = "updateFailed";
    private static final String EXTRA_SYNCDAEMON_VERSION = "syncdaemonVersion";
    private static final String EXTRA_CURRENT_ROM = "currentRom";

    private static final String KEY_APP_SHARING_CATEGORY = "app_sharing_category";
    private static final String KEY_NO_ROOT = "no_root";
    private static final String KEY_SHARE_APPS = "share_apps";
    private static final String KEY_SHARE_PAID_APPS = "share_paid_apps";
    private static final String KEY_SHARE_INDIV_APPS = "share_indiv_apps";
    private static final String KEY_UPDATE_RAMDISK = "update_ramdisk";

    private boolean mOverMin;

    private AppSharingEventCollector mEventCollector;
    private RootCheckerEventCollector mRootChecker;
    private ProgressDialog mProgressDialog;
    private boolean mRootAllowed;

    private String mSyncdaemonVersion;
    private RomInformation mCurrentRom;

    private PreferenceCategory mAppSharingCategory;

    private Preference mNoRoot;
    private CheckBoxPreference mShareApps;
    private CheckBoxPreference mSharePaidApps;
    private Preference mShareIndivApps;

    private Preference mUpdateRamdisk;
    private ProgressDialog mRamdiskDialog;
    private AlertDialog mUpdateDoneDialog;
    private boolean mUpdateFailed;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        FragmentManager fm = getFragmentManager();
        mEventCollector = (AppSharingEventCollector) fm.findFragmentByTag(
                AppSharingEventCollector.TAG);

        if (mEventCollector == null) {
            mEventCollector = new AppSharingEventCollector();
            fm.beginTransaction().add(mEventCollector, AppSharingEventCollector.TAG).commit();
        }

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
        mShareIndivApps.setOnPreferenceClickListener(this);

        mUpdateRamdisk = findPreference(KEY_UPDATE_RAMDISK);
        mUpdateRamdisk.setOnPreferenceClickListener(this);

        if (savedInstanceState != null) {
            mSyncdaemonVersion = savedInstanceState.getString(EXTRA_SYNCDAEMON_VERSION);
            mCurrentRom = savedInstanceState.getParcelable(EXTRA_CURRENT_ROM);
        }
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        getView().setVisibility(View.GONE);

        if (savedInstanceState != null) {
            if (savedInstanceState.getBoolean(EXTRA_ROOT_CHECK_DIALOG)) {
                buildProgressDialog();
            }
        }

        mRootChecker = RootCheckerEventCollector.getInstance(getFragmentManager());

        if (!mRootChecker.isAttemptedRoot()) {
            buildProgressDialog();
            mRootChecker.createListener(TAG);
            mRootChecker.requestRoot();
        }

        if (savedInstanceState != null) {
            mUpdateFailed = savedInstanceState.getBoolean(EXTRA_UPDATE_FAILED);

            if (savedInstanceState.getBoolean(EXTRA_UPDATE_RAMDISK_DIALOG, false)) {
                buildUpdateRamdiskDialog();
            }

            if (savedInstanceState.getBoolean(EXTRA_UPDATE_RAMDISK_DONE_DIALOG, false)) {
                buildUpdateRamdiskDoneDialog();
            }
        }
    }

    @Override
    public void onSaveInstanceState(Bundle outState) {
        super.onSaveInstanceState(outState);

        outState.putBoolean(EXTRA_UPDATE_FAILED, mUpdateFailed);
        outState.putString(EXTRA_SYNCDAEMON_VERSION, mSyncdaemonVersion);
        outState.putParcelable(EXTRA_CURRENT_ROM, mCurrentRom);

        if (mProgressDialog != null) {
            outState.putBoolean(EXTRA_ROOT_CHECK_DIALOG, true);
        }

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
        mRootChecker.attachListener(TAG, this);
        mEventCollector.attachListener(TAG, this);
    }

    @Override
    public void onPause() {
        super.onPause();
        mRootChecker.detachListener(TAG);
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
        if (mProgressDialog == null) {
            mProgressDialog = new ProgressDialog(getActivity());
            mProgressDialog.setMessage(getString(R.string.waiting_for_root));
            mProgressDialog.setCancelable(false);
            mProgressDialog.setIndeterminate(true);
            mProgressDialog.show();
            mProgressDialog.setOnDismissListener(this);
        }
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

            if (mUpdateFailed) {
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

        if (KEY_NO_ROOT.equals(key)) {
            mRootChecker.resetAttempt();
            reloadFragment();
            return false;
        } else if (KEY_SHARE_INDIV_APPS.equals(key)) {
            startActivity(new Intent(getActivity(), AppListActivity.class));
        } else if (KEY_UPDATE_RAMDISK.equals(key)) {
            buildUpdateRamdiskDialog();
            mEventCollector.updateRamdisk();
        }

        return true;
    }

    private void showAppSharingPrefs(boolean haveRootAccess) {
        if (!haveRootAccess) {
            mAppSharingCategory.removePreference(mShareApps);
            mAppSharingCategory.removePreference(mSharePaidApps);
            mAppSharingCategory.removePreference(mShareIndivApps);
            mAppSharingCategory.removePreference(mUpdateRamdisk);
        } else {
            mAppSharingCategory.removePreference(mNoRoot);
            mAppSharingCategory.addPreference(mShareApps);
            mAppSharingCategory.addPreference(mSharePaidApps);
            mAppSharingCategory.addPreference(mShareIndivApps);
            mAppSharingCategory.addPreference(mUpdateRamdisk);

            mShareApps.setChecked(AppSharingUtils.isShareAppsEnabled());
            mSharePaidApps.setChecked(AppSharingUtils.isSharePaidAppsEnabled());
        }
    }

    private void setupGlobalAppSharePrefs() {
        boolean isPrimary = mCurrentRom != null && mCurrentRom.id.equals(RomUtils.PRIMARY_ID);

        if (isPrimary) {
            mAppSharingCategory.removePreference(mShareApps);
            mAppSharingCategory.removePreference(mSharePaidApps);
            return;
        }

        mSharePaidApps.setSummary(Html.fromHtml(getActivity().getString(
                R.string.rom_settings_share_paid_apps_desc)));

        if (!mOverMin) {
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
            mShareIndivApps.setSummary(R.string.rom_settings_indiv_app_sharing_desc);
            mShareIndivApps.setEnabled(mOverMin);
        }
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

            mUpdateFailed = e.failed;
            buildUpdateRamdiskDoneDialog();
        } else if (event instanceof SettingsInfoEvent) {
            SettingsInfoEvent e = (SettingsInfoEvent) event;

            if (e.syncDaemonVersion != null) {
                mSyncdaemonVersion = e.syncDaemonVersion;
            } else {
                mSyncdaemonVersion = "0.0.0";
            }

            mCurrentRom = e.currentRom;

            ready();
        } else if (event instanceof RootAcknowledgedEvent) {
            RootAcknowledgedEvent e = (RootAcknowledgedEvent) event;

            mRootAllowed = e.allowed;

            if (mRootAllowed && mSyncdaemonVersion == null) {
                mEventCollector.getInfo();
            } else {
                ready();
            }
        }
    }

    private void ready() {
        showAppSharingPrefs(mRootAllowed);

        if (mRootAllowed) {
            Version minVersion = new Version(MINIMUM_VERSION);
            Version sdVersion = new Version(mSyncdaemonVersion);

            mOverMin = sdVersion.compareTo(minVersion) >= 0;

            if (mOverMin) {
                mAppSharingCategory.removePreference(mUpdateRamdisk);
            }

            setupGlobalAppSharePrefs();
            setupIndivAppSharePrefs(mShareApps.isChecked() || mSharePaidApps.isChecked());
        }

        AlphaAnimation fadeAnimation = new AlphaAnimation(0, 1);
        fadeAnimation.setDuration(100);
        fadeAnimation.setFillAfter(true);

        getView().startAnimation(fadeAnimation);
        getView().setVisibility(View.VISIBLE);
        if (mProgressDialog != null) {
            mProgressDialog.dismiss();
        }
    }
}
