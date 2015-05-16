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

package com.github.chenxiaolong.dualbootpatcher.patcher;

import android.os.Parcel;
import android.os.Parcelable;

import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.PatchInfo;
import com.github.chenxiaolong.dualbootpatcher.nativelib.LibMbp.Patcher;

public class PatcherConfigState implements Parcelable {
    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel out, int flags) {
        out.writeInt(mInitialized ? 1 : 0);
        out.writeInt(mState);
        out.writeString(mFilename);
        out.writeInt(mSupported ? 1 : 0);
        out.writeParcelable(mPatcher, 0);
        out.writeParcelable(mDevice, 0);
        out.writeParcelableArray(mPatchInfos, 0);
        out.writeParcelable(mPatchInfo, 0);
        out.writeString(mPatcherNewFile);
        out.writeInt(mPatcherFailed ? 1 : 0);
        out.writeString(mPatcherError);
    }

    private PatcherConfigState(Parcel in) {
        mInitialized = in.readInt() != 0;
        mState = in.readInt();
        mFilename = in.readString();
        mSupported = in.readInt() != 0;
        mPatcher = in.readParcelable(Patcher.class.getClassLoader());
        mDevice = in.readParcelable(Device.class.getClassLoader());
        mPatchInfos = (PatchInfo[]) in.readParcelableArray(PatchInfo.class.getClassLoader());
        mPatchInfo = in.readParcelable(PatchInfo.class.getClassLoader());
        mPatcherNewFile = in.readString();
        mPatcherFailed = in.readInt() != 0;
        mPatcherError = in.readString();
    }

    public static final Parcelable.Creator<PatcherConfigState> CREATOR
            = new Parcelable.Creator<PatcherConfigState>() {
        public PatcherConfigState createFromParcel(Parcel in) {
            return new PatcherConfigState(in);
        }

        public PatcherConfigState[] newArray(int size) {
            return new PatcherConfigState[size];
        }
    };

    public PatcherConfigState() {
    }

    public void setupInitial() {
        if (!mInitialized) {
            mPatcher = PatcherUtils.sPC.createPatcher(DEFAULT_PATCHER);

            mDevice = PatcherUtils.sPC.getDevices()[0];

            mPatchInfos = PatcherUtils.sPC.getPatchInfos(mDevice);

            mInitialized = true;
        }
    }

    private static final String DEFAULT_PATCHER = "MultiBootPatcher";

    public static final int STATE_INITIAL = 0;
    public static final int STATE_CHOSE_FILE = 1;
    public static final int STATE_PATCHING = 2;
    public static final int STATE_FINISHED = 3;

    private boolean mInitialized = false;

    // Patcher state
    public int mState;

    // Selected file
    public String mFilename;

    // Level of support for the selected file
    public boolean mSupported;

    // Selected patcher
    public Patcher mPatcher;

    // Selected device
    public Device mDevice;

    // Selected ROM ID
    public String mRomId;

    // List of patchinfos for selected device
    public PatchInfo[] mPatchInfos;
    // Selected patchinfo
    public PatchInfo mPatchInfo;

    public String mPatcherNewFile;
    public boolean mPatcherFailed;
    public String mPatcherError;
}
