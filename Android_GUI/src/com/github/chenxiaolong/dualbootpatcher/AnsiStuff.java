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

package com.github.chenxiaolong.dualbootpatcher;

import java.util.EnumSet;
import java.util.HashMap;

public class AnsiStuff {
    private static final String ANSI_ESCAPE = "\u001b";
    private static final String RESET = "0";

    private static HashMap<Color, String> mFgColorMap = new HashMap<>();
    private static HashMap<Color, String> mBgColorMap = new HashMap<>();
    private static HashMap<Attribute, String> mAttrMap = new HashMap<>();

    static {
        mFgColorMap.put(Color.BLACK, "30");
        mFgColorMap.put(Color.RED, "31");
        mFgColorMap.put(Color.GREEN, "32");
        mFgColorMap.put(Color.YELLOW, "33");
        mFgColorMap.put(Color.BLUE, "34");
        mFgColorMap.put(Color.MAGENTA, "35");
        mFgColorMap.put(Color.CYAN, "36");
        mFgColorMap.put(Color.WHITE, "37");

        mBgColorMap.put(Color.BLACK, "40");
        mBgColorMap.put(Color.RED, "41");
        mBgColorMap.put(Color.GREEN, "42");
        mBgColorMap.put(Color.YELLOW, "43");
        mBgColorMap.put(Color.BLUE, "44");
        mBgColorMap.put(Color.MAGENTA, "45");
        mBgColorMap.put(Color.CYAN, "46");
        mBgColorMap.put(Color.WHITE, "47");

        mAttrMap.put(Attribute.BOLD, "1");
        mAttrMap.put(Attribute.UNDERSCORE, "4");
        mAttrMap.put(Attribute.BLINK, "5");
    }

    public enum Color {
        BLACK,
        RED,
        GREEN,
        YELLOW,
        BLUE,
        MAGENTA,
        CYAN,
        WHITE
    }

    public enum Attribute {
        BOLD,
        UNDERSCORE,
        BLINK
    }

    public static String format(String text, EnumSet<Color> fg, EnumSet<Color> bg,
                                EnumSet<Attribute> attrs) {
        // Trivial case
        if ((fg == null || fg.isEmpty())
                && (bg == null || bg.isEmpty())
                && (attrs == null || attrs.isEmpty())) {
            return text;
        }

        StringBuilder sb = new StringBuilder();
        sb.append(ANSI_ESCAPE);
        sb.append('[');

        boolean first = true;

        if (fg != null && !fg.isEmpty()) {
            for (Color color : fg) {
                if (!first) {
                    sb.append(';');
                }
                first = false;
                sb.append(mFgColorMap.get(color));
            }
        }
        if (bg != null && !bg.isEmpty()) {
            for (Color color : bg) {
                if (!first) {
                    sb.append(';');
                }
                first = false;
                sb.append(mBgColorMap.get(color));
            }
        }
        if (attrs != null && !attrs.isEmpty()) {
            for (Attribute attr : attrs) {
                if (!first) {
                    sb.append(';');
                }
                first = false;
                sb.append(mAttrMap.get(attr));
            }
        }

        sb.append('m');

        // Actual text
        sb.append(text);

        sb.append(ANSI_ESCAPE);
        sb.append('[');
        sb.append(RESET);
        sb.append('m');

        return sb.toString();
    }
}
