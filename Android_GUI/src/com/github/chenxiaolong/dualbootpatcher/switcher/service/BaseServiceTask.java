/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.switcher.service;

import android.content.Context;

import java.lang.ref.WeakReference;
import java.util.HashSet;
import java.util.Iterator;
import java.util.concurrent.atomic.AtomicBoolean;

public abstract class BaseServiceTask implements Runnable {
    private final int mTaskId;
    private final WeakReference<Context> mContext;
    private final HashSet<BaseServiceTaskListener> mListeners = new HashSet<>();
    private final AtomicBoolean mExecuted = new AtomicBoolean(false);

    public interface BaseServiceTaskListener {
    }

    public BaseServiceTask(int taskId, Context context) {
        mTaskId = taskId;
        mContext = new WeakReference<>(context);
    }

    public int getTaskId() {
        return mTaskId;
    }

    protected Context getContext() {
        // Guaranteed to be non-null until the task has been executed. After run() finishes, the
        // service can die and the context will go away.
        return mContext.get();
    }

    /**
     * Add listener for this task's events.
     *
     * If there are cached events (such as an event with the current progress), they are guaranteed
     * to be resent before any further events to ensure chronological ordering.
     *
     * No cache events will be resent if the listener has already been added.
     *
     * @param listener Listener to add
     * @return Whether the listener was successfully added
     */
    public boolean addListener(BaseServiceTaskListener listener) {
        synchronized (mListeners) {
            if (mListeners.add(listener)) {
                onListenerAdded(listener);
                return true;
            }
            return false;
        }
    }

    /**
     * Remove listener for this task's events.
     *
     * After this method returns, it is guaranteed that no further events will be delivered to the
     * listener, regardless of which thread is making the call.
     *
     * @param listener Listener to remove
     * @return Whether the listener was successfully removed
     */
    public boolean removeListener(BaseServiceTaskListener listener) {
        synchronized (mListeners) {
            if (mListeners.remove(listener)) {
                onListenerRemoved(listener);
                return true;
            }
            return false;
        }
    }

    /**
     * Remove all listeners for this task's events
     *
     * After this method returns, it is guaranteed that no futher events will be delivered to any
     * previously added listener, regardless of which thread is making the call.
     */
    public void removeAllListeners() {
        synchronized (mListeners) {
            Iterator<BaseServiceTaskListener> iter = mListeners.iterator();
            while (iter.hasNext()) {
                onListenerRemoved(iter.next());
                iter.remove();
            }
        }
    }

    protected interface CallbackRunnable {
        void call(BaseServiceTaskListener listener);
    }

    protected void forEachListener(CallbackRunnable runnable) {
        synchronized (mListeners) {
            for (BaseServiceTaskListener listener : mListeners) {
                runnable.call(listener);
            }
        }
    }

    @Override
    public final void run() {
        if (!mExecuted.compareAndSet(false, true)) {
            throw new IllegalStateException("Task " + mTaskId + " has already been executed!");
        }

        execute();
    }

    protected void onListenerAdded(BaseServiceTaskListener listener) {
        // Override to send events upon adding a listener
    }

    protected void onListenerRemoved(BaseServiceTaskListener listener) {
        // Override to send events upon removing a listener
    }

    protected abstract void execute();
}
