package com.github.chenxiaolong.dualbootpatcher.patcher;

import android.app.IntentService;
import android.content.Intent;
import android.os.Bundle;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandListener;
import com.github.chenxiaolong.dualbootpatcher.CommandUtils.CommandResult;
import com.github.chenxiaolong.dualbootpatcher.PatcherInformation;

public class PatcherService extends IntentService {
    private static final String TAG = "PatcherService";
    public static final String BROADCAST_INTENT = "com.chenxiaolong.github.dualbootpatcher.BROADCAST_PATCHER_STATE";

    public static final String ACTION = "action";
    public static final String ACTION_PATCH_FILE = "patch_file";
    public static final String ACTION_GET_PATCHER_INFORMATION = "get_patcher_information";
    public static final String ACTION_CHECK_SUPPORTED = "check_supported";

    public static final String STATE = "state";
    public static final String STATE_ADD_TASK = "add_task";
    public static final String STATE_UPDATE_TASK = "update_task";
    public static final String STATE_UPDATE_DETAILS = "update_details";
    public static final String STATE_NEW_OUTPUT_LINE = "new_output_line";
    public static final String STATE_SET_PROGRESS_MAX = "set_progress_max";
    public static final String STATE_SET_PROGRESS = "set_progress";
    public static final String STATE_FETCHED_PATCHER_INFO = "fetched_patcher_info";
    public static final String STATE_PATCHED_FILE = "patched_file";
    public static final String STATE_CHECKED_SUPPORTED = "checked_supported";

    public static final String RESULT_TASK = "task";
    public static final String RESULT_DETAILS = "details";
    public static final String RESULT_LINE = "line";
    public static final String RESULT_STREAM = "stream";
    public static final String RESULT_MAX_PROGRESS = "max_progress";
    public static final String RESULT_PROGRESS = "progress";
    public static final String RESULT_PATCHER_INFO = "patcher_info";
    public static final String RESULT_FILENAME = "filename";
    public static final String RESULT_SUPPORTED = "supported";

    public PatcherService() {
        super(TAG);
    }

    private void addTask(String task) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_ADD_TASK);
        i.putExtra(RESULT_TASK, task);
        sendBroadcast(i);
    }

    private void updateTask(String task) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_UPDATE_TASK);
        i.putExtra(RESULT_TASK, task);
        sendBroadcast(i);
    }

    private void updateDetails(String details) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_UPDATE_DETAILS);
        i.putExtra(RESULT_DETAILS, details);
        sendBroadcast(i);
    }

    private void newOutputLine(String line, String stream) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_NEW_OUTPUT_LINE);
        i.putExtra(RESULT_LINE, line);
        i.putExtra(RESULT_STREAM, stream);
        sendBroadcast(i);
    }

    private void setProgressMax(int max) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_SET_PROGRESS_MAX);
        i.putExtra(RESULT_MAX_PROGRESS, max);
        sendBroadcast(i);
    }

    private void setProgress(int value) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_SET_PROGRESS);
        i.putExtra(RESULT_PROGRESS, value);
        sendBroadcast(i);
    }

    private void onFetchedPatcherInformation(PatcherInformation info) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_FETCHED_PATCHER_INFO);
        i.putExtra(RESULT_PATCHER_INFO, info);
        sendBroadcast(i);
    }

    private void onPatchedFile(boolean failed, String message, String newFile) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_PATCHED_FILE);
        i.putExtra(PatcherUtils.RESULT_PATCH_FILE_FAILED, failed);
        i.putExtra(PatcherUtils.RESULT_PATCH_FILE_MESSAGE, message);
        i.putExtra(PatcherUtils.RESULT_PATCH_FILE_NEW_FILE, newFile);
        sendBroadcast(i);
    }

    private void onCheckedSupported(String filename, boolean supported) {
        Intent i = new Intent(BROADCAST_INTENT);
        i.putExtra(STATE, STATE_CHECKED_SUPPORTED);
        i.putExtra(RESULT_FILENAME, filename);
        i.putExtra(RESULT_SUPPORTED, supported);
        sendBroadcast(i);
    }

    private void getPatcherInformation() {
        try {
            GetPatcherInformation task = new GetPatcherInformation();
            task.start();
            task.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    private void patchFile(Bundle data) {
        try {
            PatchFile task = new PatchFile(data);
            task.start();
            task.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    private void checkSupported(Bundle data) {
        try {
            CheckSupported task = new CheckSupported(data);
            task.start();
            task.join();
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        String action = intent.getStringExtra(ACTION);

        if (ACTION_GET_PATCHER_INFORMATION.equals(action)) {
            getPatcherInformation();
        } else if (ACTION_PATCH_FILE.equals(action)) {
            patchFile(intent.getExtras());
        } else if (ACTION_CHECK_SUPPORTED.equals(action)) {
            checkSupported(intent.getExtras());
        }
    }

    private class GetPatcherInformation extends Thread {
        @Override
        public void run() {
            PatcherInformation info = PatcherUtils
                    .getPatcherInformation(PatcherService.this);

            onFetchedPatcherInformation(info);
        }
    }

    private class PatchFile extends Thread implements CommandListener {
        private final Bundle mData;

        public PatchFile(Bundle data) {
            mData = data;
        }

        @Override
        public void run() {
            Bundle data = PatcherUtils.patchFile(PatcherService.this, this,
                    mData);

            if (data != null) {
                String newFile = data
                        .getString(PatcherUtils.RESULT_PATCH_FILE_NEW_FILE);
                String message = data
                        .getString(PatcherUtils.RESULT_PATCH_FILE_MESSAGE);
                boolean failed = data
                        .getBoolean(PatcherUtils.RESULT_PATCH_FILE_FAILED);
                onPatchedFile(failed, message, newFile);
            }
        }

        @Override
        public void onNewOutputLine(String line, String stream) {
            if (line.contains("ADDTASK:")) {
                addTask(line.replace("ADDTASK:", ""));
            } else if (line.contains("SETTASK:")) {
                updateTask(line.replace("SETTASK:", ""));
            } else if (line.contains("SETDETAILS:")) {
                updateDetails(line.replace("SETDETAILS:", ""));
            } else if (line.contains("SETMAXPROGRESS:")) {
                setProgressMax(Integer.parseInt(line.replace("SETMAXPROGRESS:",
                        "")));
            } else if (line.contains("SETPROGRESS:")) {
                setProgress(Integer.parseInt(line.replace("SETPROGRESS:", "")));
            } else {
                newOutputLine(line, stream);
            }
        }

        @Override
        public void onCommandCompletion(CommandResult result) {
        }
    }

    private class CheckSupported extends Thread {
        private final Bundle mData;

        public CheckSupported(Bundle data) {
            mData = data;
        }

        @Override
        public void run() {
            boolean supported = PatcherUtils.isFileSupported(
                    PatcherService.this, mData);

            onCheckedSupported(mData.getString(PatcherUtils.PARAM_FILENAME),
                    supported);
        }
    }
}
