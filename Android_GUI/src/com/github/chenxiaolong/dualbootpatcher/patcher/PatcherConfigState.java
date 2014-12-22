/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.PartConfig;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.PatchInfo;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.Patcher;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

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
        out.writeInt(mSupported);
        writeStringHashMap(out, mReversePatcherMap);
        out.writeParcelable(mPatcher, 0);
        out.writeParcelable(mDevice, 0);
        writeParcelableArrayList(out, mPartConfigs);
        out.writeParcelable(mPartConfig, 0);
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
        mSupported = in.readInt();
        mReversePatcherMap = readStringHashMap(in);
        mPatcher = in.readParcelable(Patcher.class.getClassLoader());
        mDevice = in.readParcelable(Device.class.getClassLoader());
        mPartConfigs = readParcelableArrayList(in, PartConfig.class);
        mPartConfig = in.readParcelable(PartConfig.class.getClassLoader());
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

    private <T extends Parcelable> void writeParcelableArrayList(Parcel out, ArrayList<T> array) {
        out.writeInt(array.size());
        for (T item : array) {
            out.writeParcelable(item, 0);
        }
    }

    private <T extends Parcelable> ArrayList<T> readParcelableArrayList(Parcel in, Class<T> clazz) {
        int size = in.readInt();
        ArrayList<T> array = new ArrayList<>(size);
        for (int i = 0; i < size; i++) {
            array.add((T) in.readParcelable(clazz.getClassLoader()));
        }
        return array;
    }

    private void writeStringHashMap(Parcel out, HashMap<String, String> map) {
        out.writeInt(map.size());
        for (Map.Entry<String, String> entry : map.entrySet()) {
            out.writeString(entry.getKey());
            out.writeString(entry.getValue());
        }
    }

    private HashMap<String, String> readStringHashMap(Parcel in) {
        int size = in.readInt();
        HashMap<String, String> map = new HashMap<>();
        for (int i = 0; i < size; i++) {
            String key = in.readString();
            String value = in.readString();
            map.put(key, value);
        }
        return map;
    }


    public PatcherConfigState() {
    }

    public void setupInitial() {
        if (!mInitialized) {
            //mPatcher = PatcherUtils.sPC.createPatcher(DEFAULT_PATCHER);
            //refreshPartConfigs();

            for (String patcherId : PatcherUtils.sPC.getPatchers()) {
                String patcherName = PatcherUtils.sPC.getPatcherName(patcherId);
                mReversePatcherMap.put(patcherName, patcherId);
            }

            mDevice = PatcherUtils.sPC.getDevices()[0];

            mPatchInfos = PatcherUtils.sPC.getPatchInfos(mDevice);

            mInitialized = true;
        }
    }

    public void refreshPartConfigs() {
        mPartConfigs.clear();

        PartConfig[] configs = PatcherUtils.sPC.getPartitionConfigs();

        for (String id : mPatcher.getSupportedPartConfigIds()) {
            PartConfig config = null;

            for (PartConfig c : configs) {
                if (c.getId().equals(id)) {
                    config = c;
                    break;
                }
            }

            if (config != null) {
                mPartConfigs.add(config);
            }
        }
    }

    private static final String DEFAULT_PATCHER = "MultiBootPatcher";

    public static final int NOT_SUPPORTED = 0x0;
    public static final int SUPPORTED_FILE = 0x1;
    public static final int SUPPORTED_PARTCONFIG = 0x2;

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
    public int mSupported;

    // Maps patcher name to patcher ID
    public HashMap<String, String> mReversePatcherMap = new HashMap<>();
    // Selected patcher
    public Patcher mPatcher;

    // Selected device
    public Device mDevice;

    // List of partconfigs for selected patcher
    public ArrayList<PartConfig> mPartConfigs = new ArrayList<>();
    // Selected partconfig
    public PartConfig mPartConfig;

    // List of patchinfos for selected device
    public PatchInfo[] mPatchInfos;
    // Selected patchinfo
    public PatchInfo mPatchInfo;

    public String mPatcherNewFile;
    public boolean mPatcherFailed;
    public String mPatcherError;
}
