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

package com.github.chenxiaolong.dualbootpatcher

import java.util.EnumSet
import java.util.HashMap

object AnsiStuff {
    private const val ANSI_ESCAPE = "\u001b"
    private const val RESET = "0"

    private val fgColorMap = HashMap<Color, String>()
    private val bgColorMap = HashMap<Color, String>()
    private val attrMap = HashMap<Attribute, String>()

    init {
        fgColorMap[Color.BLACK] = "30"
        fgColorMap[Color.RED] = "31"
        fgColorMap[Color.GREEN] = "32"
        fgColorMap[Color.YELLOW] = "33"
        fgColorMap[Color.BLUE] = "34"
        fgColorMap[Color.MAGENTA] = "35"
        fgColorMap[Color.CYAN] = "36"
        fgColorMap[Color.WHITE] = "37"

        bgColorMap[Color.BLACK] = "40"
        bgColorMap[Color.RED] = "41"
        bgColorMap[Color.GREEN] = "42"
        bgColorMap[Color.YELLOW] = "43"
        bgColorMap[Color.BLUE] = "44"
        bgColorMap[Color.MAGENTA] = "45"
        bgColorMap[Color.CYAN] = "46"
        bgColorMap[Color.WHITE] = "47"

        attrMap[Attribute.BOLD] = "1"
        attrMap[Attribute.UNDERSCORE] = "4"
        attrMap[Attribute.BLINK] = "5"
    }

    enum class Color {
        BLACK,
        RED,
        GREEN,
        YELLOW,
        BLUE,
        MAGENTA,
        CYAN,
        WHITE
    }

    enum class Attribute {
        BOLD,
        UNDERSCORE,
        BLINK
    }

    fun format(text: String, fg: EnumSet<Color>?, bg: EnumSet<Color>?,
               attrs: EnumSet<Attribute>?): String {
        // Trivial case
        if ((fg == null || fg.isEmpty())
                && (bg == null || bg.isEmpty())
                && (attrs == null || attrs.isEmpty())) {
            return text
        }

        val sb = StringBuilder()
        sb.append(ANSI_ESCAPE)
        sb.append('[')

        var first = true

        if (fg != null && !fg.isEmpty()) {
            for (color in fg) {
                if (!first) {
                    sb.append(';')
                }
                first = false
                sb.append(fgColorMap[color])
            }
        }
        if (bg != null && !bg.isEmpty()) {
            for (color in bg) {
                if (!first) {
                    sb.append(';')
                }
                first = false
                sb.append(bgColorMap[color])
            }
        }
        if (attrs != null && !attrs.isEmpty()) {
            for (attr in attrs) {
                if (!first) {
                    sb.append(';')
                }
                first = false
                sb.append(attrMap[attr])
            }
        }

        sb.append('m')

        // Actual text
        sb.append(text)

        sb.append(ANSI_ESCAPE)
        sb.append('[')
        sb.append(RESET)
        sb.append('m')

        return sb.toString()
    }
}