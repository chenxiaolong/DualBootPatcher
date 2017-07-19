/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
import android.os.Bundle;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

public class AboutFragment extends Fragment {
    public static final String FRAGMENT_TAG = AboutFragment.class.getCanonicalName();

    public static AboutFragment newInstance() {
        return new AboutFragment();
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        return inflater.inflate(R.layout.fragment_about, container, false);
    }

    @Override
    public void onActivityCreated(Bundle savedInstanceState) {
        super.onActivityCreated(savedInstanceState);

        TextView name = (TextView) getActivity().findViewById(R.id.about_name);
        name.setText(BuildConfig.APP_NAME_RESOURCE);

        TextView version = (TextView) getActivity().findViewById(R.id.about_version);
        version.setText(String.format(getString(R.string.version), BuildConfig.VERSION_NAME));

        TextView credits = (TextView) getActivity().findViewById(R.id.about_credits);

        String linkable = "<a href=\"%s\">%s</a>\n";
        String newline = "<br />";
        String separator = " | ";

        String creditsText = getString(R.string.credits);
        String sourceCode = String.format(linkable,
                getString(R.string.url_source_code),
                getString(R.string.link_source_code));
        String xdaThread = String.format(linkable,
                getString(R.string.url_xda_thread),
                getString(R.string.link_xda_thread));
        String licenses = String.format(linkable,
                getString(R.string.url_licenses),
                getString(R.string.link_licenses));

        credits.setText(Html.fromHtml(creditsText + newline + newline
                + sourceCode + separator + xdaThread + separator + licenses));
        credits.setMovementMethod(LinkMovementMethod.getInstance());
    }
}
