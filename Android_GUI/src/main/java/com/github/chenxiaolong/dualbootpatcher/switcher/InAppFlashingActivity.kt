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
import android.view.Menu
import android.view.MenuItem
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.github.chenxiaolong.dualbootpatcher.MenuUtils
import com.github.chenxiaolong.dualbootpatcher.R
import com.github.chenxiaolong.dualbootpatcher.switcher.InAppFlashingFragment.OnReadyStateChangedListener

class InAppFlashingActivity : AppCompatActivity(), OnReadyStateChangedListener {
    private var showCheckIcon: Boolean = false

    public override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_in_app_flashing)

        setSupportActionBar(findViewById(R.id.toolbar))
        supportActionBar!!.setDisplayHomeAsUpEnabled(true)
    }

    override fun onCreateOptionsMenu(menu: Menu): Boolean {
        val inflater = menuInflater
        inflater.inflate(R.menu.actionbar_check, menu)

        val primary = ContextCompat.getColor(this, R.color.text_color_primary)
        MenuUtils.tintAllMenuIcons(menu, primary)

        val checkItem = menu.findItem(R.id.check_item)
        if (!showCheckIcon) {
            checkItem.isVisible = false
        }

        return super.onCreateOptionsMenu(menu)
    }

    override fun onOptionsItemSelected(item: MenuItem): Boolean {
        when (item.itemId) {
            android.R.id.home -> {
                finish()
                return true
            }
            R.id.check_item -> {
                val frag = supportFragmentManager.findFragmentById(R.id.zip_flashing_fragment)
                        as InAppFlashingFragment
                frag.onActionBarCheckItemClicked()
                return true
            }
        }
        return super.onOptionsItemSelected(item)
    }

    override fun onReady(ready: Boolean) {
        showCheckIcon = ready
        invalidateOptionsMenu()
    }

    override fun onFinished() {
        finish()
    }
}