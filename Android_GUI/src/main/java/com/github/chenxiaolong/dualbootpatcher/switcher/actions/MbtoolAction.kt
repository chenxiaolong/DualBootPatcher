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

// This is not the ideal design, but composition is far easier than inheritance when dealing with
// Parcelables

import android.os.Parcel
import android.os.Parcelable

class MbtoolAction : Parcelable {
    var type: Type? = null
        private set
    private var _romInstallerParams: RomInstallerParams? = null
    private var _backupRestoreParams: BackupRestoreParams? = null

    val romInstallerParams: RomInstallerParams?
        get() {
            requireType(Type.ROM_INSTALLER)
            return _romInstallerParams
        }

    val backupRestoreParams: BackupRestoreParams?
        get() {
            requireType(Type.BACKUP_RESTORE)
            return _backupRestoreParams
        }

    enum class Type {
        ROM_INSTALLER,
        BACKUP_RESTORE
    }

    constructor(params: RomInstallerParams) {
        type = Type.ROM_INSTALLER
        _romInstallerParams = params
    }

    constructor(params: BackupRestoreParams) {
        type = Type.BACKUP_RESTORE
        _backupRestoreParams = params
    }

    private constructor(p: Parcel) {
        type = p.readSerializable() as Type
        when (type) {
            MbtoolAction.Type.ROM_INSTALLER ->
                _romInstallerParams = p.readParcelable(RomInstallerParams::class.java.classLoader)
            MbtoolAction.Type.BACKUP_RESTORE ->
                _backupRestoreParams = p.readParcelable(BackupRestoreParams::class.java.classLoader)
        }
    }

    override fun describeContents(): Int {
        return 0
    }

    override fun writeToParcel(dest: Parcel, flags: Int) {
        dest.writeSerializable(type)
        when (type) {
            MbtoolAction.Type.ROM_INSTALLER -> dest.writeParcelable(_romInstallerParams, 0)
            MbtoolAction.Type.BACKUP_RESTORE -> dest.writeParcelable(_backupRestoreParams, 0)
        }
    }

    private fun requireType(type: Type) {
        if (this.type != type) {
            throw IllegalStateException(
                    "Cannot call method due to incorrect type (${this.type} != $type)")
        }
    }

    companion object {
        @JvmField val CREATOR = object : Parcelable.Creator<MbtoolAction> {
            override fun createFromParcel(p: Parcel): MbtoolAction {
                return MbtoolAction(p)
            }

            override fun newArray(size: Int): Array<MbtoolAction?> {
                return arrayOfNulls(size)
            }
        }
    }
}