package com.github.chenxiaolong.dualbootpatcher;

import android.app.FragmentManager;
import android.os.AsyncTask;

public class RootCheckerEventCollector extends EventCollector {
    public static final String TAG = RootCheckerEventCollector.class.getSimpleName();

    private boolean sAttemptedRoot;

    public static RootCheckerEventCollector getInstance(FragmentManager fm) {
        RootCheckerEventCollector f = (RootCheckerEventCollector) fm.findFragmentByTag(TAG);

        if (f == null) {
            f = new RootCheckerEventCollector();
            fm.beginTransaction().add(f, TAG).commit();
        }

        return f;
    }

    public synchronized void requestRoot() {
        if (!sAttemptedRoot) {
            new RootCheckerTask().execute();
            sAttemptedRoot = true;
        }
    }

    public synchronized void resetAttempt() {
        sAttemptedRoot = false;
    }

    public synchronized boolean isAttemptedRoot() {
        return sAttemptedRoot;
    }

    // Events

    public class RootAcknowledgedEvent extends BaseEvent {
        public boolean allowed;

        public RootAcknowledgedEvent(boolean allowed) {
            setKeepInQueue(true);

            this.allowed = allowed;
        }
    }

    private class RootCheckerTask extends AsyncTask<Void, Void, Boolean> {
        @Override
        protected Boolean doInBackground(Void... args) {
            return CommandUtils.requestRootAccess();
        }

        @Override
        protected void onPostExecute(Boolean result) {
            sendEvent(new RootAcknowledgedEvent(result));
        }
    }
}
