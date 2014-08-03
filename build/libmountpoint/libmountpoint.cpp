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

#include <mntent.h>
#include "libmountpoint.h"

#ifdef __ANDROID_API__
#define SETMNTENT(x, y) fopen(x, y)
#define ENDMNTENT(x) fclose(x)
#else
#define SETMNTENT(x, y) setmntent(x, y)
#define ENDMNTENT(x) endmntent(x)
#endif

std::vector<std::string> get_mount_points() {
    std::vector<std::string> mount_points;

    FILE *f;
    struct mntent *ent;

    f = SETMNTENT(MOUNTS, "r");
    if (f != NULL) {
        while ((ent = getmntent(f)) != NULL) {
            mount_points.push_back(ent->mnt_dir);
        }
    }
    ENDMNTENT(f);

    return mount_points;
}

std::string get_mnt_fsname(std::string mountpoint) {
    FILE *f;
    struct mntent *ent;
    std::string fsname;

    f = SETMNTENT(MOUNTS, "r");
    if (f != NULL) {
        while ((ent = getmntent(f)) != NULL) {
            if (mountpoint == ent->mnt_dir) {
                fsname = ent->mnt_fsname;
                break;
            }
        }
    }

    return fsname;
}

std::string get_mnt_type(std::string mountpoint) {
    FILE *f;
    struct mntent *ent;
    std::string mnttype;

    f = SETMNTENT(MOUNTS, "r");
    if (f != NULL) {
        while ((ent = getmntent(f)) != NULL) {
            if (mountpoint == ent->mnt_dir) {
                mnttype = ent->mnt_type;
                break;
            }
        }
    }

    return mnttype;
}

std::string get_mnt_opts(std::string mountpoint) {
    FILE *f;
    struct mntent *ent;
    std::string mntopts;

    f = SETMNTENT(MOUNTS, "r");
    if (f != NULL) {
        while ((ent = getmntent(f)) != NULL) {
            if (mountpoint == ent->mnt_dir) {
                mntopts = ent->mnt_opts;
                break;
            }
        }
    }

    return mntopts;
}

int get_mnt_freq(std::string mountpoint) {
    FILE *f;
    struct mntent *ent;
    int mntfreq = 0;

    f = SETMNTENT(MOUNTS, "r");
    if (f != NULL) {
        while ((ent = getmntent(f)) != NULL) {
            if (mountpoint == ent->mnt_dir) {
                mntfreq = ent->mnt_freq;
                break;
            }
        }
    }

    return mntfreq;
}

int get_mnt_passno(std::string mountpoint) {
    FILE *f;
    struct mntent *ent;
    int mntpassno = 0;

    f = SETMNTENT(MOUNTS, "r");
    if (f != NULL) {
        while ((ent = getmntent(f)) != NULL) {
            if (mountpoint == ent->mnt_dir) {
                mntpassno = ent->mnt_passno;
                break;
            }
        }
    }

    return mntpassno;
}
