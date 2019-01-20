/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import android.os.Environment

import java.io.File

class Constants {
    /**
     * Default value constants.
     */
    object Defaults {
        /**
         * Default directory URI for storing ROM backups.
         */
        val BACKUP_DIRECTORY_URI =
                FileUtils.FILE_SCHEME + "://" + Environment.getExternalStorageDirectory() +
                        File.separator + "MultiBoot" +
                        File.separator + "backups"
    }

    /**
     * Preference key constants.
     */
    object Preferences {
        const val BACKUP_DIRECTORY_URI = "backup_directory_uri"
    }
}