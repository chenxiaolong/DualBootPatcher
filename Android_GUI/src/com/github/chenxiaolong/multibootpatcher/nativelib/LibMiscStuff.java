/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

package com.github.chenxiaolong.multibootpatcher.nativelib;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;

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
    }

    public static CLibrary INSTANCE = (CLibrary) Native.loadLibrary("miscstuff", CLibrary.class);
}
