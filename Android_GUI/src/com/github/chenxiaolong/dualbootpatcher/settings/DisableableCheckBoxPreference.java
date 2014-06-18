/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

package com.github.chenxiaolong.dualbootpatcher.settings;

import android.content.Context;
import android.preference.CheckBoxPreference;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;

public class DisableableCheckBoxPreference extends CheckBoxPreference {
    private boolean mVisuallyEnabled = true;

    public DisableableCheckBoxPreference(Context context) {
        super(context);
    }

    public DisableableCheckBoxPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public DisableableCheckBoxPreference(Context context, AttributeSet attrs,
            int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    protected void onBindView(View view) {
        super.onBindView(view);

        boolean enabled = isEnabled() && mVisuallyEnabled;
        enableView(view, enabled);
    }

    private void enableView(View view, boolean enable) {
        view.setEnabled(enable);
        if (view instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) view;
            for (int i = 0; i < group.getChildCount(); i++) {
                enableView(group.getChildAt(i), enable);
            }
        }
    }

    public boolean isVisuallyEnabled() {
        return mVisuallyEnabled;
    }

    public void setVisuallyEnabled(boolean enabled) {
        mVisuallyEnabled = enabled;
        notifyChanged();
    }
}
