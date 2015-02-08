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

package com.github.chenxiaolong.multibootpatcher.events;

import android.app.FragmentManager;
import android.os.AsyncTask;

import com.github.chenxiaolong.dualbootpatcher.EventCollector;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolSocket;

public class MbtoolConnectionEventCollector extends EventCollector {
    public static final String TAG = MbtoolConnectionEventCollector.class.getSimpleName();

    private MbtoolConnectionEvent mEvent;

    public static MbtoolConnectionEventCollector getInstance(FragmentManager fm) {
        MbtoolConnectionEventCollector f =
                (MbtoolConnectionEventCollector) fm.findFragmentByTag(TAG);

        if (f == null) {
            f = new MbtoolConnectionEventCollector();
            fm.beginTransaction().add(f, TAG).commit();
        }

        return f;
    }

    public synchronized void connect() {
        if (mEvent == null) {
            new MbtoolConnectionTask().execute();
        } else {
            sendEvent();
        }
    }

    public synchronized void resetAttempt() {
        mEvent = null;
    }

    public synchronized boolean isAttemptedConnect() {
        return mEvent != null;
    }

    private synchronized void sendEvent() {
        sendEventIfNotQueued(mEvent);
    }

    // Events

    public class MbtoolConnectionEvent extends BaseEvent {
        public boolean connected;

        public MbtoolConnectionEvent(boolean connected) {
            setKeepInQueue(true);

            this.connected = connected;
        }
    }

    private class MbtoolConnectionTask extends AsyncTask<Void, Void, Boolean> {
        @Override
        protected Boolean doInBackground(Void... args) {
            return MbtoolSocket.getInstance().connect();
        }

        @Override
        protected void onPostExecute(Boolean result) {
            mEvent = new MbtoolConnectionEvent(result);
            removeAllEvents(MbtoolConnectionEvent.class);
            sendEvent();
        }
    }
}
