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

package com.github.chenxiaolong.dualbootpatcher;

import java.util.Arrays;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import android.os.Parcel;
import android.os.Parcelable;

public class PatcherInformation implements Parcelable {
    public Device[] mDevices;
    public PatchInfo[] mPatchInfos;
    public PartitionConfig[] mPartitionConfigs;
    public String[] mAutopatchers;
    public String[] mInits;
    public String[] mRamdisks;

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeParcelableArray(mDevices, 0);
        dest.writeParcelableArray(mPatchInfos, 0);
        dest.writeParcelableArray(mPartitionConfigs, 0);
        dest.writeStringArray(mAutopatchers);
        dest.writeStringArray(mInits);
        dest.writeStringArray(mRamdisks);
    }

    public static final Parcelable.Creator<PatcherInformation> CREATOR = new Parcelable.Creator<PatcherInformation>() {
        @Override
        public PatcherInformation createFromParcel(Parcel in) {
            return new PatcherInformation(in);
        }

        @Override
        public PatcherInformation[] newArray(int size) {
            return new PatcherInformation[size];
        }
    };

    private PatcherInformation(Parcel in) {
        Parcelable[] p = in.readParcelableArray(Device.class
                .getClassLoader());
        mDevices = Arrays.copyOf(p, p.length, Device[].class);
        p = in.readParcelableArray(PatchInfo.class.getClassLoader());
        mPatchInfos = Arrays.copyOf(p, p.length, PatchInfo[].class);
        p = in.readParcelableArray(PartitionConfig.class.getClassLoader());
        mPartitionConfigs = Arrays.copyOf(p, p.length,
                PartitionConfig[].class);
        mAutopatchers = in.createStringArray();
        mInits = in.createStringArray();
        mRamdisks = in.createStringArray();
    }

    public PatcherInformation() {
    }

    public void loadJson(String json) throws JSONException {
        JSONObject root = new JSONObject(json);

        JSONArray devices = root.getJSONArray("devices");
        mDevices = new Device[devices.length()];
        for (int i = 0; i < devices.length(); i++) {
            JSONObject deviceInfo = devices.getJSONObject(i);
            Device device = new Device();
            device.mCodeName = deviceInfo.getString("codename");
            device.mName = deviceInfo.getString("name");
            mDevices[i] = device;
        }

        JSONArray autopatchers = root.getJSONArray("autopatchers");
        mAutopatchers = new String[autopatchers.length()];
        for (int i = 0; i < autopatchers.length(); i++) {
            JSONObject autopatcher = autopatchers.getJSONObject(i);
            mAutopatchers[i] = autopatcher.getString("name");
        }

        JSONArray patchinfos = root.getJSONArray("patchinfos");
        mPatchInfos = new PatchInfo[patchinfos.length()];
        for (int i = 0; i < patchinfos.length(); i++) {
            JSONObject patchinfo = patchinfos.getJSONObject(i);
            PatchInfo info = new PatchInfo();
            info.mPath = patchinfo.getString("path");
            info.mName = patchinfo.getString("name");
            mPatchInfos[i] = info;
        }

        JSONArray partconfigs = root.getJSONArray("partitionconfigs");
        mPartitionConfigs = new PartitionConfig[partconfigs
                .length()];
        for (int i = 0; i < partconfigs.length(); i++) {
            JSONObject partconfig = partconfigs.getJSONObject(i);
            PartitionConfig config = new PartitionConfig();
            config.mId = partconfig.getString("id");
            config.mName = partconfig.getString("name");
            config.mDesc = partconfig.getString("description");
            mPartitionConfigs[i] = config;
        }

        JSONArray inits = root.getJSONArray("inits");
        mInits = new String[inits.length()];
        for (int i = 0; i < inits.length(); i++) {
            mInits[i] = inits.getString(i);
        }

        JSONArray ramdisks = root.getJSONArray("ramdisks");
        mRamdisks = new String[ramdisks.length()];
        for (int i = 0; i < ramdisks.length(); i++) {
            mRamdisks[i] = ramdisks.getString(i);
        }
    }

    public static class Device implements Parcelable {
        public String mCodeName;
        public String mName;

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeString(mCodeName);
            dest.writeString(mName);
        }

        public static final Parcelable.Creator<Device> CREATOR = new Parcelable.Creator<Device>() {
            @Override
            public Device createFromParcel(Parcel in) {
                return new Device(in);
            }

            @Override
            public Device[] newArray(int size) {
                return new Device[size];
            }
        };

        private Device(Parcel in) {
            mCodeName = in.readString();
            mName = in.readString();
        }

        public Device() {
        }
    }

    public static class PatchInfo implements Parcelable {
        public String mPath;
        public String mName;

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeString(mPath);
            dest.writeString(mName);
        }

        public static final Parcelable.Creator<PatchInfo> CREATOR = new Parcelable.Creator<PatchInfo>() {
            @Override
            public PatchInfo createFromParcel(Parcel in) {
                return new PatchInfo(in);
            }

            @Override
            public PatchInfo[] newArray(int size) {
                return new PatchInfo[size];
            }
        };

        private PatchInfo(Parcel in) {
            mPath = in.readString();
            mName = in.readString();
        }

        public PatchInfo() {
        }
    }

    public static class PartitionConfig implements Parcelable {
        public String mId;
        public String mName;
        public String mDesc;

        @Override
        public int describeContents() {
            return 0;
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            dest.writeString(mId);
            dest.writeString(mName);
            dest.writeString(mDesc);
        }

        public static final Parcelable.Creator<PartitionConfig> CREATOR = new Parcelable.Creator<PartitionConfig>() {
            @Override
            public PartitionConfig createFromParcel(Parcel in) {
                return new PartitionConfig(in);
            }

            @Override
            public PartitionConfig[] newArray(int size) {
                return new PartitionConfig[size];
            }
        };

        private PartitionConfig(Parcel in) {
            mId = in.readString();
            mName = in.readString();
            mDesc = in.readString();
        }

        public PartitionConfig() {
        }
    }
}
