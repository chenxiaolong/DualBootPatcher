/*
 * Copyright (C) 2015-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher

import android.app.Application
import android.content.Context
import android.util.Log
import androidx.appcompat.app.AppCompatDelegate
import com.squareup.leakcanary.LeakCanary
import com.squareup.leakcanary.RefWatcher

class MainApplication : Application() {
    private lateinit var refWatcher: RefWatcher

    override fun onCreate() {
        super.onCreate()

        if (LeakCanary.isInAnalyzerProcess(this)) {
            return
        }

        val defaultUEH = Thread.getDefaultUncaughtExceptionHandler()

        Thread.setDefaultUncaughtExceptionHandler { thread, throwable ->
            Log.e(TAG, "$thread has crashed with an uncaught exception; dumping log")
            LogUtils.dump("crash.log")

            defaultUEH?.uncaughtException(thread, throwable)
        }

        refWatcher = LeakCanary.install(this)

        val prefs = getSharedPreferences("settings", 0)
        useDarkTheme = prefs.getBoolean("use_dark_theme", false)
    }

    companion object {
        private val TAG = MainApplication::class.java.simpleName

        fun getRefWatcher(context: Context): RefWatcher {
            val application = context.applicationContext as MainApplication
            return application.refWatcher
        }

        var useDarkTheme: Boolean
            get() = AppCompatDelegate.getDefaultNightMode() == AppCompatDelegate.MODE_NIGHT_YES
            set(useDarkTheme) = AppCompatDelegate.setDefaultNightMode(
                    if (useDarkTheme) AppCompatDelegate.MODE_NIGHT_YES
                            else AppCompatDelegate.MODE_NIGHT_NO)
    }
}