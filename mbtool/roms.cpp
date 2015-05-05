/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "roms.h"

#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include <cppformat/format.h>

#include "util/logging.h"
#include "util/properties.h"

#define SYSTEM "/system"
#define CACHE "/cache"
#define DATA "/data"

#define BUILD_PROP "build.prop"


namespace mb
{

void Roms::add_builtin()
{
    // Primary
    std::shared_ptr<Rom> primary(new Rom());
    primary->id = "primary";
    primary->system_path = SYSTEM;
    primary->cache_path = CACHE;
    primary->data_path = DATA;
    roms.push_back(std::move(primary));

    // Secondary
    std::shared_ptr<Rom> dual(new Rom());
    dual->id = "dual";
    dual->system_path.append(SYSTEM).append("/")
                     .append("multiboot").append("/")
                     .append("dual").append("/")
                     .append("system");
    dual->cache_path.append(CACHE).append("/")
                    .append("multiboot").append("/")
                    .append("dual").append("/")
                    .append("cache");
    dual->data_path.append(DATA).append("/")
                   .append("multiboot").append("/")
                   .append("dual").append("/")
                   .append("data");
    roms.push_back(std::move(dual));

    // Multislots
    for (int i = 1; i <= 3; ++i) {
        std::shared_ptr<Rom> multislot(new Rom());
        multislot->id = fmt::format("multi-slot-{:d}", i);
        multislot->system_path.append(CACHE).append("/")
                              .append("multiboot").append("/")
                              .append(multislot->id).append("/")
                              .append("system");
        multislot->cache_path.append(SYSTEM).append("/")
                             .append("multiboot").append("/")
                             .append(multislot->id).append("/")
                             .append("cache");
        multislot->data_path.append(DATA).append("/")
                            .append("multiboot").append("/")
                            .append(multislot->id).append("/")
                            .append("data");
        roms.push_back(std::move(multislot));
    }
}

void Roms::add_installed()
{
    Roms all_roms;
    all_roms.add_builtin();

    struct stat sb;

    for (auto rom : all_roms.roms) {
        // Old style: /system -> /raw-system, etc.
        std::string raw_bp_path_old(rom->system_path);
        raw_bp_path_old.insert(1, "raw-");
        raw_bp_path_old += "/build.prop";

        // New style: /system -> /raw/system, etc.
        std::string raw_bp_path_new("/raw");
        raw_bp_path_new += rom->system_path;
        raw_bp_path_new += "/build.prop";

        // Plain path
        std::string bp_path(rom->system_path);
        bp_path += "/build.prop";

        if (stat(raw_bp_path_old.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
            rom->system_path.insert(1, "raw-");
            rom->cache_path.insert(1, "raw-");
            rom->data_path.insert(1, "raw-");
            roms.push_back(rom);
        } else if (stat(raw_bp_path_new.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
            rom->system_path.insert(0, "/raw");
            rom->cache_path.insert(0, "/raw");
            rom->data_path.insert(0, "/raw");
            roms.push_back(rom);
        } else if (stat(bp_path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
            roms.push_back(rom);
        }
    }
}

std::shared_ptr<Rom> Roms::find_by_id(const std::string &id) const
{
    for (auto r : roms) {
        if (r->id == id) {
            return r;
        }
    }

    return std::shared_ptr<Rom>();
}

std::shared_ptr<Rom> Roms::get_current_rom()
{
    Roms roms;
    roms.add_installed();

    // This is set if mbtool is handling the boot process
    std::string prop_id;
    util::get_property("ro.multiboot.romid", &prop_id, std::string());

    if (!prop_id.empty()) {
        auto rom = roms.find_by_id(prop_id);
        if (rom) {
            return rom;
        }
    }

    // If /raw/ or /raw-system/ does not exist, then this is an unpatched
    // primary ROM
    struct stat sb;
    bool has_raw = stat("/raw", &sb) == 0;
    bool has_raw_system = stat("/raw-system", &sb) == 0;
    if (!has_raw && !has_raw_system) {
        // Cache the result
        util::set_property("ro.multiboot.romid", "primary");

        return roms.find_by_id("primary");
    }

    // Otherwise, iterate through the installed ROMs

    if (stat("/system/build.prop", &sb) == 0) {
        for (auto rom : roms.roms) {
            std::string build_prop(rom->system_path);
            build_prop += "/build.prop";

            struct stat sb2;
            if (stat(build_prop.c_str(), &sb2) == 0
                    && sb.st_dev == sb2.st_dev
                    && sb.st_ino == sb2.st_ino) {
                // Cache the result
                util::set_property("ro.multiboot.romid", rom->id);

                return rom;
            }
        }
    }

    return std::shared_ptr<Rom>();
}

std::string get_raw_path(const std::string &path)
{
    struct stat sb;
    if (stat("/raw", &sb) == 0 && S_ISDIR(sb.st_mode)) {
        std::string result("/raw");
        if (!path.empty() && path[0] != '/') {
            result += "/";
        }
        result += path;
        return result;
    } else {
        return path;
    }
}

}
