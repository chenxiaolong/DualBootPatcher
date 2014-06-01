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
import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.os.Vibrator;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.ImageView;
import android.widget.TextView;

public class AboutFragment extends Fragment {
    public static final String TAG = "about";

    private ImageView mLogo;

    public static AboutFragment newInstance() {
        AboutFragment f = new AboutFragment();

        return f;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_about, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        TextView version = (TextView) getActivity().findViewById(
                R.id.about_version);
        try {
            String versionName = getActivity().getPackageManager()
                    .getPackageInfo(getActivity().getPackageName(), 0).versionName;
            version.setText(String.format(
                    getActivity().getString(R.string.version), versionName));
        } catch (NameNotFoundException e) {
            e.printStackTrace();
        }

        TextView credits = (TextView) getActivity().findViewById(
                R.id.about_credits);
        credits.setMovementMethod(LinkMovementMethod.getInstance());

        mLogo = (ImageView) getActivity().findViewById(R.id.logo);
        mLogo.setOnLongClickListener(new View.OnLongClickListener() {
            @Override
            public boolean onLongClick(View view) {
                Vibrator vb = (Vibrator) getActivity().getSystemService(
                        Context.VIBRATOR_SERVICE);
                vb.vibrate(100);

                SharedPreferences sp = getActivity().getSharedPreferences(
                        "settings", 0);
                boolean showExit = !sp.getBoolean("showExit", false);
                Editor e = sp.edit();
                e.putBoolean("showExit", showExit);
                e.apply();

                ((MainActivity) getActivity()).showExit();

                return false;
            }
        });
    }
}
