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

package com.github.chenxiaolong.dualbootpatcher.settings;

import android.content.SharedPreferences;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceFragment;

import com.github.chenxiaolong.dualbootpatcher.MainApplication;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.patcher.PatcherService;

public class RomSettingsFragment extends PreferenceFragment implements OnPreferenceChangeListener {
    public static final String TAG = RomSettingsFragment.class.getSimpleName();

    private static final String KEY_PARALLEL_PATCHING = "parallel_patching_threads";
    private static final String KEY_USE_DARK_THEME = "use_dark_theme";

    private Preference mParallelPatchingPref;
    private Preference mUseDarkThemePref;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        getPreferenceManager().setSharedPreferencesName("settings");

        addPreferencesFromResource(R.xml.rom_settings);

        int threads = getPreferenceManager().getSharedPreferences().getInt(
                KEY_PARALLEL_PATCHING, PatcherService.DEFAULT_PATCHING_THREADS);

        mParallelPatchingPref = findPreference(KEY_PARALLEL_PATCHING);
        mParallelPatchingPref.setDefaultValue(Integer.toString(threads));
        mParallelPatchingPref.setOnPreferenceChangeListener(this);
        updateParallelPatchingSummary(threads);

        mUseDarkThemePref = findPreference(KEY_USE_DARK_THEME);
        mUseDarkThemePref.setOnPreferenceChangeListener(this);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (preference == mParallelPatchingPref) {
            try {
                int threads = Integer.parseInt(newValue.toString());
                if (threads >= 1) {
                    // Do this instead of using EditTextPreference's built-in persisting since we
                    // want it saved as an integer
                    SharedPreferences.Editor editor =
                            getPreferenceManager().getSharedPreferences().edit();
                    editor.putInt(KEY_PARALLEL_PATCHING, threads);
                    editor.apply();

                    updateParallelPatchingSummary(threads);
                    return true;
                }
            } catch (NumberFormatException e) {
            }
        } else if (preference == mUseDarkThemePref) {
            // Apply dark theme and recreate activity
            MainApplication.setUseDarkTheme((Boolean) newValue);
            getActivity().recreate();
            return true;
        }
        return false;
    }

    private void updateParallelPatchingSummary(int threads) {
        String summary = getString(R.string.rom_settings_parallel_patching_desc, threads);
        mParallelPatchingPref.setSummary(summary);
    }
}
