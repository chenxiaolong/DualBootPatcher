/*
 * Copyright (C) 2018-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

package com.github.chenxiaolong.dualbootpatcher.freespace

import android.os.Build
import android.os.StatFs
import android.util.Log
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModel
import com.github.chenxiaolong.dualbootpatcher.FileUtils
import com.github.chenxiaolong.dualbootpatcher.FileUtils.MountEntry
import java.io.IOException
import java.util.*

internal class FreeSpaceViewModel : ViewModel() {
    private lateinit var _mounts: MutableLiveData<List<MountInfo>>

    val mounts: LiveData<List<MountInfo>>
        get() {
            if (!::_mounts.isInitialized) {
                _mounts = MutableLiveData()
                loadMounts()
            }
            return _mounts
        }

    private fun loadMounts() {
        val entries: Array<MountEntry>

        try {
            entries = FileUtils.mounts
        } catch (e: IOException) {
            Log.e(TAG, "Failed to get mount entries", e)
            return
        }

        val mounts = ArrayList<MountInfo>()

        for (entry in entries) {
            // Ignore irrelevant filesystems
            if (SKIPPED_FS_TYPES.contains(entry.mnt_type)
                    || SKIPPED_FS_NAMES.contains(entry.mnt_fsname)
                    || entry.mnt_dir.startsWith("/mnt")
                    || entry.mnt_dir.startsWith("/dev")
                    || entry.mnt_dir.startsWith("/proc")
                    || entry.mnt_dir.startsWith("/data/data")) {
                continue
            }

            val statFs: StatFs

            try {
                statFs = StatFs(entry.mnt_dir)
            } catch (e: IllegalArgumentException) {
                // Thrown if Os.statvfs() throws ErrnoException
                Log.w(TAG, "Exception during statfs of ${entry.mnt_dir}", e)
                continue
            }

            val totalSpace: Long
            val availSpace: Long

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
                totalSpace = statFs.blockSizeLong * statFs.blockCountLong
                availSpace = statFs.blockSizeLong * statFs.availableBlocksLong
            } else {
                @Suppress("DEPRECATION")
                totalSpace = (statFs.blockSize * statFs.blockCount).toLong()
                @Suppress("DEPRECATION")
                availSpace = (statFs.blockSize * statFs.availableBlocks).toLong()
            }

            mounts.add(MountInfo(entry.mnt_dir, entry.mnt_fsname, entry.mnt_type, totalSpace,
                    availSpace))
        }

        _mounts.value = mounts
    }

    fun refresh() {
        loadMounts()
    }

    companion object {
        private val TAG = FreeSpaceViewModel::class.java.simpleName

        private val SKIPPED_FS_TYPES = HashSet<String>()
        private val SKIPPED_FS_NAMES = HashSet<String>()

        init {
            SKIPPED_FS_TYPES.add("cgroup")
            SKIPPED_FS_TYPES.add("debugfs")
            SKIPPED_FS_TYPES.add("devpts")
            SKIPPED_FS_TYPES.add("proc")
            SKIPPED_FS_TYPES.add("pstore")
            SKIPPED_FS_TYPES.add("rootfs")
            SKIPPED_FS_TYPES.add("selinuxfs")
            SKIPPED_FS_TYPES.add("sysfs")
            SKIPPED_FS_TYPES.add("tmpfs")
            SKIPPED_FS_TYPES.add("tracefs")

            SKIPPED_FS_NAMES.add("debugfs")
            SKIPPED_FS_NAMES.add("devpts")
            SKIPPED_FS_NAMES.add("none")
            SKIPPED_FS_NAMES.add("proc")
            SKIPPED_FS_NAMES.add("pstore")
            SKIPPED_FS_NAMES.add("rootfs")
            SKIPPED_FS_NAMES.add("selinuxfs")
            SKIPPED_FS_NAMES.add("sysfs")
            SKIPPED_FS_NAMES.add("tmpfs")
            SKIPPED_FS_NAMES.add("tracefs")
        }
    }
}
