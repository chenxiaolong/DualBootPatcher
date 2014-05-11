package com.github.chenxiaolong.dualbootpatcher;

import android.app.Activity;
import android.app.Fragment;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Configuration;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

public class PatcherUIFragment extends Fragment {
    public interface ProgressListener {
        public void onAddTask(String task);

        public void onUpdateTask(String task);
    }

    ProgressListener mProgressListener;

    private TextView mTask = null;
    private TextView mDetails = null;
    private TextView mPercentage = null;
    private TextView mFiles = null;
    private ProgressBar mProgress = null;

    private String mCurTask = "";
    private String mCurDetails = "";

    private int mCurProgress = 0;
    private int mMaxProgress = 0;

    public PatcherUIFragment() {
    }

    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Bundle bundle = intent.getExtras();

            if (bundle != null) {
                String state = bundle.getString(PatcherService.STATE);

                if (state.equals(PatcherService.STATE_ADD_TASK)) {
                    addTask(bundle.getString("task"));
                } else if (state.equals(PatcherService.STATE_UPDATE_TASK)) {
                    updateTask(bundle.getString("task"), true);
                } else if (state.equals(PatcherService.STATE_UPDATE_DETAILS)) {
                    updateDetails(bundle.getString("details"));
                } else if (state.equals(PatcherService.STATE_NEW_OUTPUT_LINE)) {
                    String line = bundle.getString("line");
                    String stream = bundle.getString("stream");

                    if (stream.equals("stdout")) {
                        updateDetails(line);
                    }
                } else if (state.equals(PatcherService.STATE_SET_PROGRESS_MAX)) {
                    mMaxProgress = bundle.getInt("value");
                    updateProgress();
                } else if (state.equals(PatcherService.STATE_SET_PROGRESS)) {
                    mCurProgress = bundle.getInt("value");
                    updateProgress();
                } else if (state.equals(PatcherService.STATE_PATCHED_FILE)) {
                    updateTask(getActivity().getString(R.string.task_done),
                            false);
                    updateDetails(getActivity()
                            .getString(R.string.details_done));
                    Intent data = new Intent();
                    data.putExtra("failed", bundle.getBoolean("failed"));
                    data.putExtra("message", bundle.getString("exitMsg"));
                    data.putExtra("newZipFile", bundle.getString("newFile"));
                    getActivity().setResult(Activity.RESULT_OK, data);
                    getActivity().finish();
                }

                // Ignored:
                // PatcherService.STATE_EXTRACTED_PATCHER
                // PatcherService.STATE_FETCHED_PART_CONFIGS
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setRetainInstance(true);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.patcherui_view, container, false);

        CardBackground backgroundTop = new CardBackground(getResources()
                .getDisplayMetrics());
        CardBackground backgroundBottom = new CardBackground(getResources()
                .getDisplayMetrics());

        backgroundTop.widgetOnBottom();
        backgroundBottom.widgetOnTop();

        if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
            backgroundTop.widgetOnLeft();
            backgroundBottom.widgetOnLeft();
        } else {
            backgroundTop.widgetOnTop();
        }

        ((LinearLayout) v.findViewById(R.id.lltop))
                .setBackground(backgroundTop);
        ((LinearLayout) v.findViewById(R.id.llbottom))
                .setBackground(backgroundBottom);

        return v;
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        mTask = (TextView) getView().findViewById(R.id.task);
        mDetails = (TextView) getView().findViewById(R.id.details);
        mPercentage = (TextView) getView().findViewById(R.id.percentage);
        mFiles = (TextView) getView().findViewById(R.id.files);
        mProgress = (ProgressBar) getView().findViewById(R.id.progress_bar);

        // Restore values
        updateProgress();
        mTask.setText(mCurTask);
        mDetails.setText(mCurDetails);
    }

    @Override
    public void onResume() {
        super.onResume();
        getActivity().registerReceiver(mReceiver,
                new IntentFilter(PatcherService.BROADCAST_INTENT));
    }

    @Override
    public void onPause() {
        super.onPause();
        getActivity().unregisterReceiver(mReceiver);
    }

    @Override
    public void onDetach() {
        mTask = null;
        mDetails = null;
        mPercentage = null;
        mFiles = null;
        mProgress = null;
        super.onDetach();
    }

    private void addTask(String text) {
        if (mProgressListener != null) {
            mProgressListener.onAddTask(text);
        }
    }

    private void updateTask(String text, boolean ellipsis) {
        mCurTask = text;
        if (mTask != null) {
            mTask.setText(mCurTask + (ellipsis ? "..." : ""));
        }
        if (mProgressListener != null) {
            mProgressListener.onUpdateTask(text);
        }
    }

    private void updateDetails(String text) {
        mCurDetails = text;
        if (mDetails != null) {
            mDetails.setText(mCurDetails);
        }
    }

    private void updateProgress() {
        if (mProgress != null) {
            mProgress.setMax(mMaxProgress);
            mProgress.setProgress(mCurProgress);
        }
        if (mPercentage != null) {
            if (mMaxProgress == 0) {
                mPercentage.setText("0%");
            } else {
                mPercentage.setText(String.format("%.1f%%",
                        (double) mCurProgress / mMaxProgress * 100));
            }
        }
        if (mFiles != null) {
            mFiles.setText(String.format(
                    getString(R.string.overall_progress_files),
                    Integer.toString(mCurProgress),
                    Integer.toString(mMaxProgress)));
        }
    }
}
