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

package com.github.chenxiaolong.dualbootpatcher;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.os.SystemClock;
import android.util.Log;

import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.ThreadPoolExecutor;
import java.util.concurrent.TimeUnit;

public class ThreadPoolService extends Service {
    private static final boolean DEBUG = true;
    private final String TAG = getClass().getSimpleName();

    /** Exit action for intent */
    private final String ACTION_EXIT = getClass().getCanonicalName() + ".exit";
    /** Number of milliseconds for delayed exit */
    private final long EXIT_DELAY = 10 * 1000;
    /** Pending intent to exit the service */
    private PendingIntent mExitPendingIntent;
    /** Alarm manager */
    private AlarmManager mAlarmManager;

    /** Number of cores */
    private static final int NUMBER_OF_CORES = Runtime.getRuntime().availableProcessors();
    /** Time an idle thread will wait before it exits */
    private static final int KEEP_ALIVE_TIME = 1;
    /** Units of {@link #KEEP_ALIVE_TIME} */
    private static final TimeUnit KEEP_ALIVE_TIME_UNITS = TimeUnit.SECONDS;

    /** Thread pool for executing operations */
    private HashMap<String, ThreadPoolExecutor> mThreadPools = new HashMap<>();

    /** Number of currently running or pending operations */
    private int mOperations = 0;

    private final Object mLock = new Object();

    /** Whether there are bound clients */
    private boolean mBound = false;

    /** Binder instance */
    private final IBinder mBinder = new ThreadPoolServiceBinder();

    /** Local Binder class */
    public class ThreadPoolServiceBinder extends Binder {
        public ThreadPoolService getService() {
            return ThreadPoolService.this;
        }
    }

    /** Log debug messages if {@link #DEBUG} is true */
    private void log(String message) {
        if (DEBUG) {
            Log.d(TAG, message);
        }
    }

    /** {@inheritDoc} */
    @Override
    public void onCreate() {
        super.onCreate();
        log("onCreate()");

        mAlarmManager = (AlarmManager) getSystemService(Context.ALARM_SERVICE);
        Intent exitIntent = new Intent(this, getClass());
        exitIntent.setAction(ACTION_EXIT);
        mExitPendingIntent = PendingIntent.getService(this, 0, exitIntent, 0);

        scheduleDelayedExit();
    }

    /** {@inheritDoc} */
    @Override
    public void onDestroy() {
        super.onDestroy();
        log("onDestroy()");

        // Attempt to stop thread pools. This shouldn't be an issue as there should be no tasks
        // running at this point. The service stops when all clients have unbound from it and there
        // are no pending tasks.
        for (Map.Entry<String, ThreadPoolExecutor> entry : mThreadPools.entrySet()) {
            String id = entry.getKey();
            ThreadPoolExecutor executor = entry.getValue();
            executor.shutdownNow();
            try {
                executor.awaitTermination(60, TimeUnit.SECONDS);
            } catch (InterruptedException e) {
                Log.e(TAG, "Failed to wait 60 seconds for thread pool termination: " + id, e);
            }
        }
    }

    /** {@inheritDoc} */
    @Override
    public int onStartCommand(final Intent intent, final int flags, final int startId) {
        log("onStartCommand(intent=" + intent + ", flags=" + flags + ", startId=" + startId + ")");

        if (intent != null && ACTION_EXIT.equals(intent.getAction())) {
            attemptToStop();
            return START_NOT_STICKY;
        }

        return START_STICKY;
    }

    /** {@inheritDoc} */
    @Override
    public IBinder onBind(final Intent intent) {
        log("onBind(intent=" + intent + ")");
        synchronized (mLock) {
            mBound = true;
        }
        return mBinder;
    }

    /** {@inheritDoc} */
    @Override
    public boolean onUnbind(final Intent intent) {
        log("onUnbind(intent=" + intent + ")");
        synchronized (mLock) {
            mBound = false;
        }
        attemptToStop();
        // We're don't need to differentiate between bind and rebind
        return false;
    }

    /**
     * Add a new thread pool with the maximum number of threads equal to the number of CPU threads
     *
     * @param id ID for the thread pool
     * @throws IllegalArgumentException if maxThreads <= 0 or if the ID already exists
     */
    protected void addThreadPool(String id) {
        addThreadPool(id, NUMBER_OF_CORES);
    }

    /**
     * Add a new thread pool
     *
     * @param id ID for the thread pool
     * @param maxThreads Maximum threads in the thread pool
     * @throws IllegalArgumentException if maxThreads <= 0 or if the ID already exists
     */
    protected void addThreadPool(String id, int maxThreads) {
        if (mThreadPools.containsKey(id)) {
            throw new IllegalStateException("Thread pool already exists: " + id);
        }

        ThreadPoolExecutor executor = new ThreadPoolExecutor(maxThreads, maxThreads,
                KEEP_ALIVE_TIME, KEEP_ALIVE_TIME_UNITS, new LinkedBlockingQueue<Runnable>());
        mThreadPools.put(id, executor);
    }

    /**
     * Enqueue operation
     *
     * @param id Thread pool ID
     * @param runnable Task to run in thread pool
     * @throws IllegalArgumentException if the thread pool ID doesn't exist
     */
    protected void enqueueOperation(String id, final Runnable runnable) {
        ThreadPoolExecutor executor = mThreadPools.get(id);
        if (executor == null) {
            throw new IllegalArgumentException("Thread pool does not exist: " + id);
        }

        cancelDelayedExit();

        synchronized (mLock) {
            mOperations++;
            executor.execute(new Runnable() {
                @Override
                public void run() {
                    try {
                        runnable.run();
                    } finally {
                        synchronized (mLock) {
                            mOperations--;
                            attemptToStop();
                        }
                    }
                }
            });
        }
    }

    /**
     * Cancel operation
     *
     * @param id Thread pool ID
     * @param runnable Task to cancel
     * @throws IllegalArgumentException if the thread pool ID doesn't exist
     */
    protected boolean cancelOperation(String id, Runnable runnable) {
        ThreadPoolExecutor executor = mThreadPools.get(id);
        if (executor == null) {
            throw new IllegalArgumentException("Thread pool does not exist: " + id);
        }

        boolean ret = executor.remove(runnable);
        synchronized (mLock) {
            if (ret) {
                mOperations--;
                attemptToStop();
            }
        }
        return ret;
    }

    /**
     * Attempt to stop the service
     *
     * This will not call {@link #stopSelf()} unless there are no tasks running and no clients bound
     */
    private void attemptToStop() {
        synchronized (mLock) {
            log("Attempting to stop service");
            if (mOperations > 0 || mBound) {
                log("Not stopping: # of operations: " + mOperations + ", is bound: " + mBound);
                return;
            }

            log("Calling stopSelf(): there are no more operations");
            stopSelf();
        }
    }

    /**
     * Schedule delayed exit.
     */
    private void scheduleDelayedExit() {
        log("Scheduling delayed exit after " + EXIT_DELAY + "ms");
        mAlarmManager.set(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                SystemClock.elapsedRealtime() + EXIT_DELAY, mExitPendingIntent);
    }

    /**
     * Cancel delayed exit
     */
    private void cancelDelayedExit() {
        log("Cancelling delayed exit");
        mAlarmManager.cancel(mExitPendingIntent);
    }
}