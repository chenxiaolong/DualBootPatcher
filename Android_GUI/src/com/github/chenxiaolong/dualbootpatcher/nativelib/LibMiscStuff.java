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

package com.github.chenxiaolong.dualbootpatcher.nativelib;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.ptr.IntByReference;

import java.util.Arrays;
import java.util.List;

public class LibMiscStuff {
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

    public interface CLibrary extends Library {
        Pointer setmntent(String file, String mode);
        mntent getmntent(Pointer stream);
        int endmntent(Pointer stream);

        long get_mnt_total_size(String mountpoint);
        long get_mnt_avail_size(String mountpoint);

        boolean is_same_file(String path1, String path2);

        boolean extract_archive(String filename, String target);

        boolean find_string_in_file(String path, String str, IntByReference result);

        void mblog_set_logcat();

        int get_pid();

        Pointer read_link(String path);
    }

    public static final CLibrary INSTANCE =
            (CLibrary) Native.loadLibrary("miscstuff", CLibrary.class);

    public static String readLink(String path) {
        Pointer p = INSTANCE.read_link(path);
        if (p == null) {
            return null;
        }

        String result = p.getString(0);
        Native.free(Pointer.nativeValue(p));
        return result;
    }
}
