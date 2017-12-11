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

package com.github.chenxiaolong.dualbootpatcher.nativelib;

import com.github.chenxiaolong.dualbootpatcher.nativelib.libmiscstuff.LibMiscStuff;
import com.sun.jna.Native;

@SuppressWarnings("unused")
public class LibMbCommon {
    @SuppressWarnings("JniMissingFunction")
    private static class CWrapper {
        static {
            Native.register(CWrapper.class, "mbcommon");
            LibMiscStuff.mblogSetLogcat();
        }

        // BEGIN: mbcommon/version.h
        static native String mb_version();
        static native String mb_git_version();
        // END: mbcommon/version.h
    }

    public static String getVersion() {
        return CWrapper.mb_version();
    }

    public static String getGitVersion() {
        return CWrapper.mb_git_version();
    }
}
