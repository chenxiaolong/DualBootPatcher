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

package com.github.chenxiaolong.dualbootpatcher.freespace;

import android.app.Fragment;
import android.content.Context;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.os.StatFs;
import android.support.v4.widget.SwipeRefreshLayout;
import android.support.v4.widget.SwipeRefreshLayout.OnRefreshListener;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.FileUtils;
import com.github.chenxiaolong.dualbootpatcher.FileUtils.MountEntry;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.views.CircularProgressBar;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Random;

public class FreeSpaceFragment extends Fragment {
    public static final String FRAGMENT_TAG = FreeSpaceFragment.class.getSimpleName();
    private static final String TAG = FreeSpaceFragment.class.getSimpleName();

    private ArrayList<MountInfo> mMounts = new ArrayList<>();
    private MountInfoAdapter mAdapter;

    private static final int[] COLORS = new int[] {
            Color.parseColor("#F44336"),
            Color.parseColor("#8E24AA"),
            Color.parseColor("#3F51B5"),
            Color.parseColor("#2196F3"),
            Color.parseColor("#4CAF50"),
            Color.parseColor("#FBC02D"),
            Color.parseColor("#E65100"),
            Color.parseColor("#607D8B")
    };

    private static final HashSet<String> SKIPPED_FSTYPES = new HashSet<>();
    private static final HashSet<String> SKIPPED_FSNAMES = new HashSet<>();

    static {
        SKIPPED_FSTYPES.add("cgroup");
        SKIPPED_FSTYPES.add("debugfs");
        SKIPPED_FSTYPES.add("devpts");
        SKIPPED_FSTYPES.add("proc");
        SKIPPED_FSTYPES.add("pstore");
        SKIPPED_FSTYPES.add("rootfs");
        SKIPPED_FSTYPES.add("selinuxfs");
        SKIPPED_FSTYPES.add("sysfs");
        SKIPPED_FSTYPES.add("tmpfs");

        SKIPPED_FSNAMES.add("debugfs");
        SKIPPED_FSNAMES.add("devpts");
        SKIPPED_FSNAMES.add("none");
        SKIPPED_FSNAMES.add("proc");
        SKIPPED_FSNAMES.add("pstore");
        SKIPPED_FSNAMES.add("rootfs");
        SKIPPED_FSNAMES.add("selinuxfs");
        SKIPPED_FSNAMES.add("sysfs");
        SKIPPED_FSNAMES.add("tmpfs");
    }

    public static FreeSpaceFragment newInstance() {
        return new FreeSpaceFragment();
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        mAdapter = new MountInfoAdapter(getActivity(), mMounts, COLORS);

        RecyclerView rv = (RecyclerView) getActivity().findViewById(R.id.mountpoints);
        rv.setHasFixedSize(true);
        rv.setAdapter(mAdapter);

        LinearLayoutManager llm = new LinearLayoutManager(getActivity());
        llm.setOrientation(LinearLayoutManager.VERTICAL);
        rv.setLayoutManager(llm);

        final SwipeRefreshLayout srl =
                (SwipeRefreshLayout) getActivity().findViewById(R.id.swiperefresh);
        srl.setOnRefreshListener(new OnRefreshListener() {
            @Override
            public void onRefresh() {
                refreshMounts();
                srl.setRefreshing(false);
            }
        });

        srl.setColorSchemeResources(R.color.swipe_refresh_color1, R.color.swipe_refresh_color2,
                R.color.swipe_refresh_color3, R.color.swipe_refresh_color4);

        refreshMounts();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_free_space, container, false);
    }

    private void refreshMounts() {
        mMounts.clear();

        MountEntry[] entries;

        try {
            entries = FileUtils.getMounts();
        } catch (IOException e) {
            Log.e(TAG, "Failed to get mount entries", e);
            return;
        }

        for (MountEntry entry : entries) {
            MountInfo info = new MountInfo();
            info.mountpoint = entry.mnt_dir;
            info.fsname = entry.mnt_fsname;
            info.fstype = entry.mnt_type;

            // Ignore irrelevant filesystems
            if (SKIPPED_FSTYPES.contains(info.fstype)
                    || SKIPPED_FSNAMES.contains(info.fsname)
                    || info.mountpoint.startsWith("/mnt")
                    || info.mountpoint.startsWith("/dev")
                    || info.mountpoint.startsWith("/proc")
                    || info.mountpoint.startsWith("/data/data")) {
                continue;
            }

            StatFs statFs;

            try {
                statFs = new StatFs(info.mountpoint);
            } catch (IllegalArgumentException e) {
                // Thrown if Os.statvfs() throws ErrnoException
                Log.e(TAG, "Exception during statfs of " + info.mountpoint + ": " + e.getMessage());
                continue;
            }

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
                info.totalSpace = statFs.getBlockSizeLong() * statFs.getBlockCountLong();
                info.availSpace = statFs.getBlockSizeLong() * statFs.getAvailableBlocksLong();
            } else {
                info.totalSpace = statFs.getBlockSize() * statFs.getBlockCount();
                info.availSpace = statFs.getBlockSize() * statFs.getAvailableBlocks();
            }

            mMounts.add(info);
        }

        mAdapter.notifyDataSetChanged();
    }

    private static class MountInfo {
        public String mountpoint;
        public String fsname;
        public String fstype;
        public long totalSpace;
        public long availSpace;
    }

    protected static class MountInfoViewHolder extends ViewHolder {
        protected TextView vMountPoint;
        protected TextView vTotalSize;
        protected TextView vAvailSize;
        protected CircularProgressBar vProgress;

        public MountInfoViewHolder(View v) {
            super(v);
            vMountPoint = (TextView) v.findViewById(R.id.mount_point);
            vTotalSize = (TextView) v.findViewById(R.id.size_total);
            vAvailSize = (TextView) v.findViewById(R.id.size_free);
            vProgress = (CircularProgressBar) v.findViewById(R.id.mountpoint_usage);
        }
    }

    public static class MountInfoAdapter extends RecyclerView.Adapter<MountInfoViewHolder> {
        private Context mContext;
        private List<MountInfo> mMounts;
        private int[] mColors;
        private int mStartingColorIndex;

        public MountInfoAdapter(Context context, List<MountInfo> mounts, int[] colors) {
            mContext = context;
            mMounts = mounts;
            mColors = colors;

            // Choose random color to start from (and go down the list)
            mStartingColorIndex = new Random().nextInt(mColors.length);
        }

        @Override
        public MountInfoViewHolder onCreateViewHolder(ViewGroup parent, int viewType) {
            View view = LayoutInflater
                    .from(parent.getContext())
                    .inflate(R.layout.card_v7_free_space, parent, false);
            return new MountInfoViewHolder(view);
        }

        @Override
        public void onBindViewHolder(MountInfoViewHolder holder, int position) {
            MountInfo info = mMounts.get(position);

            String strTotal = FileUtils.toHumanReadableSize(mContext, info.totalSpace, 2);
            String strAvail = FileUtils.toHumanReadableSize(mContext, info.availSpace, 2);

            holder.vMountPoint.setText(info.mountpoint);
            holder.vTotalSize.setText(strTotal);
            holder.vAvailSize.setText(strAvail);

            if (info.totalSpace == 0) {
                holder.vProgress.setProgress(0);
            } else {
                holder.vProgress.setProgress(1.0f - (float) info.availSpace / info.totalSpace);
            }

            int color = COLORS[(mStartingColorIndex + position) % mColors.length];
            holder.vProgress.setProgressColor(color);
        }

        @Override
        public int getItemCount() {
            return mMounts.size();
        }
    }
}
