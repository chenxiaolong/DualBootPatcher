/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "libmountpoint.h"

#ifdef __ANDROID_API__
#include "mntent.h"
#include <sys/vfs.h>
#define statvfs statfs
#define fstatvfs fstatfs
#else
#include <mntent.h>
#include <sys/statvfs.h>
#endif

std::vector<std::string> get_mount_points() {
    std::vector<std::string> mount_points;

    FILE *f;
    struct mntent *ent;

    f = setmntent(MOUNTS, "r");
    if (f != NULL) {
        while ((ent = getmntent(f)) != NULL) {
            mount_points.push_back(ent->mnt_dir);
        }
    }
    endmntent(f);

    return mount_points;
}

std::string get_mnt_fsname(std::string mountpoint) {
    FILE *f;
    struct mntent *ent;
    std::string fsname;

    f = setmntent(MOUNTS, "r");
    if (f != NULL) {
        while ((ent = getmntent(f)) != NULL) {
            if (mountpoint == ent->mnt_dir) {
                fsname = ent->mnt_fsname;
                break;
            }
        }
    }
    endmntent(f);

    return fsname;
}

std::string get_mnt_type(std::string mountpoint) {
    FILE *f;
    struct mntent *ent;
    std::string mnttype;

    f = setmntent(MOUNTS, "r");
    if (f != NULL) {
        while ((ent = getmntent(f)) != NULL) {
            if (mountpoint == ent->mnt_dir) {
                mnttype = ent->mnt_type;
                break;
            }
        }
    }
    endmntent(f);

    return mnttype;
}

std::string get_mnt_opts(std::string mountpoint) {
    FILE *f;
    struct mntent *ent;
    std::string mntopts;

    f = setmntent(MOUNTS, "r");
    if (f != NULL) {
        while ((ent = getmntent(f)) != NULL) {
            if (mountpoint == ent->mnt_dir) {
                mntopts = ent->mnt_opts;
                break;
            }
        }
    }
    endmntent(f);

    return mntopts;
}

int get_mnt_freq(std::string mountpoint) {
    FILE *f;
    struct mntent *ent;
    int mntfreq = 0;

    f = setmntent(MOUNTS, "r");
    if (f != NULL) {
        while ((ent = getmntent(f)) != NULL) {
            if (mountpoint == ent->mnt_dir) {
                mntfreq = ent->mnt_freq;
                break;
            }
        }
    }
    endmntent(f);

    return mntfreq;
}

int get_mnt_passno(std::string mountpoint) {
    FILE *f;
    struct mntent *ent;
    int mntpassno = 0;

    f = setmntent(MOUNTS, "r");
    if (f != NULL) {
        while ((ent = getmntent(f)) != NULL) {
            if (mountpoint == ent->mnt_dir) {
                mntpassno = ent->mnt_passno;
                break;
            }
        }
    }
    endmntent(f);

    return mntpassno;
}

unsigned long long get_mnt_total_size(std::string mountpoint) {
    struct statvfs s;

    if (statvfs(mountpoint.c_str(), &s) < 0) {
        return 0;
    }

    return s.f_bsize * s.f_blocks;
}

unsigned long long get_mnt_avail_space(std::string mountpoint) {
    struct statvfs s;

    if (statvfs(mountpoint.c_str(), &s) < 0) {
        return 0;
    }

    return s.f_bsize * s.f_bavail;
}
