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

package com.github.chenxiaolong.dualbootpatcher.nativelib

import com.github.chenxiaolong.dualbootpatcher.nativelib.libmiscstuff.LibMiscStuff
import com.sun.jna.Native

object LibMbCommon {
    val version: String
        get() = CWrapper.mb_version()

    val gitVersion: String
        get() = CWrapper.mb_git_version()

    private object CWrapper {
        init {
            Native.register(CWrapper::class.java, "mbcommon")
            LibMiscStuff.mblogSetLogcat()
        }

        // BEGIN: mbcommon/version.h
        @JvmStatic
        external fun mb_version(): String

        @JvmStatic
        external fun mb_git_version(): String
        // END: mbcommon/version.h
    }
}
