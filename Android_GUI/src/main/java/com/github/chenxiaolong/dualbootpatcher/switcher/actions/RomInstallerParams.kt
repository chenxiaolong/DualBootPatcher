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

class RomInstallerParams : Parcelable {
    var uri: Uri? = null
    var displayName: String? = null
    var romId: String? = null
    var skipMounts = false
    var allowOverwrite = false

    constructor()

    constructor(uri: Uri, displayName: String, romId: String) {
        this.uri = uri
        this.displayName = displayName
        this.romId = romId
    }

    private constructor(p: Parcel) {
        uri = p.readParcelable(Uri::class.java.classLoader)
        romId = p.readString()
    }

    override fun describeContents(): Int {
        return 0
    }

    override fun writeToParcel(dest: Parcel, flags: Int) {
        dest.writeParcelable(uri, 0)
        dest.writeString(romId)
    }

    companion object {
        @JvmField val CREATOR = object : Parcelable.Creator<RomInstallerParams> {
            override fun createFromParcel(p: Parcel): RomInstallerParams {
                return RomInstallerParams(p)
            }

            override fun newArray(size: Int): Array<RomInstallerParams?> {
                return arrayOfNulls(size)
            }
        }
    }
}
