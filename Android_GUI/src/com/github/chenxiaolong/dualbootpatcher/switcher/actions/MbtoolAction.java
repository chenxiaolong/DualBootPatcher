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

// This is not the ideal design, but composition is far easier than inheritance when dealing with
// Parcelables

import android.os.Parcel;
import android.os.Parcelable;

public class MbtoolAction implements Parcelable {
    public enum Type {
        ROM_INSTALLER,
        BACKUP_RESTORE,
    }

    private Type mType;
    private RomInstallerParams mRomInstallerParams;
    private BackupRestoreParams mBackupRestoreParams;

    public MbtoolAction(RomInstallerParams params) {
        mType = Type.ROM_INSTALLER;
        mRomInstallerParams = params;
    }

    public MbtoolAction(BackupRestoreParams params) {
        mType = Type.BACKUP_RESTORE;
        mBackupRestoreParams = params;
    }

    protected MbtoolAction(Parcel in) {
        mType = (Type) in.readSerializable();
        switch (mType) {
        case ROM_INSTALLER:
            mRomInstallerParams = in.readParcelable(RomInstallerParams.class.getClassLoader());
            break;
        case BACKUP_RESTORE:
            mBackupRestoreParams = in.readParcelable(BackupRestoreParams.class.getClassLoader());
            break;
        default:
            throw new IllegalStateException("Invalid type: " + mType);
        }
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeSerializable(mType);
        switch (mType) {
        case ROM_INSTALLER:
            dest.writeParcelable(mRomInstallerParams, 0);
            break;
        case BACKUP_RESTORE:
            dest.writeParcelable(mBackupRestoreParams, 0);
            break;
        }
    }

    @SuppressWarnings("unused")
    public static final Parcelable.Creator<MbtoolAction> CREATOR =
            new Parcelable.Creator<MbtoolAction>() {
                @Override
                public MbtoolAction createFromParcel(Parcel in) {
                    return new MbtoolAction(in);
                }

                @Override
                public MbtoolAction[] newArray(int size) {
                    return new MbtoolAction[size];
                }
            };

    public Type getType() {
        return mType;
    }

    private void requireType(Type type) {
        if (mType != type) {
            throw new IllegalStateException(
                    "Cannot call method due to incorrect type (" + mType + " != " + type + ")");
        }
    }

    public RomInstallerParams getRomInstallerParams() {
        requireType(Type.ROM_INSTALLER);
        return mRomInstallerParams;
    }

    public BackupRestoreParams getBackupRestoreParams() {
        requireType(Type.BACKUP_RESTORE);
        return mBackupRestoreParams;
    }
}
