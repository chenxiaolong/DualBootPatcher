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

package com.github.chenxiaolong.dualbootpatcher.patcher;

import it.gmariotti.cardslib.library.internal.Card;

import java.util.ArrayList;

import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;

public class TasksCard extends Card {
    private final ArrayList<Task> mTasks = new ArrayList<Task>();
    private LinearLayout mTasksContainer;
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

    public TasksCard(Context context) {
        this(context, R.layout.cardcontent_tasks);
    }

    public TasksCard(Context context, int innerLayout) {
        super(context, innerLayout);
    }

    @Override
    public void setupInnerViewElements(ViewGroup parent, View view) {
        if (view != null) {
            mInflater = (LayoutInflater) getContext().getSystemService(
                    Context.LAYOUT_INFLATER_SERVICE);
            mTasksContainer = (LinearLayout) view
                    .findViewById(R.id.tasks_container);

            for (int i = 0; i < mTasks.size(); i++) {
                Task t = mTasks.get(i);
                showTask(t);
                mTasksContainer.addView(t.mItem);
            }
        }
    }

    public void onDetachBlahBlah() {
        for (int i = 0; i < mTasks.size(); i++) {
            mTasks.get(i).mImageView = null;
            mTasks.get(i).mProgressBar = null;
            mTasks.get(i).mTextView = null;
            mTasks.get(i).mItem = null;
        }

        mTasksContainer = null;
        mInflater = null;
    }

    public void addTask(String task) {
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

    public void updateTask(String task) {
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
        if (t.mItem == null) {
            t.mItem = (LinearLayout) mInflater
                    .inflate(R.layout.task_item, null);
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
            t.mImageView.setImageResource(R.drawable.btn_check_on_holo_light);
        } else {
            t.mImageView.setImageResource(R.drawable.btn_check_off_holo_light);
        }

        if (t.mProgressBarShown) {
            t.mProgressBar.setVisibility(View.VISIBLE);
        } else {
            t.mProgressBar.setVisibility(View.GONE);
        }
    }

    public void reset() {
        for (int i = mTasks.size() - 1; i >= 0; i--) {
            Task t = mTasks.get(i);
            mTasksContainer.removeView(t.mItem);
            t.mImageView = null;
            t.mProgressBar = null;
            t.mTextView = null;
            t.mItem = null;
            mTasks.remove(i);
        }
    }

    public void onSaveInstanceState(Bundle outState) {
        int n = mTasks.size();
        outState.putInt("tasks_size", n);

        if (n > 0) {
            boolean[] imageShown = new boolean[n];
            boolean[] imageChecked = new boolean[n];
            boolean[] progressShown = new boolean[n];
            String[] name = new String[n];
            int[] color = new int[n];

            for (int i = 0; i < n; i++) {
                Task t = mTasks.get(i);
                imageShown[i] = t.mImageViewShown;
                imageChecked[i] = t.mImageViewChecked;
                progressShown[i] = t.mProgressBarShown;
                name[i] = t.mName;
                color[i] = t.mColor;
            }

            outState.putBooleanArray("tasks_imageshown", imageShown);
            outState.putBooleanArray("tasks_imagechecked", imageChecked);
            outState.putBooleanArray("tasks_progressshown", progressShown);
            outState.putStringArray("tasks_name", name);
            outState.putIntArray("tasks_color", color);
        }
    }

    public void onRestoreInstanceState(Bundle savedInstanceState) {
        int n = savedInstanceState.getInt("tasks_size");

        if (n > 0) {
            boolean[] imageShown = savedInstanceState
                    .getBooleanArray("tasks_imageshown");
            boolean[] imageChecked = savedInstanceState
                    .getBooleanArray("tasks_imagechecked");
            boolean[] progressShown = savedInstanceState
                    .getBooleanArray("tasks_progressshown");
            String[] name = savedInstanceState.getStringArray("tasks_name");
            int[] color = savedInstanceState.getIntArray("tasks_color");

            for (int i = 0; i < n; i++) {
                Task t = new Task();
                t.mImageViewShown = imageShown[i];
                t.mImageViewChecked = imageChecked[i];
                t.mProgressBarShown = progressShown[i];
                t.mName = name[i];
                t.mColor = color[i];
                mTasks.add(t);
            }
        }
    }

    // if (getResources().getConfiguration().orientation ==
    // Configuration.ORIENTATION_LANDSCAPE) {
    // } else {
    // }
}
