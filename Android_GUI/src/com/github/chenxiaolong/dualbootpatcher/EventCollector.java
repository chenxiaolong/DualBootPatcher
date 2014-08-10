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

import android.app.Fragment;
import android.os.Bundle;

import java.util.ArrayList;
import java.util.Iterator;

public class EventCollector extends Fragment {
    private EventCollectorListener mListener;
    private ArrayList<BaseEvent> mEventQueue = new ArrayList<BaseEvent>();

    public interface EventCollectorListener {
        public void onEventReceived(BaseEvent event);
    }

    public static class BaseEvent {
        // If true, the event will always remain in the queue and will be resent whenever the
        // listener is attached.
        private boolean mKeepInQueue;

        public boolean getKeepInQueue() {
            return mKeepInQueue;
        }

        public void setKeepInQueue(boolean keepInQueue) {
            mKeepInQueue = keepInQueue;
        }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setRetainInstance(true);
    }

    public synchronized void attachListener(EventCollectorListener listener) {
        mListener = listener;

        Iterator<BaseEvent> iter = mEventQueue.iterator();
        while (iter.hasNext()) {
            BaseEvent event = iter.next();
            mListener.onEventReceived(event);

            if (!event.getKeepInQueue()) {
                iter.remove();
            }
        }
    }

    public synchronized void detachListener() {
        mListener = null;
    }

    protected synchronized void sendEvent(BaseEvent event) {
        if (mListener != null) {
            mListener.onEventReceived(event);
        } else {
            mEventQueue.add(event);
        }
    }
}
