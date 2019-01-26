/*
 * Copyright (C) 2015-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.patcher

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import android.widget.Toast
import androidx.fragment.app.transaction
import com.github.chenxiaolong.dualbootpatcher.R

class AutomatedPatcherActivity : AppCompatActivity() /* , PatcherListener */ {
    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.automated_patcher_layout)

        val prefs = getSharedPreferences("settings", 0)

        if (!prefs.getBoolean(PREF_ALLOW_3RD_PARTY_INTENTS, false)) {
            Toast.makeText(this, R.string.third_party_intents_not_allowed, Toast.LENGTH_LONG).show()
            finish()
            return
        }

        setSupportActionBar(findViewById(R.id.toolbar))

        val args = intent.extras
        if (args == null) {
            finish()
        }

        if (savedInstanceState == null) {
            supportFragmentManager.transaction {
                val frag = PatchFileFragment.newInstance()
                frag.arguments = args
                replace(R.id.content_frame, frag)
            }
        }
    }

    override fun onBackPressed() {
        // We never want the user to exit the activity. The activity will be automatically closed
        // once the patching finishes.
        Toast.makeText(this, R.string.wait_until_finished, Toast.LENGTH_SHORT).show()
    }

    /*
    fun onPatcherResult(code: Int, message: String?, newFile: String?) {
        val intent = Intent()

        when (code) {
            PatchFileFragment.RESULT_PATCHING_SUCCEEDED -> {
                intent.putExtra(RESULT_CODE, "PATCHING_SUCCEEDED")
                intent.putExtra(RESULT_NEW_FILE, newFile)
            }
            PatchFileFragment.RESULT_PATCHING_FAILED -> {
                intent.putExtra(RESULT_CODE, "PATCHING_FAILED")
                intent.putExtra(RESULT_MESSAGE, message)
            }
            PatchFileFragment.RESULT_INVALID_OR_MISSING_ARGUMENTS -> {
                intent.putExtra(RESULT_CODE, "INVALID_OR_MISSING_ARGUMENTS")
                intent.putExtra(RESULT_MESSAGE, message)
            }
        }

        setResult(RESULT_OK, intent)
        finish()
    }
    */

    companion object {
        /*
        val RESULT_CODE = "code"
        val RESULT_MESSAGE = "message"
        val RESULT_NEW_FILE = "new_file"
        */

        private const val PREF_ALLOW_3RD_PARTY_INTENTS = "allow_3rd_party_intents"
    }
}
