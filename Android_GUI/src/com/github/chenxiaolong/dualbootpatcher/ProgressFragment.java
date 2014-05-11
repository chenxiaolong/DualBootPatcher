package com.github.chenxiaolong.dualbootpatcher;

import java.util.ArrayList;

import android.app.Fragment;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Color;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

public class ProgressFragment extends Fragment implements
        PatcherUIFragment.ProgressListener {
    private LinearLayout mTasksContainer;
    private ArrayList<Task> mTasks = new ArrayList<Task>();
    private LayoutInflater mInflater;

    private class Task {
        public LinearLayout mItem;

        public ImageView mImageView;
        public boolean mImageViewShown;
        public boolean mImageViewChecked;

        public ProgressBar mProgressBar;
        public boolean mProgressBarShown;

        public TextView mTextView;
        public String mName;
        public int mColor;
    }

    public ProgressFragment() {
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View v = inflater.inflate(R.layout.progress_view, container, false);

        CardBackground background = new CardBackground(getResources()
                .getDisplayMetrics());

        if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
            background.widgetOnRight();
        } else {
            background.widgetOnBottom();
        }

        v.setBackground(background);

        mInflater = (LayoutInflater) getActivity().getSystemService(
                Context.LAYOUT_INFLATER_SERVICE);

        mTasksContainer = (LinearLayout) v.findViewById(R.id.tasks_container);

        for (int i = 0; i < mTasks.size(); i++) {
            Task t = mTasks.get(i);
            showTask(t);
            mTasksContainer.addView(t.mItem);
        }

        return v;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setRetainInstance(true);
    }

    @Override
    public void onDetach() {
        for (int i = 0; i < mTasks.size(); i++) {
            mTasks.get(i).mImageView = null;
            mTasks.get(i).mProgressBar = null;
            mTasks.get(i).mTextView = null;
            mTasks.get(i).mItem = null;
        }

        mTasksContainer = null;

        mInflater = null;

        super.onDetach();
    }

    @Override
    public void onAddTask(String task) {
        Task t = new Task();

        t.mName = task;
        t.mColor = Color.BLACK;

        t.mImageViewChecked = false;
        t.mImageViewShown = true;

        t.mProgressBarShown = false;

        mTasks.add(t);
        showTask(t);
        mTasksContainer.addView(t.mItem);
    }

    @Override
    public void onUpdateTask(String task) {
        boolean reachedTask = false;

        for (int i = 0; i < mTasks.size(); i++) {
            Task t = mTasks.get(i);

            if (t.mName.equals(task)) {
                t.mProgressBarShown = true;
                t.mImageViewShown = false;

                reachedTask = true;

                t.mColor = Color.BLACK;
            } else {
                t.mProgressBarShown = false;
                t.mImageViewShown = true;

                if (!reachedTask) {
                    t.mImageViewChecked = true;
                } else {
                    t.mImageViewChecked = false;
                }

                t.mColor = Color.LTGRAY;
            }

            showTask(t);
        }
    }

    private void showTask(Task t) {
        if (getActivity() != null) {
            if (t.mItem == null) {
                t.mItem = (LinearLayout) mInflater.inflate(R.layout.task_item,
                        null);
            }

            if (t.mTextView == null) {
                t.mTextView = (TextView) t.mItem.findViewById(R.id.task_text);
            }

            if (t.mImageView == null) {
                t.mImageView = (ImageView) t.mItem.findViewById(R.id.task_icon);
            }

            if (t.mProgressBar == null) {
                t.mProgressBar = (ProgressBar) t.mItem
                        .findViewById(R.id.task_progress);
            }

            t.mTextView.setText(t.mName);
            t.mTextView.setTextColor(t.mColor);

            if (t.mImageViewShown) {
                t.mImageView.setVisibility(View.VISIBLE);
            } else {
                t.mImageView.setVisibility(View.GONE);
            }
            if (t.mImageViewChecked) {
                t.mImageView
                        .setImageResource(R.drawable.btn_check_on_holo_light);
            } else {
                t.mImageView
                        .setImageResource(R.drawable.btn_check_off_holo_light);
            }

            if (t.mProgressBarShown) {
                t.mProgressBar.setVisibility(View.VISIBLE);
            } else {
                t.mProgressBar.setVisibility(View.GONE);
            }
        }
    }
}
