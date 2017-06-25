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

#include "emergency.h"

#include <algorithm>

#include <cerrno>
#include <cstring>

#include <sys/klog.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mbdevice/json.h"
#include "mblog/logging.h"
#include "mbutil/autoclose/file.h"
#include "mbutil/directory.h"
#include "mbutil/file.h"
#include "mbutil/finally.h"
#include "mbutil/fts.h"
#include "mbutil/mount.h"
#include "mbutil/time.h"
#include "mbutil/vibrate.h"

#include "multiboot.h"
#include "reboot.h"

namespace mb
{

class BlockDevFinder : public util::FTSWrapper {
public:
    BlockDevFinder(std::string path, std::vector<std::string> names)
        : FTSWrapper(path, 0),
        _names(std::move(names))
    {
    }

    virtual int on_reached_symlink() override
    {
        struct stat sb;

        if (std::find(_names.begin(), _names.end(), _curr->fts_name)
                != _names.end()
                && stat(_curr->fts_accpath, &sb) == 0
                && S_ISBLK(sb.st_mode)) {
            _results.push_back(_curr->fts_path);
        }

        return Action::FTS_OK;
    }

    virtual int on_reached_block_device() override
    {
        if (std::find(_names.begin(), _names.end(), _curr->fts_name)
                != _names.end()) {
            _results.push_back(_curr->fts_path);
        }

        return Action::FTS_OK;
    }

    const std::vector<std::string> & results() const
    {
        return _results;
    }

private:
    std::vector<std::string> _names;
    std::vector<std::string> _results;
};

static bool dump_kernel_log(const char *file)
{
    int len = klogctl(KLOG_SIZE_BUFFER, nullptr, 0);
    if (len < 0) {
        LOGE("Failed to get kernel log buffer size: %s", strerror(errno));
        return false;
    }

    char *buf = (char *) malloc(len);
    if (!buf) {
        LOGE("Failed to allocate %d bytes: %s", len, strerror(errno));
        return false;
    }

    auto free_buf = util::finally([&]{
        free(buf);
    });

    len = klogctl(KLOG_READ_ALL, buf, len);
    if (len < 0) {
        LOGE("Failed to read kernel log buffer: %s", strerror(errno));
        return false;
    }

    autoclose::file fp(autoclose::fopen(file, "wb"));
    if (!fp) {
        LOGE("%s: Failed to open for writing: %s", file, strerror(errno));
        return false;
    }

    std::string timestamp = util::format_time("%Y/%m/%d %H:%M:%S %Z\n");
    if (fwrite(timestamp.data(), timestamp.length(), 1, fp.get()) != 1) {
        LOGE("%s: Failed to write timestamp: %s", file, strerror(errno));
        return false;
    }

    if (len > 0) {
        if (fwrite(buf, len, 1, fp.get()) != 1) {
            LOGE("%s: Failed to write data: %s", file, strerror(errno));
            return false;
        }
        if (buf[len - 1] != '\n') {
            if (fputc('\n', fp.get()) == EOF) {
                LOGE("%s: Failed to write data: %s", file, strerror(errno));
                return false;
            }
        }
    }

    return true;
}

struct EmergencyMount
{
    std::string mount_point;
    std::string log_path;
    std::vector<std::string> paths;
};

bool emergency_reboot()
{
    util::vibrate(100, 150);
    util::vibrate(100, 150);
    util::vibrate(100, 150);
    util::vibrate(100, 150);
    util::vibrate(100, 150);

    LOGW("--- EMERGENCY REBOOT FROM MBTOOL ---");

    std::vector<EmergencyMount> ems;
    MbDeviceJsonError error;

    std::vector<unsigned char> contents;
    util::file_read_all(DEVICE_JSON_PATH, &contents);
    contents.push_back('\0');

    Device *device = mb_device_new_from_json(
            (char *) contents.data(), &error);
    auto free_device = util::finally([&]{
        mb_device_free(device);
    });

    // /data
    {
        EmergencyMount em;
        BlockDevFinder finder("/dev/block", {
            "data", "DATA", "userdata", "USERDATA", "UDA"
        });

        em.mount_point = "/raw/data";
        em.log_path = "media/0/MultiBoot/logs/kmsg.log";

        LOGV("Searching for data partition block device paths");

        if (device) {
            auto devs = mb_device_data_block_devs(device);
            if (devs) {
                for (auto it = devs; *it; ++it) {
                    LOGV("- %s", *it);
                    em.paths.push_back(*it);
                }
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

        if (device) {
            auto devs = mb_device_cache_block_devs(device);
            if (devs) {
                for (auto it = devs; *it; ++it) {
                    LOGV("- %s", *it);
                    em.paths.push_back(*it);
                }
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
        if (!util::mkdir_recursive(em.mount_point, 0755) && errno != EEXIST) {
            LOGW("%s: Failed to create directory: %s",
                 em.mount_point.c_str(), strerror(errno));
        }

        if (!util::is_mounted(em.mount_point)) {
            for (const std::string &path : em.paths) {
                if (util::mount(path.c_str(), em.mount_point.c_str(),
                                "auto", 0, "")) {
                    LOGV("%s: Mounted %s",
                         em.mount_point.c_str(), path.c_str());
                    break;
                } else {
                    LOGW("%s: Failed to mount %s: %s",
                         em.mount_point.c_str(), path.c_str(), strerror(errno));
                }
            }
        }

        std::string log_path(em.mount_point);
        log_path += "/";
        log_path += em.log_path;
        std::string log_path_old(log_path);
        log_path_old += ".old";

        if (!util::mkdir_parent(log_path, 0755) && errno != EEXIST) {
            LOGW("%s: Failed to create parent directory: %s",
                 log_path.c_str(), strerror(errno));
        }

        LOGI("Dumping kernel log to %s", log_path.c_str());

        rename(log_path.c_str(), log_path_old.c_str());
        dump_kernel_log(log_path.c_str());
        sync();

        util::umount(em.mount_point.c_str());
    }

    fix_multiboot_permissions();

    // Does not return if successful
    reboot_directly("recovery");

    return false;
}

}
