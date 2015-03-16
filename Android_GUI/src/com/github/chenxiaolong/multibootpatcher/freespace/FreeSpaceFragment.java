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

package com.github.chenxiaolong.multibootpatcher.freespace;

import android.app.Fragment;
import android.content.Context;
import android.graphics.Color;
import android.os.Bundle;
import android.support.v7.widget.LinearLayoutManager;
import android.support.v7.widget.RecyclerView;
import android.support.v7.widget.RecyclerView.ViewHolder;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMiscStuff;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMiscStuff.mntent;
import com.github.chenxiaolong.multibootpatcher.views.CircularProgressBar;
import com.sun.jna.Pointer;

import java.text.DecimalFormat;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class FreeSpaceFragment extends Fragment {
    public static final String TAG = FreeSpaceFragment.class.getSimpleName();

    private ArrayList<MountInfo> mMounts = new ArrayList<>();

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

    public static FreeSpaceFragment newInstance() {
        return new FreeSpaceFragment();
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        Pointer stream = LibMiscStuff.INSTANCE.setmntent("/proc/mounts", "r");
        mntent ent;
        while ((ent = LibMiscStuff.INSTANCE.getmntent(stream)) != null) {
            MountInfo info = new MountInfo();
            info.mountpoint = ent.mnt_dir;
            info.fsname = ent.mnt_fsname;
            info.fstype = ent.mnt_type;

            // Ignore irrelevant filesystems
            if ("cgroup".equals(info.fstype)
                    || "debugfs".equals(info.fstype)
                    || "devpts".equals(info.fstype)
                    || "proc".equals(info.fstype)
                    || "pstore".equals(info.fstype)
                    || "rootfs".equals(info.fstype)
                    || "selinuxfs".equals(info.fstype)
                    || "sysfs".equals(info.fstype)
                    || "tmpfs".equals(info.fstype)) {
                continue;
            } else if ("debugfs".equals(info.fsname)
                    || "devpts".equals(info.fsname)
                    || "none".equals(info.fsname)
                    || "proc".equals(info.fsname)
                    || "pstore".equals(info.fsname)
                    || "rootfs".equals(info.fsname)
                    || "selinuxfs".equals(info.fsname)
                    || "sysfs".equals(info.fsname)
                    || "tmpfs".equals(info.fsname)) {
                continue;
            } else if (info.mountpoint.startsWith("/mnt")
                    || info.mountpoint.startsWith("/dev")
                    || info.mountpoint.startsWith("/proc")) {
                continue;
            }

            info.totalSpace = LibMiscStuff.INSTANCE.get_mnt_total_size(info.mountpoint);
            info.availSpace = LibMiscStuff.INSTANCE.get_mnt_avail_size(info.mountpoint);

            mMounts.add(info);
        }

        LibMiscStuff.INSTANCE.endmntent(stream);

        RecyclerView rv = (RecyclerView) getActivity().findViewById(R.id.mountpoints);
        rv.setHasFixedSize(true);
        rv.setAdapter(new MountInfoAdapter(getActivity(), mMounts, COLORS));

        LinearLayoutManager llm = new LinearLayoutManager(getActivity());
        llm.setOrientation(LinearLayoutManager.VERTICAL);
        rv.setLayoutManager(llm);
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_free_space, container, false);
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
        private DecimalFormat mSizeFormat = new DecimalFormat("#.##");
        private String mFormatBytes;
        private String mFormatKiloBytes;
        private String mFormatMegaBytes;
        private String mFormatGigaBytes;

        public MountInfoAdapter(Context context, List<MountInfo> mounts, int[] colors) {
            mContext = context;
            mMounts = mounts;
            mColors = colors;

            // Choose random color to start from (and go down the list)
            mStartingColorIndex = new Random().nextInt(mColors.length);

            mFormatBytes = mContext.getString(R.string.format_bytes);
            mFormatKiloBytes = mContext.getString(R.string.format_kilobytes);
            mFormatMegaBytes = mContext.getString(R.string.format_megabytes);
            mFormatGigaBytes = mContext.getString(R.string.format_gigabytes);
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

            String strTotal;
            String strAvail;

            if (info.totalSpace < 1e3) {
                strTotal = String.format(mFormatBytes, info.totalSpace);
                strAvail = String.format(mFormatBytes, info.availSpace);
            } else if (info.totalSpace < 1e6) {
                strTotal = String.format(mFormatKiloBytes,
                        mSizeFormat.format((double) info.totalSpace / 1e3));
                strAvail = String.format(mFormatKiloBytes,
                        mSizeFormat.format((double) info.availSpace / 1e3));
            } else if (info.totalSpace < 1e9) {
                strTotal = String.format(mFormatMegaBytes,
                        mSizeFormat.format((double) info.totalSpace / 1e6));
                strAvail = String.format(mFormatMegaBytes,
                        mSizeFormat.format((double) info.availSpace / 1e6));
            } else {
                strTotal = String.format(mFormatGigaBytes,
                        mSizeFormat.format((double) info.totalSpace / 1e9));
                strAvail = String.format(mFormatGigaBytes,
                        mSizeFormat.format((double) info.availSpace / 1e9));
            }

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
