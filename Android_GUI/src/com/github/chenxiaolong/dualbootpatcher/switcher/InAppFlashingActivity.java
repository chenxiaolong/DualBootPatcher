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
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

import com.github.chenxiaolong.dualbootpatcher.MenuUtils;
import com.github.chenxiaolong.dualbootpatcher.R;
import com.github.chenxiaolong.dualbootpatcher.switcher.InAppFlashingFragment
        .OnReadyStateChangedListener;

public class InAppFlashingActivity extends AppCompatActivity implements OnReadyStateChangedListener {
    boolean mShowCheckIcon;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_in_app_flashing);

        Toolbar toolbar = (Toolbar) findViewById(R.id.toolbar);
        setSupportActionBar(toolbar);
        getSupportActionBar().setDisplayHomeAsUpEnabled(true);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        MenuInflater inflater = getMenuInflater();
        inflater.inflate(R.menu.actionbar_check, menu);

        int primary = ContextCompat.getColor(this, R.color.text_color_primary);
        MenuUtils.tintAllMenuIcons(menu, primary);

        MenuItem checkItem = menu.findItem(R.id.check_item);
        if (!mShowCheckIcon) {
            checkItem.setVisible(false);
        }

        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
        case android.R.id.home:
            finish();
            return true;
        case R.id.check_item:
            InAppFlashingFragment frag = (InAppFlashingFragment)
                    getFragmentManager().findFragmentById(R.id.zip_flashing_fragment);
            frag.onActionBarCheckItemClicked();
            return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    public void onReady(boolean ready) {
        mShowCheckIcon = ready;
        invalidateOptionsMenu();
    }

    @Override
    public void onFinished() {
        finish();
    }
}
