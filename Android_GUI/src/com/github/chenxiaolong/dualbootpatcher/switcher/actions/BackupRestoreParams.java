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

import android.os.Parcel;
import android.os.Parcelable;

public class BackupRestoreParams implements Parcelable {
    public enum Action {
        BACKUP,
        RESTORE,
    }

    private Action mAction;
    private String mRomId;
    private String[] mTargets;
    private String mBackupName;
    private String mBackupDir;
    private boolean mForce;

    public BackupRestoreParams() {
    }

    public BackupRestoreParams(Action action, String romId, String[] targets, String backupName,
                               String backupDir, boolean force) {
        mAction = action;
        mRomId = romId;
        mTargets = targets;
        mBackupName = backupName;
        mBackupDir = backupDir;
        mForce = force;
    }

    protected BackupRestoreParams(Parcel in) {
        mAction = (Action) in.readSerializable();
        mRomId = in.readString();
        mTargets = in.createStringArray();
        mBackupName = in.readString();
        mBackupDir = in.readString();
        mForce = in.readInt() != 0;
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeSerializable(mAction);
        dest.writeString(mRomId);
        dest.writeStringArray(mTargets);
        dest.writeString(mBackupName);
        dest.writeString(mBackupDir);
        dest.writeInt(mForce ? 1 : 0);
    }

    @SuppressWarnings("unused")
    public static final Parcelable.Creator<BackupRestoreParams> CREATOR =
            new Parcelable.Creator<BackupRestoreParams>() {
                @Override
                public BackupRestoreParams createFromParcel(Parcel in) {
                    return new BackupRestoreParams(in);
                }

                @Override
                public BackupRestoreParams[] newArray(int size) {
                    return new BackupRestoreParams[size];
                }
            };

    public Action getAction() {
        return mAction;
    }

    public void setAction(Action action) {
        mAction = action;
    }

    public String getRomId() {
        return mRomId;
    }

    public void setRomId(String romId) {
        mRomId = romId;
    }

    public String[] getTargets() {
        return mTargets;
    }

    public void setTargets(String[] targets) {
        mTargets = targets;
    }

    public String getBackupName() {
        return mBackupName;
    }

    public void setBackupName(String backupName) {
        mBackupName = backupName;
    }

    public String getBackupDir() {
        return mBackupDir;
    }

    public void setBackupDir(String backupDir) {
        mBackupDir = backupDir;
    }

    public boolean getForce() {
        return mForce;
    }

    public void setForce(boolean force) {
        mForce = force;
    }
}
