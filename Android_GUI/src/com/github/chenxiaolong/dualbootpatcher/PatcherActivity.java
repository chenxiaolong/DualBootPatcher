package com.github.chenxiaolong.dualbootpatcher;

import android.app.Activity;
import android.app.FragmentManager;
import android.content.Intent;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.view.WindowManager;
import android.widget.LinearLayout;

public class PatcherActivity extends Activity {
    private boolean mRunning = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_patcher);

        // Keep screen on
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        // Include version in action bar
        try {
            String version = getPackageManager().getPackageInfo(
                    getPackageName(), 0).versionName;
            setTitle(getTitle() + " (v" + version + ")");
        } catch (NameNotFoundException e) {
            e.printStackTrace();
        }

        LinearLayout fragmentContainer = (LinearLayout) findViewById(R.id.fragment_container);
        // Immersive mode in Kit Kat
        if (Build.VERSION.SDK_INT >= 19) {
            fragmentContainer
                    .setSystemUiVisibility(View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                            | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                            | View.SYSTEM_UI_FLAG_FULLSCREEN
                            | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY);
        }

        FragmentManager fm = getFragmentManager();

        ProgressFragment pf = (ProgressFragment) fm
                .findFragmentByTag("progress_fragment");
        if (pf == null) {
            pf = new ProgressFragment();
            fm.beginTransaction()
                    .add(R.id.fragment_container, pf, "progress_fragment")
                    .commit();
        }

        PatcherUIFragment puif = (PatcherUIFragment) fm
                .findFragmentByTag("patcherui_fragment");
        if (puif == null) {
            puif = new PatcherUIFragment();
            fm.beginTransaction()
                    .add(R.id.fragment_container, puif, "patcherui_fragment")
                    .commit();
            puif.mProgressListener = pf;
        }

        if (getResources().getConfiguration().orientation == Configuration.ORIENTATION_LANDSCAPE) {
            fragmentContainer.setOrientation(LinearLayout.HORIZONTAL);
        } else {
            fragmentContainer.setOrientation(LinearLayout.VERTICAL);
        }
    }

    @Override
    public void onPostCreate(Bundle savedInstanceState) {
        super.onPostCreate(savedInstanceState);
        if (!mRunning) {
            mRunning = true;
            Intent intent = new Intent(this, PatcherService.class);
            intent.putExtra(PatcherService.ACTION,
                    PatcherService.ACTION_PATCH_FILE);
            intent.putExtra("zipFile", getIntent().getStringExtra("zipFile"));
            intent.putExtra("partConfig",
                    getIntent().getStringExtra("partConfig"));
            intent.putExtra("device", getIntent().getStringExtra("device"));
            startService(intent);
        }
    }

    @Override
    public void onSaveInstanceState(Bundle savedInstanceState) {
        super.onSaveInstanceState(savedInstanceState);
        savedInstanceState.putBoolean("running", mRunning);
    }

    @Override
    public void onRestoreInstanceState(Bundle savedInstanceState) {
        super.onRestoreInstanceState(savedInstanceState);
        mRunning = savedInstanceState.getBoolean("running");
    }
}
