/*
 * Copyright (C) 2016-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.pathchooser

import android.app.Activity
import android.content.Intent
import android.content.SharedPreferences
import android.net.Uri
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.edit

import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.pathchooser.PathChooserDialog.FileChooserDialogListener
import com.github.chenxiaolong.dualbootpatcher.pathchooser.PathChooserDialog.Type

import java.io.File

class PathChooserActivity : AppCompatActivity(), FileChooserDialogListener {
    private lateinit var prefs: SharedPreferences

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_path_chooser)

        prefs = getSharedPreferences("path_chooser", 0)

        if (savedInstanceState == null) {
            val intent = intent
            val action = intent.action
            val mimeType = intent.type
            val defaultName = intent.getStringExtra(Intent.EXTRA_TITLE)
            val type: Type

            type = when (action) {
                ACTION_OPEN_FILE -> Type.OPEN_FILE
                ACTION_OPEN_DIRECTORY -> Type.OPEN_DIRECTORY
                ACTION_SAVE_FILE -> Type.SAVE_FILE
                else -> throw IllegalArgumentException("Invalid action: $action")
            }

            val builder = PathChooserDialog.Builder(type)
            if (mimeType != null) {
                builder.mimeType(mimeType)
            }
            if (defaultName != null) {
                builder.defaultName(defaultName)
            }

            val lastCwd = prefs.getString("last_cwd", null)
            if (lastCwd != null && File(lastCwd).isDirectory) {
                builder.initialPath(lastCwd)
            }

            val dialog = builder.buildFromActivity(DIALOG_PATH_CHOOSER)
            dialog.show(supportFragmentManager, DIALOG_PATH_CHOOSER)
        }
    }

    override fun onPathSelected(tag: String, file: File) {
        if (DIALOG_PATH_CHOOSER == tag) {
            prefs.edit {
                putString("last_cwd", file.parent)
            }

            val intent = Intent()
            intent.data = Uri.fromFile(file)
            setResult(Activity.RESULT_OK, intent)
            finish()
        }
    }

    override fun onPathSelectionCancelled(tag: String) {
        if (DIALOG_PATH_CHOOSER == tag) {
            setResult(Activity.RESULT_CANCELED)
            finish()
        }
    }

    companion object {
        val ACTION_OPEN_FILE =
                "${PathChooserActivity::class.java.name}.open_file"
        val ACTION_OPEN_DIRECTORY =
                "${PathChooserActivity::class.java.name}.open_directory"
        val ACTION_SAVE_FILE =
                "${PathChooserActivity::class.java.name}.save_file"

        private val DIALOG_PATH_CHOOSER =
                "${PathChooserActivity::class.java.name}.path_chooser"
    }
}