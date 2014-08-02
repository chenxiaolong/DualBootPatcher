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

package com.github.chenxiaolong.dualbootpatcher.settings;

import android.content.Context;
import android.os.AsyncTask;

import com.squareup.otto.Bus;

public class AppSharingTasks {
    private static final Bus BUS = new Bus();

    // Otto bus

    public static Bus getBusInstance() {
        return BUS;
    }

    // Events

    public static class OnUpdatedRamdiskEvent {
        boolean failed;
    }

    // Task starters

    public static void updateRamdisk(Context context) {
        new UpdateRamdiskTask(context).execute();
    }

    // Tasks

    private static class UpdateRamdiskTask extends AsyncTask<Void, Void, Boolean> {
        private Context mContext;

        public UpdateRamdiskTask(Context context) {
            mContext = context;
        }

        @Override
        protected Boolean doInBackground(Void... params) {
            try {
                AppSharingUtils.updateRamdisk(mContext);
                return true;
            } catch (Exception e) {
                e.printStackTrace();
                return false;
            }
        }

        @Override
        protected void onPostExecute(Boolean result) {
            OnUpdatedRamdiskEvent e = new OnUpdatedRamdiskEvent();
            e.failed = !result;
            getBusInstance().post(e);
        }
    }
}
