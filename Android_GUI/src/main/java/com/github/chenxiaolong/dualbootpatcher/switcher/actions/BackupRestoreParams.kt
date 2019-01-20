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

package com.github.chenxiaolong.dualbootpatcher.switcher.actions

import android.net.Uri
import android.os.Parcel
import android.os.Parcelable

class BackupRestoreParams : Parcelable {
    var action: Action? = null
    var romId: String? = null
    var targets: Array<String>? = null
    var backupName: String? = null
    var backupDirUri: Uri? = null
    var force: Boolean = false

    enum class Action {
        BACKUP,
        RESTORE
    }

    constructor()

    constructor(action: Action, romId: String, targets: Array<String>, backupName: String,
                backupDirUri: Uri, force: Boolean) {
        this.action = action
        this.romId = romId
        this.targets = targets
        this.backupName = backupName
        this.backupDirUri = backupDirUri
        this.force = force
    }

    private constructor(p: Parcel) {
        action = p.readSerializable() as Action
        romId = p.readString()
        targets = p.createStringArray()
        backupName = p.readString()
        backupDirUri = p.readParcelable(Uri::class.java.classLoader)
        force = p.readInt() != 0
    }

    override fun describeContents(): Int {
        return 0
    }

    override fun writeToParcel(dest: Parcel, flags: Int) {
        dest.writeSerializable(action)
        dest.writeString(romId)
        dest.writeStringArray(targets)
        dest.writeString(backupName)
        dest.writeParcelable(backupDirUri, 0)
        dest.writeInt(if (force) 1 else 0)
    }

    companion object {
        @JvmField val CREATOR = object : Parcelable.Creator<BackupRestoreParams> {
            override fun createFromParcel(p: Parcel): BackupRestoreParams {
                return BackupRestoreParams(p)
            }

            override fun newArray(size: Int): Array<BackupRestoreParams?> {
                return arrayOfNulls(size)
            }
        }
    }
}
