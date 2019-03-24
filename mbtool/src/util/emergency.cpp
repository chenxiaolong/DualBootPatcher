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

#include "util/emergency.h"

#include <cerrno>
#include <cstring>

#include <sys/klog.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/error_code.h"
#include "mbcommon/file/standard.h"
#include "mbcommon/file_util.h"
#include "mbcommon/finally.h"
#include "mbcommon/outcome.h"

#include "mblog/logging.h"

#include "mbutil/directory.h"
#include "mbutil/file.h"
#include "mbutil/mount.h"
#include "mbutil/reboot.h"
#include "mbutil/time.h"
#include "mbutil/vibrate.h"

#include "util/config.h"
#include "util/multiboot.h"
#include "util/uevent.h"

#define LOG_TAG "mbtool/main/emergency"

#define EMERGENCY_BLOCK_DEV     "/dbp.emerg.blk"
#define EMERGENCY_MOUNT_POINT   "/dbp.emerg.mnt"

namespace mb
{

static oc::result<void> dump_kernel_log(const char *path)
{
    int len = klogctl(KLOG_SIZE_BUFFER, nullptr, 0);
    if (len < 0) {
        return ec_from_errno();
    }

    std::vector<char> buf(static_cast<size_t>(len));

    len = klogctl(KLOG_READ_ALL, buf.data(), static_cast<int>(buf.size()));
    if (len < 0) {
        return ec_from_errno();
    }

    auto s = as_uchars(span(buf).subspan(static_cast<size_t>(len)));
    StandardFile file;

    OUTCOME_TRYV(file.open(path, FileOpenMode::WriteOnly));

    if (auto t = util::format_time("%Y/%m/%d %H:%M:%S %Z\n",
                                   std::chrono::system_clock::now())) {
        OUTCOME_TRYV(file_write_exact(file, as_uchars(span(t.value()))));
    }

    if (len > 0) {
        OUTCOME_TRYV(file_write_exact(file, s));

        if (s.back() != '\n') {
            OUTCOME_TRYV(file_write_exact(file, "\n"_uchars));
        }
    }

    return file.close();
}

static bool persist_logs()
{
    auto config = load_shim_config();
    if (!config) {
        LOGE("Failed to load shim config. Cannot persist logs");
        return false;
    }

    if (!create_block_dev(config->uevent_path.c_str(), EMERGENCY_BLOCK_DEV)) {
        return false;
    }

    if (mkdir(EMERGENCY_MOUNT_POINT, 0755) < 0 && errno != EEXIST) {
        LOGE("%s: Failed to create directory: %s",
             EMERGENCY_MOUNT_POINT, strerror(errno));
        return false;
    }

    auto delete_paths = finally([&] {
        rmdir(EMERGENCY_MOUNT_POINT);
        unlink(EMERGENCY_BLOCK_DEV);
    });

    if (auto r = util::mount(EMERGENCY_BLOCK_DEV, EMERGENCY_MOUNT_POINT,
                             "auto", 0, ""); !r) {
        LOGE("%s: Failed to mount %s: %s",
             EMERGENCY_MOUNT_POINT, EMERGENCY_BLOCK_DEV,
             r.error().message().c_str());
        return false;
    }

    auto unmount_path = finally([&] {
        sync();
        (void) util::umount(EMERGENCY_MOUNT_POINT);
    });

    std::string log_path(EMERGENCY_MOUNT_POINT);
    log_path += "/multiboot/logs/kmsg.log";
    std::string log_path_old(log_path);
    log_path_old += ".old";

    if (auto r = util::mkdir_parent(log_path, 0755); !r) {
        LOGE("%s: Failed to create parent directory: %s",
             log_path.c_str(), strerror(errno));
        return false;
    }

    LOGI("Dumping kernel log to %s", log_path.c_str());

    rename(log_path.c_str(), log_path_old.c_str());

    if (auto r = dump_kernel_log(log_path.c_str()); !r) {
        LOGW("Failed to dump kernel log: %s", r.error().message().c_str());
        return false;
    }

    return true;
}

void emergency_reboot()
{
    using namespace std::chrono_literals;

    LOGW("--- EMERGENCY REBOOT FROM MBTOOL ---");

    if (!util::is_mounted("/sys")) {
        mkdir("/sys", 0755);
        mount("sysfs", "/sys", "sysfs", 0, nullptr);
    }

    for (int i = 0; i < 5; ++i) {
        (void) util::vibrate(100ms, 250ms);
    }

    persist_logs();

    // Does not return if successful
    util::reboot_via_syscall("recovery");

    abort();
}

}
