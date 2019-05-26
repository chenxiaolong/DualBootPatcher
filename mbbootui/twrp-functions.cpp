/*
 * Copyright 2012 bigbiff/Dees_Troy TeamWin
 * This file is part of TWRP/TeamWin Recovery Project.
 *
 * TWRP is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * TWRP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with TWRP.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "twrp-functions.hpp"

#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/file.h"
#include "mbutil/reboot.h"
#include "mbutil/string.h"
#include "mbutil/time.h"

#include "config/config.hpp"

#include "data.hpp"
#include "variables.h"

#define LOG_TAG "mbbootui/twrp-functions"

std::string TWFunc::get_resource_path(const std::string &res_path)
{
    std::string result;

    if (!tw_resource_path.empty()) {
        result += tw_resource_path;
        if (!result.empty() && result.back() != '/') {
            result += '/';
        }
        if (!res_path.empty() && res_path[0] == '/') {
            result += res_path.substr(1);
        } else {
            result += res_path;
        }
    }

    return result;
}

static std::string current_date_time()
{
    auto str = mb::util::format_time("%Y-%m-%d--%H-%M-%S",
                                     std::chrono::system_clock::now());
    return str ? str.value() : std::string();
}

static bool convertToUint64(const char *str, uint64_t *out)
{
    char *end;
    errno = 0;
    unsigned long long num = strtoull(str, &end, 10);
    if (errno == ERANGE || num > UINT64_MAX
            || *str == '\0' || *end != '\0') {
        return false;
    }
    *out = (uint64_t) num;
    return true;
}

void TWFunc::Fixup_Time_On_Boot()
{
    if (tw_device.tw_flags() & mb::device::TwFlag::QcomRtcFix) {
        LOGI("TWFunc::Fixup_Time: Pre-fix date and time: %s",
             current_date_time().c_str());

        struct timespec ts;
        uint64_t offset = 0;
        std::string sepoch = "/sys/class/rtc/rtc0/since_epoch";

        if (auto r = mb::util::file_first_line(sepoch);
                r && convertToUint64(r.value().c_str(), &offset)) {
            LOGI("TWFunc::Fixup_Time: Setting time offset from file %s", sepoch.c_str());

            ts.tv_sec = offset;
            ts.tv_nsec = 0;
            clock_settime(CLOCK_REALTIME, &ts);

            clock_gettime(CLOCK_REALTIME, &ts);

            if (ts.tv_sec > 1405209403) { // Anything older then 12 Jul 2014 23:56:43 GMT will do nicely thank you ;)
                LOGI("TWFunc::Fixup_Time: Date and time corrected: %s",
                     current_date_time().c_str());
                return;
            }
        } else {
            LOGI("TWFunc::Fixup_Time: opening %s failed: %s",
                 sepoch.c_str(), r.error().message().c_str());
        }

        LOGI("TWFunc::Fixup_Time: will attempt to use the ats files now.");

        // Devices with Qualcomm Snapdragon 800 do some shenanigans with RTC.
        // They never set it, it just ticks forward from 1970-01-01 00:00,
        // and then they have files /data/system/time/ats_* with 64bit offset
        // in miliseconds which, when added to the RTC, gives the correct time.
        // So, the time is: (offset_from_ats + value_from_RTC)
        // There are multiple ats files, they are for different systems? Bases?
        // Like, ats_1 is for modem and ats_2 is for TOD (time of day?).
        // Look at file time_genoff.h in CodeAurora, qcom-opensource/time-services

        static const char *paths[] = {
            "/raw/data/system/time/",
            "/raw/data/time/",
            "/data/system/time/",
            "/data/time/"
        };

        FILE *f;
        DIR *d;
        offset = 0;
        struct dirent *dt;
        std::string ats_path;

        // Prefer ats_2, it seems to be the one we want according to logcat on hammerhead
        // - it is the one for ATS_TOD (time of day?).
        // However, I never saw a device where the offset differs between ats files.
        for (size_t i = 0; i < (sizeof(paths)/sizeof(paths[0])); ++i) {
            d = opendir(paths[i]);
            if (!d) {
                continue;
            }

            while ((dt = readdir(d))) {
                if (dt->d_type != DT_REG || strncmp(dt->d_name, "ats_", 4) != 0) {
                    continue;
                }

                if (ats_path.empty() || strcmp(dt->d_name, "ats_2") == 0) {
                    ats_path = std::string(paths[i]).append(dt->d_name);
                }
            }

            closedir(d);
        }

        if (ats_path.empty()) {
            LOGI("TWFunc::Fixup_Time: no ats files found, leaving untouched!");
            return;
        }

        f = fopen(ats_path.c_str(), "re");
        if (!f) {
            LOGI("TWFunc::Fixup_Time: failed to open file %s", ats_path.c_str());
            return;
        }

        if (fread(&offset, sizeof(offset), 1, f) != 1) {
            LOGI("TWFunc::Fixup_Time: failed load uint64 from file %s", ats_path.c_str());
            fclose(f);
            return;
        }
        fclose(f);

        LOGI("TWFunc::Fixup_Time: Setting time offset from file %s, offset %" PRIu64, ats_path.c_str(), offset);

        clock_gettime(CLOCK_REALTIME, &ts);

        ts.tv_sec += offset / 1000;
        ts.tv_nsec += (offset % 1000) * 1000000;

        while (ts.tv_nsec >= 1000000000) {
            ++ts.tv_sec;
            ts.tv_nsec -= 1000000000;
        }

        clock_settime(CLOCK_REALTIME, &ts);

        LOGI("TWFunc::Fixup_Time: Date and time corrected: %s",
             current_date_time().c_str());
    }
}

int TWFunc::Set_Brightness(std::string brightness_value)
{
    int result = -1;
    std::string secondary_brightness_file;

    if (DataManager::GetIntValue(VAR_TW_HAS_BRIGHTNESS_FILE)) {
        LOGI("Setting brightness control to %s", brightness_value.c_str());
        result = mb::util::file_write_data(
                DataManager::GetStrValue(VAR_TW_BRIGHTNESS_FILE),
                brightness_value.data(), brightness_value.size())
                ? 0 : -1;
        if (!secondary_brightness_file.empty()) {
            LOGI("Setting secondary brightness control to %s",
                 brightness_value.c_str());
            (void) mb::util::file_write_data(
                    secondary_brightness_file,
                    brightness_value.data(), brightness_value.size());
        }
    }

    return result;
}
