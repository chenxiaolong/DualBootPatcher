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

package com.github.chenxiaolong.dualbootpatcher.switcher.actions;

import android.net.Uri;
import android.os.Parcel;
import android.os.Parcelable;

public class RomInstallerParams implements Parcelable {
    private Uri mUri;
    private String mDisplayName;
    private String mRomId;

    public RomInstallerParams() {
    }

    public RomInstallerParams(Uri uri, String displayName, String romId) {
        mUri = uri;
        mDisplayName = displayName;
        mRomId = romId;
    }

    protected RomInstallerParams(Parcel in) {
        mUri = in.readParcelable(Uri.class.getClassLoader());
        mRomId = in.readString();
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeParcelable(mUri, 0);
        dest.writeString(mRomId);
    }

    @SuppressWarnings("unused")
    public static final Parcelable.Creator<RomInstallerParams> CREATOR =
            new Parcelable.Creator<RomInstallerParams>() {
                @Override
                public RomInstallerParams createFromParcel(Parcel in) {
                    return new RomInstallerParams(in);
                }

                @Override
                public RomInstallerParams[] newArray(int size) {
                    return new RomInstallerParams[size];
                }
            };

    public Uri getUri() {
        return mUri;
    }

    public void setUri(Uri uri) {
        mUri = uri;
    }

    public String getDisplayName() {
        return mDisplayName;
    }

    public void setDisplayName(String displayName) {
        mDisplayName = displayName;
    }

    public String getRomId() {
        return mRomId;
    }

    public void setRomId(String romId) {
        mRomId = romId;
    }
}
