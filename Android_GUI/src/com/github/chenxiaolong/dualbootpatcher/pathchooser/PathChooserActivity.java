/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.pathchooser;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.net.Uri;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.support.v7.app.AppCompatActivity;

import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.pathchooser.PathChooserDialog.FileChooserDialogListener;
import com.github.chenxiaolong.dualbootpatcher.pathchooser.PathChooserDialog.Type;

import java.io.File;

public class PathChooserActivity extends AppCompatActivity implements FileChooserDialogListener {
    public static final String ACTION_OPEN_FILE =
            PathChooserActivity.class.getCanonicalName() + ".open_file";
    public static final String ACTION_OPEN_DIRECTORY =
            PathChooserActivity.class.getCanonicalName() + ".open_directory";
    public static final String ACTION_SAVE_FILE =
            PathChooserActivity.class.getCanonicalName() + ".save_file";

    private static final String DIALOG_PATH_CHOOSER =
            PathChooserActivity.class.getCanonicalName() + ".path_chooser";

    private SharedPreferences mPrefs;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_path_chooser);

        mPrefs = getSharedPreferences("path_chooser", 0);

        if (savedInstanceState == null) {
            Intent intent = getIntent();
            String action = intent.getAction();
            String mimeType = intent.getType();
            String defaultName = intent.getStringExtra(Intent.EXTRA_TITLE);
            Type type;

            if (ACTION_OPEN_FILE.equals(action)) {
                type = Type.OPEN_FILE;
            } else if (ACTION_OPEN_DIRECTORY.equals(action)) {
                type = Type.OPEN_DIRECTORY;
            } else if (ACTION_SAVE_FILE.equals(action)) {
                type = Type.SAVE_FILE;
            } else {
                throw new IllegalArgumentException("Invalid action: " + action);
            }

            PathChooserDialog.Builder builder = new PathChooserDialog.Builder(type);
            if (mimeType != null) {
                builder.mimeType(mimeType);
            }
            if (defaultName != null) {
                builder.defaultName(defaultName);
            }

            String lastCwd = mPrefs.getString("last_cwd", null);
            if (lastCwd != null && new File(lastCwd).isDirectory()) {
                builder.initialPath(lastCwd);
            }

            PathChooserDialog dialog = builder.buildFromActivity(DIALOG_PATH_CHOOSER);
            dialog.show(getSupportFragmentManager(), DIALOG_PATH_CHOOSER);
        }
    }

    @Override
    public void onPathSelected(@Nullable String tag, @NonNull File file) {
        if (DIALOG_PATH_CHOOSER.equals(tag)) {
            Editor editor = mPrefs.edit();
            editor.putString("last_cwd", file.getParent());
            editor.apply();

            Intent intent = new Intent();
            intent.setData(Uri.fromFile(file));
            setResult(Activity.RESULT_OK, intent);
            finish();
        }
    }

    @Override
    public void onPathSelectionCancelled(@Nullable String tag) {
        if (DIALOG_PATH_CHOOSER.equals(tag)) {
            setResult(Activity.RESULT_CANCELED);
            finish();
        }
    }
}
