/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "boot/emergency.h"

#include <algorithm>

#include <cerrno>
#include <cstring>

#include <sys/klog.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mbcommon/error_code.h"
#include "mbcommon/finally.h"
#include "mbcommon/outcome.h"
#include "mbdevice/json.h"
#include "mblog/logging.h"
#include "mbutil/directory.h"
#include "mbutil/file.h"
#include "mbutil/fts.h"
#include "mbutil/mount.h"
#include "mbutil/reboot.h"
#include "mbutil/time.h"
#include "mbutil/vibrate.h"

#include "util/multiboot.h"

#define LOG_TAG "mbtool/boot/emergency"

using namespace mb::device;

namespace mb
{

using ScopedFILE = std::unique_ptr<FILE, decltype(fclose) *>;

class BlockDevFinder : public util::FtsWrapper {
public:
    BlockDevFinder(std::string path, std::vector<std::string> names)
        : FtsWrapper(path, 0)
        , _names(std::move(names))
    {
    }

    Actions on_reached_symlink() override
    {
        struct stat sb;

        if (std::find(_names.begin(), _names.end(), _curr->fts_name)
                != _names.end()
                && stat(_curr->fts_accpath, &sb) == 0
                && S_ISBLK(sb.st_mode)) {
            _results.push_back(_curr->fts_path);
        }

        return Action::Ok;
    }

    Actions on_reached_block_device() override
    {
        if (std::find(_names.begin(), _names.end(), _curr->fts_name)
                != _names.end()) {
            _results.push_back(_curr->fts_path);
        }

        return Action::Ok;
    }

    const std::vector<std::string> & results() const
    {
        return _results;
    }

private:
    std::vector<std::string> _names;
    std::vector<std::string> _results;
};

static oc::result<void> dump_kernel_log(const char *file)
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

    ScopedFILE fp(fopen(file, "wbe"), fclose);
    if (!fp) {
        return ec_from_errno();
    }

    auto timestamp = util::format_time("%Y/%m/%d %H:%M:%S %Z\n",
                                       std::chrono::system_clock::now());
    if (timestamp && fwrite(timestamp.value().data(),
            timestamp.value().length(), 1, fp.get()) != 1) {
        return ec_from_errno();
    }

    if (len > 0) {
        if (fwrite(buf.data(), static_cast<size_t>(len), 1, fp.get()) != 1) {
            return ec_from_errno();
        }
        if (buf[static_cast<size_t>(len - 1)] != '\n') {
            if (fputc('\n', fp.get()) == EOF) {
                return ec_from_errno();
            }
        }
    }

    return oc::success();
}

struct EmergencyMount
{
    std::string mount_point;
    std::string log_path;
    std::vector<std::string> paths;
};

bool emergency_reboot()
{
    using namespace std::chrono_literals;

    (void) util::vibrate(100ms, 250ms);
    (void) util::vibrate(100ms, 250ms);
    (void) util::vibrate(100ms, 250ms);
    (void) util::vibrate(100ms, 250ms);
    (void) util::vibrate(100ms, 250ms);

    LOGW("--- EMERGENCY REBOOT FROM MBTOOL ---");

    std::vector<EmergencyMount> ems;
    Device device;
    JsonError error;
    bool loaded_json = false;

    auto contents = util::file_read_all(DEVICE_JSON_PATH);
    if (contents) {
        loaded_json = device_from_json(contents.value(), device, error);
    }

    // /data
    {
        EmergencyMount em;
        BlockDevFinder finder("/dev/block", {
            "data", "DATA", "userdata", "USERDATA", "UDA"
        });

        em.mount_point = "/raw/data";
        em.log_path = "media/0/MultiBoot/logs/kmsg.log";

        LOGV("Searching for data partition block device paths");

        if (loaded_json) {
            for (auto const &path : device.data_block_devs()) {
                LOGV("- %s", path.c_str());
                em.paths.push_back(path);
            }
        }

        finder.run();
        for (auto const &path : finder.results()) {
            LOGV("- %s", path.c_str());
            em.paths.push_back(path);
        }

        ems.push_back(std::move(em));
    }

    // /cache
    {
        EmergencyMount em;
        BlockDevFinder finder("/dev/block", {
            "cache", "CACHE", "CAC"
        });

        em.mount_point = "/raw/cache";
        em.log_path = "multiboot/logs/kmsg.log";

        LOGV("Searching for cache partition block device paths");

        if (loaded_json) {
            for (auto const &path : device.cache_block_devs()) {
                LOGV("- %s", path.c_str());
                em.paths.push_back(path);
            }
        }

        finder.run();
        for (auto const &path : finder.results()) {
            LOGV("- %s", path.c_str());
            em.paths.push_back(path);
        }

        ems.push_back(std::move(em));
    }

    for (auto const &em : ems) {
        if (auto r = util::mkdir_recursive(em.mount_point, 0755);
                !r && r.error() != std::errc::file_exists) {
            LOGW("%s: Failed to create directory: %s",
                 em.mount_point.c_str(), r.error().message().c_str());
        }

        if (!util::is_mounted(em.mount_point)) {
            for (const std::string &path : em.paths) {
                if (auto ret = util::mount(path, em.mount_point, "auto", 0, "")) {
                    LOGV("%s: Mounted %s",
                         em.mount_point.c_str(), path.c_str());
                    break;
                } else {
                    LOGW("%s: Failed to mount %s: %s",
                         em.mount_point.c_str(), path.c_str(),
                         ret.error().message().c_str());
                }
            }
        }

        std::string log_path(em.mount_point);
        log_path += "/";
        log_path += em.log_path;
        std::string log_path_old(log_path);
        log_path_old += ".old";

        if (auto r = util::mkdir_parent(log_path, 0755);
                !r && r.error() != std::errc::file_exists) {
            LOGW("%s: Failed to create parent directory: %s",
                 log_path.c_str(), strerror(errno));
        }

        LOGI("Dumping kernel log to %s", log_path.c_str());

        rename(log_path.c_str(), log_path_old.c_str());
        if (auto ret = dump_kernel_log(log_path.c_str()); !ret) {
            LOGW("Failed to dump kernel log: %s",
                 ret.error().message().c_str());
        }
        sync();

        (void) util::umount(em.mount_point);
    }

    fix_multiboot_permissions();

    // Does not return if successful
    util::reboot_via_syscall("recovery");

    return false;
}

}
