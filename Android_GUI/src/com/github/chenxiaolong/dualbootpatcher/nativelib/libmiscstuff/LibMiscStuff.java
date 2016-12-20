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

package com.github.chenxiaolong.dualbootpatcher.nativelib.libmiscstuff;

import java.io.IOException;

@SuppressWarnings("JniMissingFunction")
public final class LibMiscStuff {
    private LibMiscStuff() {
    }

    public static native void extractArchive(String filename, String target) throws IOException;

    public static native boolean findStringInFile(String path, String jstr) throws IOException;

    public static native void mblogSetLogcat();

    public static native String readLink(String path) throws IOException;

    static {
        System.loadLibrary("miscstuff-jni");
    }
}