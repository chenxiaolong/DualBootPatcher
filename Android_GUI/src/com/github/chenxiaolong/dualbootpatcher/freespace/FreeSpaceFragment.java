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

package com.github.chenxiaolong.dualbootpatcher.freespace;

import android.app.Fragment;
import android.graphics.Color;
import android.os.Bundle;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.nhaarman.listviewanimations.swinginadapters.AnimationAdapter;
import com.nhaarman.listviewanimations.swinginadapters.prepared.AlphaInAnimationAdapter;

import java.util.ArrayList;
import java.util.Random;

import it.gmariotti.cardslib.library.internal.Card;
import it.gmariotti.cardslib.library.internal.CardArrayAdapter;
import it.gmariotti.cardslib.library.view.CardListView;

public class FreeSpaceFragment extends Fragment {
    public static final String TAG = FreeSpaceFragment.class.getSimpleName();

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

    private CardListView mList;

    public static FreeSpaceFragment newInstance() {
        return new FreeSpaceFragment();
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        ArrayList<Card> cards = new ArrayList<Card>();

        // Choose random color to start from (and go down the list)
        int colorIndex = new Random().nextInt(COLORS.length);

        String[] mountpoints = getMountPoints();
        for (String mountpoint : mountpoints) {
            String fstype = getMntType(mountpoint);
            String fsname = getMntFsname(mountpoint);

            // Ignore irrelevant filesystems
            if ("cgroup".equals(fstype)
                    || "debugfs".equals(fsname)
                    || "devpts".equals(fstype)
                    || "proc".equals(fstype)
                    || "rootfs".equals(fstype)
                    || "selinuxfs".equals(fstype)
                    || "sysfs".equals(fstype)
                    || "tmpfs".equals(fstype)) {
                continue;
            } else if ("debugfs".equals(fsname)
                    || "devpts".equals(fsname)
                    || "none".equals(fsname)
                    || "proc".equals(fsname)
                    || "rootfs".equals(fsname)
                    || "selinuxfs".equals(fsname)
                    || "sysfs".equals(fsname)
                    || "tmpfs".equals(fsname)) {
                continue;
            } else if (mountpoint.startsWith("/mnt")) {
                continue;
            }

            long total = getMntTotalSize(mountpoint);
            long avail = getMntAvailSpace(mountpoint);

            int color = COLORS[colorIndex];
            FreeSpaceCard card = new FreeSpaceCard(getActivity(), mountpoint, total, avail, color);
            cards.add(card);

            colorIndex = (colorIndex + 1) % COLORS.length;
        }

        CardArrayAdapter cardArrayAdapter = new CardArrayAdapter(getActivity(), cards);
        if (mList != null) {
            AnimationAdapter animArrayAdapter = new AlphaInAnimationAdapter(cardArrayAdapter);
            animArrayAdapter.setAbsListView(mList);
            mList.setExternalAdapter(animArrayAdapter, cardArrayAdapter);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
                             Bundle savedInstanceState) {
        View view = inflater.inflate(R.layout.free_space_list, container, false);
        mList = (CardListView) view.findViewById(R.id.mountpoints);
        return view;
    }

    static {
        System.loadLibrary("mountpoint-jni");
    }

    private static native String[] getMountPoints();

    private static native String getMntFsname(String mountpoint);

    private static native String getMntType(String mountpoint);

    @SuppressWarnings("unused")
    private static native String getMntOpts(String mountpoint);

    @SuppressWarnings("unused")
    private static native int getMntFreq(String mountpoint);

    @SuppressWarnings("unused")
    private static native int getMntPassno(String mountpoint);

    private static native long getMntTotalSize(String mountpoint);

    private static native long getMntAvailSpace(String mountpoint);
}
