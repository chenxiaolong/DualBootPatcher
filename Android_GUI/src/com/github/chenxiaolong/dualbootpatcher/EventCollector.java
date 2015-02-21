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

package com.github.chenxiaolong.dualbootpatcher;

import android.app.Fragment;
import android.os.Bundle;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

public class EventCollector extends Fragment {
    private HashMap<String, ListenerAndQueue> mEventQueues = new HashMap<>();

    private class ListenerAndQueue {
        EventCollectorListener listener;
        ArrayList<BaseEvent> queue = new ArrayList<>();
    }

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

    // Create a null listener to catch events before attachListener() is called
    public synchronized void createListener(String tag) {
        attachListener(tag, null);
    }

    public synchronized void attachListener(String tag, EventCollectorListener listener) {
        ListenerAndQueue lq;

        if (mEventQueues.containsKey(tag)) {
            lq = mEventQueues.get(tag);
        } else {
            lq = new ListenerAndQueue();
        }

        lq.listener = listener;

        if (lq.listener != null) {
            Iterator<BaseEvent> iter = lq.queue.iterator();
            while (iter.hasNext()) {
                BaseEvent event = iter.next();
                lq.listener.onEventReceived(event);

                if (!event.getKeepInQueue()) {
                    iter.remove();
                }
            }
        }

        mEventQueues.put(tag, lq);
    }

    public synchronized void detachListener(String tag) {
        // If this fails, something really bad happened anyway
        if (mEventQueues.containsKey(tag)) {
            mEventQueues.get(tag).listener = null;
        }
    }

    protected synchronized void sendEvent(BaseEvent event) {
        for (Map.Entry<String, ListenerAndQueue> entry : mEventQueues.entrySet()) {
            ListenerAndQueue lq = entry.getValue();

            if (lq.listener != null) {
                lq.listener.onEventReceived(event);

                if (event.getKeepInQueue()) {
                    lq.queue.add(event);
                }
            } else {
                lq.queue.add(event);
            }
        }
    }

    protected synchronized void sendEventIfNotQueued(BaseEvent event) {
        for (Map.Entry<String, ListenerAndQueue> entry : mEventQueues.entrySet()) {
            ListenerAndQueue lq = entry.getValue();

            boolean skip = false;

            for (BaseEvent e : lq.queue) {
                if (e == event) {
                    skip = true;
                    break;
                }
            }

            if (skip) {
                continue;
            }

            if (lq.listener != null) {
                lq.listener.onEventReceived(event);

                if (event.getKeepInQueue()) {
                    lq.queue.add(event);
                }
            } else {
                lq.queue.add(event);
            }
        }
    }

    // Remove all events of a certain type
    protected synchronized void removeAllEvents(Class type) {
        for (Map.Entry<String, ListenerAndQueue> entry : mEventQueues.entrySet()) {
            ListenerAndQueue lq = entry.getValue();

            Iterator<BaseEvent> iter = lq.queue.iterator();
            while (iter.hasNext()) {
                BaseEvent e = iter.next();

                if (type.isInstance(e)) {
                    iter.remove();
                }
            }
        }
    }
}
