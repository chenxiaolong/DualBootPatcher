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

#ifndef LIBMOUNTPOINT_H
#define LIBMOUNTPOINT_H

#include <string>
#include <vector>

#define MOUNTS "/proc/mounts"

std::vector<std::string> get_mount_points();
std::string get_mnt_fsname(std::string mountpoint);
std::string get_mnt_type(std::string mountpoint);
std::string get_mnt_opts(std::string mountpoint);
int get_mnt_freq(std::string mountpoint);
int get_mnt_passno(std::string mountpoint);

#endif