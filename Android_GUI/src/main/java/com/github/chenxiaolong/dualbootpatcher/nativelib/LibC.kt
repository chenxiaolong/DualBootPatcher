/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

import com.sun.jna.Native
import com.sun.jna.Pointer

// NOTE: Almost no checking of parameters is performed on both the Java and C side of this native
//       wrapper. As a rule of thumb, don't pass null to any function.

object LibC {
    object CWrapper {
        init {
            Native.register(CWrapper::class.java, "c")
        }

        @JvmStatic
        external fun free(ptr: Pointer)

        @JvmStatic
        external fun getpid(): Int /* pid_t */
    }

    fun free(ptr: Pointer) {
        CWrapper.free(ptr)
    }

    fun freeArray(ptr: Pointer?) {
        if (ptr != null) {
            for (p in ptr.getPointerArray(0)) {
                free(p)
            }
            free(ptr)
        }
    }

    fun getStringAndFree(p: Pointer): String {
        val str = p.getString(0)
        free(p)
        return str
    }

    fun getStringArrayAndFree(p: Pointer): Array<String> {
        val array = p.getStringArray(0)
        freeArray(p)
        return array
    }
}
