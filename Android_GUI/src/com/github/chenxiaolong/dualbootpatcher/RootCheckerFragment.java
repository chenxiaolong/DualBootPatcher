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
import android.app.FragmentManager;
import android.os.AsyncTask;
import android.os.Bundle;

import java.util.ArrayList;

public class RootCheckerFragment extends Fragment {
    public static final String TAG = RootCheckerFragment.class.getName();

    private boolean mAttemptedRootRequest;
    private boolean mHaveRootAccess;

    private ArrayList<RootCheckerListener> mListeners = new ArrayList<RootCheckerListener>();

    public interface RootCheckerListener {
        public void rootRequestAcknowledged(boolean allowed);
    }

    public static RootCheckerFragment getInstance(FragmentManager fm) {
        RootCheckerFragment f = (RootCheckerFragment) fm.findFragmentByTag(TAG);

        if (f == null) {
            f = new RootCheckerFragment();
            fm.beginTransaction().add(f, TAG).commit();
        }

        return f;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setRetainInstance(true);
    }

    public synchronized void requestRoot() {
        mAttemptedRootRequest = false;
        new RootCheckerTask().execute();
    }

    public synchronized void attachListenerAndResendEvents(RootCheckerListener listener) {
        mListeners.add(listener);

        if (mAttemptedRootRequest) {
            for (RootCheckerListener rcl : mListeners) {
                rcl.rootRequestAcknowledged(mHaveRootAccess);
            }
        }
    }

    public synchronized void detachListener(RootCheckerListener listener) {
        mListeners.remove(listener);
    }

    private synchronized void onRootRequestAcknowledged(boolean allowed) {
        mHaveRootAccess = allowed;
        mAttemptedRootRequest = true;

        for (RootCheckerListener rcl : mListeners) {
            rcl.rootRequestAcknowledged(allowed);
        }
    }

    private class RootCheckerTask extends AsyncTask<Void, Void, Boolean> {
        @Override
        protected Boolean doInBackground(Void... args) {
            return CommandUtils.requestRootAccess();
        }

        @Override
        protected void onPostExecute(Boolean result) {
            synchronized (RootCheckerFragment.this) {
                onRootRequestAcknowledged(result);
            }
        }
    }
}
