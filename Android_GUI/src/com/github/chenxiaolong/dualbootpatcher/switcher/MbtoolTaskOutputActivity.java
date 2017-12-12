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

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.MenuItem;
import android.view.WindowManager;
import android.widget.Toast;

import com.github.chenxiaolong.dualbootpatcher.R;

public class MbtoolTaskOutputActivity extends AppCompatActivity {
    MbtoolTaskOutputFragment mFragment;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_mbtool_task_output);

        if (savedInstanceState == null) {
            mFragment = new MbtoolTaskOutputFragment();
            mFragment.setArguments(getIntent().getExtras());
            getFragmentManager().beginTransaction().add(
                    R.id.content_frame, mFragment, MbtoolTaskOutputFragment.FRAGMENT_TAG).commit();
        } else {
            mFragment = (MbtoolTaskOutputFragment)
                    getFragmentManager().findFragmentByTag(MbtoolTaskOutputFragment.FRAGMENT_TAG);
        }

        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);

        // Keep screen on
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case android.R.id.home:
            if (mFragment.isRunning()) {
                showWaitUntilFinished();
            } else {
                finish();
            }
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onBackPressed() {
        if (mFragment.isRunning()) {
            showWaitUntilFinished();
        } else {
            super.onBackPressed();
        }
    }

    private void showWaitUntilFinished() {
        Toast.makeText(this, R.string.wait_until_finished, Toast.LENGTH_SHORT).show();
    }
}
