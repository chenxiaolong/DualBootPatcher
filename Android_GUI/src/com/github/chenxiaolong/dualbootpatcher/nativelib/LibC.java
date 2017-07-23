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

package com.github.chenxiaolong.dualbootpatcher.nativelib;

import com.sun.jna.Native;
import com.sun.jna.Pointer;

// NOTE: Almost no checking of parameters is performed on both the Java and C side of this native
//       wrapper. As a rule of thumb, don't pass null to any function.

@SuppressWarnings("JniMissingFunction")
public class LibC {
    @SuppressWarnings({"WeakerAccess", "unused"})
    public static class CWrapper {
        static {
            Native.register(CWrapper.class, "c");
        }

        static native void free(Pointer ptr);

        public static native /* pid_t */ int getpid();
    }

    public static void free(Pointer ptr) {
        CWrapper.free(ptr);
    }

    public static void freeArray(Pointer ptr) {
        if (ptr != null) {
            Pointer[] ptrs = ptr.getPointerArray(0);
            for (Pointer p : ptrs) {
                free(p);
            }
            free(ptr);
        }
    }

    public static String getStringAndFree(Pointer p) {
        String str = p.getString(0);
        free(p);
        return str;
    }

    public static String[] getStringArrayAndFree(Pointer p) {
        String[] array = p.getStringArray(0);
        freeArray(p);
        return array;
    }
}
