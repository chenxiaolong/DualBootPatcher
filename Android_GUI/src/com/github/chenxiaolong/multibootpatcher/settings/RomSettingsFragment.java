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

import android.app.Dialog;
import android.app.DialogFragment;
import android.content.Context;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.PreferenceCategory;
import android.preference.PreferenceFragment;

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
        EventCollectorListener {
    public static final String TAG = RomSettingsFragment.class.getSimpleName();

    private static final String KEY_BOOTING_CATEGORY = "booting_category";
    private static final String KEY_UPDATE_RAMDISK = "update_ramdisk";

    private RomSettingsEventCollector mEventCollector;

    private PreferenceCategory mBootingCategory;

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

        Version version = MbtoolUtils.getSystemMbtoolVersion(getActivity());
        if (version.compareTo(MbtoolUtils.getMinimumRequiredVersion(Feature.DAEMON)) >= 0) {
            //bootingCategory.removePreference(updateRamdisk);
            getPreferenceScreen().removePreference(mBootingCategory);
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
    public boolean onPreferenceClick(Preference preference) {
        final String key = preference.getKey();

        if (KEY_UPDATE_RAMDISK.equals(key)) {
            IndeterminateProgressDialog d = IndeterminateProgressDialog.newInstance();
            d.show(getFragmentManager(), IndeterminateProgressDialog.TAG);

            mEventCollector.updateRamdisk();
        }

        return true;
    }

    @Override
    public void onEventReceived(BaseEvent event) {
        if (event instanceof UpdatedRamdiskEvent) {
            UpdatedRamdiskEvent e = (UpdatedRamdiskEvent) event;

            IndeterminateProgressDialog d = (IndeterminateProgressDialog) getFragmentManager()
                    .findFragmentByTag(IndeterminateProgressDialog.TAG);
            if (d != null) {
                d.dismiss();
            }

            UpdateRamdiskResultDialog dialog = UpdateRamdiskResultDialog.newInstance(e.success);
            dialog.show(getFragmentManager(), UpdateRamdiskResultDialog.TAG);
        }
    }

    public static class IndeterminateProgressDialog extends DialogFragment {
        private static final String TAG = IndeterminateProgressDialog.class.getSimpleName();

        public static IndeterminateProgressDialog newInstance() {
            IndeterminateProgressDialog frag = new IndeterminateProgressDialog();
            return frag;
        }

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            Dialog dialog = new MaterialDialog.Builder(getActivity())
                    .content(R.string.please_wait)
                    .progress(true, 0)
                    .build();

            setCancelable(false);
            dialog.setCanceledOnTouchOutside(false);

            return dialog;
        }
    }

    public static class UpdateRamdiskResultDialog extends DialogFragment {
        private static final String TAG = UpdateRamdiskResultDialog.class.getSimpleName();

        private static final String ARG_SUCCEEDED = "succeeded";

        public static UpdateRamdiskResultDialog newInstance(boolean succeeded) {
            UpdateRamdiskResultDialog frag = new UpdateRamdiskResultDialog();
            Bundle args = new Bundle();
            args.putBoolean(ARG_SUCCEEDED, succeeded);
            frag.setArguments(args);
            return frag;
        }

        @Override
        public Dialog onCreateDialog(Bundle savedInstanceState) {
            boolean succeeded = getArguments().getBoolean(ARG_SUCCEEDED);

            MaterialDialog.Builder builder = new MaterialDialog.Builder(getActivity());

            if (!succeeded) {
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

            Dialog dialog = builder.build();

            setCancelable(false);
            dialog.setCanceledOnTouchOutside(false);

            return dialog;
        }
    }
}
