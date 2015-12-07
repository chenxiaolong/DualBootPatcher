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
import java.util.concurrent.atomic.AtomicReference;

public abstract class BaseServiceTask implements Runnable {
    private final int mTaskId;
    private final WeakReference<Context> mContext;
    private AtomicReference<TaskState> mState = new AtomicReference<>(TaskState.NOT_STARTED);

    public enum TaskState {
        NOT_STARTED,
        RUNNING,
        FINISHED
    }

    public BaseServiceTask(int taskId, Context context) {
        mTaskId = taskId;
        mContext = new WeakReference<>(context);
    }

    public int getTaskId() {
        return mTaskId;
    }

    public TaskState getState() {
        return mState.get();
    }

    protected Context getContext() {
        // Guaranteed to be non-null until the task has been executed. After run() finishes, the
        // service can die and the context will go away.
        return mContext.get();
    }

    @Override
    public final void run() {
        if (!mState.compareAndSet(TaskState.NOT_STARTED, TaskState.RUNNING)) {
            throw new IllegalStateException("Task " + mTaskId + " has already been executed!");
        }

        onPreExecute();
        execute();
        onPostExecute();
        mState.set(TaskState.FINISHED);
    }

    protected abstract void execute();

    protected void onPreExecute() {
        // Do nothing by default
    }

    protected void onPostExecute() {
        // Do nothing by default
    }
}
