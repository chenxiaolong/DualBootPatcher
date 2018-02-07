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

package com.github.chenxiaolong.dualbootpatcher.socket.interfaces

class StatBuf {
    var st_dev: Long = 0
    var st_ino: Long = 0
    var st_mode: Int = 0
    var st_nlink: Long = 0
    var st_uid: Int = 0
    var st_gid: Int = 0
    var st_rdev: Long = 0
    var st_size: Long = 0
    var st_blksize: Long = 0
    var st_blocks: Long = 0
    var st_atime: Long = 0
    var st_mtime: Long = 0
    var st_ctime: Long = 0
}