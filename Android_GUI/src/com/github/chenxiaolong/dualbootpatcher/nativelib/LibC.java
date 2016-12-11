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

import com.sun.jna.LastErrorException;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

import java.util.Arrays;
import java.util.List;

// NOTE: Almost no checking of parameters is performed on both the Java and C side of this native
//       wrapper. As a rule of thumb, don't pass null to any function.

@SuppressWarnings("JniMissingFunction")
public class LibC {
    @SuppressWarnings({"WeakerAccess", "unused"})
    public static class CWrapper {
        static {
            Native.register(CWrapper.class, "c");
        }

        public static class mntent extends Structure {
            public String mnt_fsname;
            public String mnt_dir;
            public String mnt_type;
            public String mnt_opts;
            public int mnt_freq;
            public int mnt_passno;

            @Override
            protected List getFieldOrder() {
                return Arrays.asList("mnt_fsname", "mnt_dir", "mnt_type", "mnt_opts", "mnt_freq",
                        "mnt_passno");
            }
        }

        public static native void free(Pointer ptr);

        public static native /* pid_t */ int getpid();

        // bionic does not provide setmntent
        public static native Pointer fopen(String file, String mode) throws LastErrorException;

        // bionic does not provide endmntent
        public static native Pointer fclose(Pointer stream) throws LastErrorException;

        public static native mntent getmntent(Pointer stream);
    }
}
