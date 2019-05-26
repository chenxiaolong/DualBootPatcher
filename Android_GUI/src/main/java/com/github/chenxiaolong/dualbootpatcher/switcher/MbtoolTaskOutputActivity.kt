/*
 * Copyright (C) 2014-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.switcher

import android.os.Bundle
import android.view.MenuItem
import android.view.WindowManager
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.fragment.app.transaction
import com.github.chenxiaolong.dualbootpatcher.R

class MbtoolTaskOutputActivity : AppCompatActivity() {
    private lateinit var fragment: MbtoolTaskOutputFragment

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_mbtool_task_output)

        if (savedInstanceState == null) {
            fragment = MbtoolTaskOutputFragment()
            fragment.arguments = intent.extras
            supportFragmentManager.transaction {
                add(R.id.content_frame, fragment, MbtoolTaskOutputFragment.FRAGMENT_TAG)
            }
        } else {
            fragment = supportFragmentManager.findFragmentByTag(
                    MbtoolTaskOutputFragment.FRAGMENT_TAG) as MbtoolTaskOutputFragment
        }

        setSupportActionBar(findViewById(R.id.toolbar))
        supportActionBar!!.setDisplayHomeAsUpEnabled(true)

        // Keep screen on
        window.addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON)
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        when (item.itemId) {
            android.R.id.home -> {
                if (fragment.isRunning) {
                    showWaitUntilFinished()
                } else {
                    finish()
                }
                return true
            }
        }
        return super.onOptionsItemSelected(item)
    }

    override fun onBackPressed() {
        if (fragment.isRunning) {
            showWaitUntilFinished()
        } else {
            super.onBackPressed()
        }
    }

    private fun showWaitUntilFinished() {
        Toast.makeText(this, R.string.wait_until_finished, Toast.LENGTH_SHORT).show()
    }
}