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

package com.github.chenxiaolong.multibootpatcher.appsharing;

import android.app.FragmentManager;

import com.github.chenxiaolong.multibootpatcher.EventCollector;

public class AppSharingEventCollector extends EventCollector {
    public static final String TAG = AppSharingEventCollector.class.getSimpleName();

    public static AppSharingEventCollector getInstance(FragmentManager fm) {
        AppSharingEventCollector asec = (AppSharingEventCollector) fm.findFragmentByTag(TAG);
        if (asec == null) {
            asec = new AppSharingEventCollector();
            fm.beginTransaction().add(asec, AppSharingEventCollector.TAG).commit();
        }
        return asec;
    }
}
