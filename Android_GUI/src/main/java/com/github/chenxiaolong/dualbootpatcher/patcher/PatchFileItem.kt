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

package com.github.chenxiaolong.dualbootpatcher.patcher

import android.net.Uri
import android.os.Parcel
import android.os.Parcelable

import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.Device

class PatchFileItem(
        // Patching information
        internal val patcherId: String,
        internal val inputUri: Uri,
        internal val outputUri: Uri,
        internal val displayName: String,
        internal var device: Device,
        internal var romId: String
) : Parcelable {
    internal var taskId: Int = 0
    // Progress information
    internal var state: PatchFileState? = null
    internal var details: String? = null
    internal var bytes: Long = 0
    internal var maxBytes: Long = 0
    internal var files: Long = 0
    internal var maxFiles: Long = 0
    // Completion information
    internal var successful: Boolean = false
    internal var errorCode: Int = 0

    private constructor(p: Parcel) : this(
            patcherId = p.readString()!!,
            inputUri = p.readParcelable(Uri::class.java.classLoader)!!,
            outputUri = p.readParcelable(Uri::class.java.classLoader)!!,
            displayName = p.readString()!!,
            device = p.readParcelable(Device::class.java.classLoader)!!,
            romId = p.readString()!!
    ) {
        details = p.readString()
        bytes = p.readLong()
        maxBytes = p.readLong()
        files = p.readLong()
        maxFiles = p.readLong()
        successful = p.readInt() != 0
        errorCode = p.readInt()
    }

    override fun describeContents() = 0

    override fun writeToParcel(dest: Parcel, flags: Int) {
        dest.writeString(patcherId)
        dest.writeParcelable(inputUri, 0)
        dest.writeParcelable(outputUri, 0)
        dest.writeString(displayName)
        dest.writeParcelable(device, 0)
        dest.writeString(romId)
        dest.writeString(details)
        dest.writeLong(bytes)
        dest.writeLong(maxBytes)
        dest.writeLong(files)
        dest.writeLong(maxFiles)
        dest.writeInt(if (successful) 1 else 0)
        dest.writeInt(errorCode)
    }

    companion object {
        @JvmField val CREATOR = object : Parcelable.Creator<PatchFileItem> {
            override fun createFromParcel(parcel: Parcel): PatchFileItem {
                return PatchFileItem(parcel)
            }

            override fun newArray(size: Int): Array<PatchFileItem?> {
                return arrayOfNulls(size)
            }
        }
    }
}