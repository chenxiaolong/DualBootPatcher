package com.github.chenxiaolong.dualbootpatcher;

import android.app.Fragment;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Bundle;
import android.text.method.LinkMovementMethod;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

public class AboutFragment extends Fragment {
    public static final String TAG = "about";

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
    }
}
