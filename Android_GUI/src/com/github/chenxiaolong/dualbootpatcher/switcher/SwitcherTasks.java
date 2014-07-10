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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.content.Context;
import android.os.AsyncTask;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.RomUtils;
import com.github.chenxiaolong.dualbootpatcher.RomUtils.RomInformation;
import com.squareup.otto.Bus;

public class SwitcherTasks {
    private static final Bus BUS = new Bus();

    // Otto bus

    public static Bus getBusInstance() {
        return BUS;
    }

    // Events

    public static class SwitcherTaskEvent {
    }

    public static class OnChoseRomEvent extends SwitcherTaskEvent {
        boolean failed;
        String message;
        String kernelId;
    }

    public static class OnSetKernelEvent extends SwitcherTaskEvent {
        boolean failed;
        String message;
        String kernelId;
    }

    public static class OnObtainedRomsEvent extends SwitcherTaskEvent {
        RomInformation[] roms;
        String[] names;
        String[] versions;
        int[] imageResIds;
    }

    // Task starters

    public static void chooseRom(String kernelId) {
        new ChooseRomTask(kernelId).execute();
    }

    public static void setKernel(String kernelId) {
        new SetKernelTask(kernelId).execute();
    }

    public static void obtainRoms(Context context) {
        new ObtainRomsTask(context).execute();
    }

    // Tasks

    private static class ChooseRomTask extends AsyncTask<Void, Void, Void> {
        private String mKernelId;

        private boolean mFailed;
        private String mMessage;

        public ChooseRomTask(String kernelId) {
            mKernelId = kernelId;
        }

        @Override
        protected Void doInBackground(Void... params) {
            mFailed = true;
            mMessage = "";

            try {
                SwitcherUtils.writeKernel(mKernelId);
                mFailed = false;
            } catch (Exception e) {
                mMessage = e.getMessage();
                e.printStackTrace();
            }

            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            OnChoseRomEvent event = new OnChoseRomEvent();
            event.failed = mFailed;
            event.message = mMessage;
            event.kernelId = mKernelId;
            getBusInstance().post(event);
        }
    }

    private static class SetKernelTask extends AsyncTask<Void, Void, Void> {
        private String mKernelId;

        private boolean mFailed;
        private String mMessage;

        public SetKernelTask(String kernelId) {
            mKernelId = kernelId;
        }

        @Override
        protected Void doInBackground(Void... params) {
            mFailed = true;
            mMessage = "";

            try {
                SwitcherUtils.backupKernel(mKernelId);
                mFailed = false;
            } catch (Exception e) {
                mMessage = e.getMessage();
                e.printStackTrace();
            }

            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            OnSetKernelEvent event = new OnSetKernelEvent();
            event.failed = mFailed;
            event.message = mMessage;
            event.kernelId = mKernelId;
            getBusInstance().post(event);
        }
    }

    private static class ObtainRomsTask extends AsyncTask<Void, Void, Void> {
        private final Context mContext;
        private RomInformation[] mRoms;
        private String[] mNames;
        private String[] mVersions;
        private int[] mImageResIds;

        public ObtainRomsTask(Context context) {
            mContext = context;
        }

        @Override
        protected Void doInBackground(Void... params) {
            mRoms = RomUtils.getRoms();
            mNames = new String[mRoms.length];
            mVersions = new String[mRoms.length];
            mImageResIds = new int[mRoms.length];

            for (int i = 0; i < mRoms.length; i++) {
                mNames[i] = RomUtils.getName(mContext, mRoms[i]);
                mVersions[i] = RomUtils.getVersion(mRoms[i]);
                if (mVersions[i] == null) {
                    mVersions[i] = mContext.getString(R.string.couldnt_determine_version);
                }
                mImageResIds[i] = RomUtils.getIconResource(mRoms[i]);
            }

            return null;
        }

        @Override
        protected void onPostExecute(Void result) {
            OnObtainedRomsEvent event = new OnObtainedRomsEvent();
            event.roms = mRoms;
            event.names = mNames;
            event.versions = mVersions;
            event.imageResIds = mImageResIds;
            getBusInstance().post(event);
        }
    }
}
