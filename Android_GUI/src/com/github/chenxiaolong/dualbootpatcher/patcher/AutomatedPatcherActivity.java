/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import android.content.SharedPreferences;
import android.os.Bundle;
import android.support.v4.app.Fragment;
import android.support.v4.app.FragmentTransaction;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.widget.Toast;

import com.github.chenxiaolong.dualbootpatcher.R;

public class AutomatedPatcherActivity extends AppCompatActivity /* implements PatcherListener */ {
    public static final String RESULT_CODE = "code";
    public static final String RESULT_MESSAGE = "message";
    public static final String RESULT_NEW_FILE = "new_file";

    private static final String PREF_ALLOW_3RD_PARTY_INTENTS = "allow_3rd_party_intents";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.automated_patcher_layout);

        SharedPreferences prefs = getSharedPreferences("settings", 0);

        if (!prefs.getBoolean(PREF_ALLOW_3RD_PARTY_INTENTS, false)) {
            Toast.makeText(this, R.string.third_party_intents_not_allowed, Toast.LENGTH_LONG).show();
            finish();
            return;
        }

        Toolbar toolbar = findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);

        Bundle args = getIntent().getExtras();
        if (args == null) {
            finish();
        }

        if (savedInstanceState == null) {
            FragmentTransaction ft = getSupportFragmentManager().beginTransaction();
            Fragment frag = PatchFileFragment.newInstance();
            frag.setArguments(args);
            ft.replace(R.id.content_frame, frag);
            ft.commit();
        }
    }

    @Override
    public void onBackPressed() {
        // We never want the user to exit the activity. The activity will be automatically closed
        // once the patching finishes.
        Toast.makeText(this, R.string.wait_until_finished, Toast.LENGTH_SHORT).show();
    }

    /*
    @Override
    public void onPatcherResult(int code, @Nullable String message, @Nullable String newFile) {
        Intent intent = new Intent();

        switch (code) {
        case PatchFileFragment.RESULT_PATCHING_SUCCEEDED:
            intent.putExtra(RESULT_CODE, "PATCHING_SUCCEEDED");
            intent.putExtra(RESULT_NEW_FILE, newFile);
            break;
        case PatchFileFragment.RESULT_PATCHING_FAILED:
            intent.putExtra(RESULT_CODE, "PATCHING_FAILED");
            intent.putExtra(RESULT_MESSAGE, message);
            break;
        case PatchFileFragment.RESULT_INVALID_OR_MISSING_ARGUMENTS:
            intent.putExtra(RESULT_CODE, "INVALID_OR_MISSING_ARGUMENTS");
            intent.putExtra(RESULT_MESSAGE, message);
            break;
        }

        setResult(Activity.RESULT_OK, intent);
        finish();
    }
    */
}
