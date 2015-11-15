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

package com.github.chenxiaolong.dualbootpatcher.picasso;

import android.graphics.Bitmap;
import android.support.v7.graphics.Palette;

import com.squareup.picasso.Transformation;

import java.util.WeakHashMap;

public class PaletteGeneratorTransformation implements Transformation {
    private static final PaletteGeneratorTransformation INSTANCE =
            new PaletteGeneratorTransformation();
    private static final WeakHashMap<Bitmap, Palette> CACHE = new WeakHashMap<>();

    public static PaletteGeneratorTransformation getInstance() {
        return INSTANCE;
    }

    private PaletteGeneratorTransformation() {
    }

    @Override
    public Bitmap transform(Bitmap source) {
        Palette palette = Palette.from(source).generate();
        CACHE.put(source, palette);
        return source;
    }

    @Override
    public String key() {
        return getClass().getCanonicalName();
    }

    public Palette getPalette(Bitmap bitmap) {
        return CACHE.get(bitmap);
    }
}
