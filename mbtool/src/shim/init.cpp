/*
 * Copyright (C) 2015-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "shim/init.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/finally.h"
#include "mbcommon/string.h"
#include "mbcommon/version.h"

#include "mblog/logging.h"

#include "mbsign/sign.h"

#include "mbutil/archive.h"
#include "mbutil/mount.h"

#include "util/config.h"
#include "util/emergency.h"
#include "util/kmsg.h"
#include "util/signature.h"
#include "util/uevent.h"

#define LOG_TAG     "mbtool/shim/init"

#define DBP_ROOT    "/dbp"
#define DBP_MBTOOL  DBP_ROOT "/mbtool"

#define MOUNT_POINT "/dbp.shim.mnt"
#define BLOCK_DEV   "/dbp.shim.blk"

namespace mb
{

//! Extract DBP archive
static bool extract_dbp(const char *block_dev, const char *mount_point,
                        const char *archive_path, const char *target_dir)
{
    if (mkdir(mount_point, 0700) < 0) {
        LOGE("%s: Failed to create directory: %s",
             mount_point, strerror(errno));
        return false;
    }

    auto delete_mnt = finally([&] {
        rmdir(mount_point);
    });

    if (auto r = util::mount(
            block_dev, mount_point, "auto", MS_RDONLY, ""); !r) {
        LOGE("%s: Failed to mount %s: %s",
             mount_point, BLOCK_DEV, r.error().message().c_str());
        return false;
    }

    auto unmount_mnt = finally([&] {
        umount(mount_point);
    });

    std::string archive(mount_point);
    archive += "/";
    archive += archive_path;

    if (!util::extract_archive(archive, target_dir)) {
        LOGE("%s: Failed to extract archive", archive.c_str());
        return false;
    }

    return true;
}

static void stage_1()
{
    LOGV("mbtool-shim version %s (%s)", version(), git_version());

    auto config = load_shim_config();
    if (!config) {
        return;
    }

    {
        if (!create_block_dev(config->uevent_path.c_str(), BLOCK_DEV)) {
            return;
        }

        auto delete_block_dev = finally([&] {
            unlink(BLOCK_DEV);
        });

        if (!extract_dbp(BLOCK_DEV, MOUNT_POINT, config->archive_path.c_str(),
                         DBP_ROOT)) {
            return;
        }
    }

    if (!sign::initialize()) {
        LOGE("Failed to initialize libmbsign");
        return;
    }

    auto trusted = verify_signature(DBP_MBTOOL, nullptr, nullptr);
    if (!trusted) {
        LOGE("%s: Failed to verify signature: %s",
             DBP_MBTOOL, trusted.error().message().c_str());
        return;
    }

    auto sig_version = trusted.value()[TRUSTED_PROP_VERSION];

    // Prevent downgrade attack
    if (version_compare(sig_version, version()) < 0) {
        LOGE("%s: mbtool (%s) is older than shim (%s)",
             DBP_MBTOOL, sig_version.c_str(), version());
        return;
    }

    // Execute mbtool stage 2
    LOGV("Launching mbtool stage 2 ...");
    execlp(DBP_MBTOOL, "boot", nullptr);
    LOGV("%s: Failed to exec: %s", DBP_MBTOOL, strerror(errno));
}

int init_main(int argc, char *argv[])
{
    (void) argc;

    if (getppid() != 0) {
        LOGE("%s must be run as PID 1", argv[0]);
        return EXIT_FAILURE;
    }

    // Mount base directories
    mkdir("/dev", 0755);
    mkdir("/proc", 0755);
    mkdir("/sys", 0755);

    mount("tmpfs", "/dev", "tmpfs", MS_NOSUID, "mode=0755");
    mkdir("/dev/pts", 0755);
    mount("devpts", "/dev/pts", "devpts", 0, nullptr);
    mount("proc", "/proc", "proc", 0, nullptr);
    mount("sysfs", "/sys", "sysfs", 0, nullptr);

    set_kmsg_logging();

    stage_1();

    emergency_reboot();
}

}
