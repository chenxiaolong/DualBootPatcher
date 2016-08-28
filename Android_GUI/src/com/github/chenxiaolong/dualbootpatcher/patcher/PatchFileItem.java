/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.patcher;

import android.net.Uri;
import android.os.Parcel;
import android.os.Parcelable;

import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbDevice.Device;

public class PatchFileItem implements Parcelable {
    int taskId;
    // Patching information
    String patcherId;
    Uri inputUri;
    Uri outputUri;
    String displayName;
    Device device;
    String romId;
    // Progress information
    PatchFileState state;
    String details;
    long bytes;
    long maxBytes;
    long files;
    long maxFiles;
    // Completion information
    boolean successful;
    int errorCode;

    public PatchFileItem() {
    }

    protected PatchFileItem(Parcel in) {
        patcherId = in.readString();
        inputUri = in.readParcelable(Uri.class.getClassLoader());
        outputUri = in.readParcelable(Uri.class.getClassLoader());
        displayName = in.readString();
        device = in.readParcelable(Device.class.getClassLoader());
        romId = in.readString();
        details = in.readString();
        bytes = in.readLong();
        maxBytes = in.readLong();
        files = in.readLong();
        maxFiles = in.readLong();
        successful = in.readInt() != 0;
        errorCode = in.readInt();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeString(patcherId);
        dest.writeParcelable(inputUri, 0);
        dest.writeParcelable(outputUri, 0);
        dest.writeString(displayName);
        dest.writeParcelable(device, 0);
        dest.writeString(romId);
        dest.writeString(details);
        dest.writeLong(bytes);
        dest.writeLong(maxBytes);
        dest.writeLong(files);
        dest.writeLong(maxFiles);
        dest.writeInt(successful ? 1 : 0);
        dest.writeInt(errorCode);
    }

    @SuppressWarnings("unused")
    public static final Parcelable.Creator<PatchFileItem> CREATOR =
            new Parcelable.Creator<PatchFileItem>() {
                @Override
                public PatchFileItem createFromParcel(Parcel in) {
                    return new PatchFileItem(in);
                }

                @Override
                public PatchFileItem[] newArray(int size) {
                    return new PatchFileItem[size];
                }
            };
}
