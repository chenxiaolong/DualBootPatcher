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

#include "util/logging.h"

#define SYSTEM "/system"
#define CACHE "/cache"
#define DATA "/data"

#define BUILD_PROP "build.prop"


namespace mb
{

Rom::Rom() : use_raw_paths(false)
{
}

bool mb_roms_add_builtin(std::vector<std::shared_ptr<Rom>> *roms)
{
    // Primary
    std::shared_ptr<Rom> primary(new Rom());
    primary->id = "primary";
    primary->system_path = SYSTEM;
    primary->cache_path = CACHE;
    primary->data_path = DATA;
    roms->push_back(std::move(primary));

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
    roms->push_back(std::move(dual));

    // Multislots
    char id[20] = { 0 };
    for (int i = 1; i <= 3; ++i) {
        snprintf(id, 20, "multi-slot-%d", i);

        std::shared_ptr<Rom> multislot(new Rom());
        multislot->id = id;
        multislot->system_path.append(CACHE).append("/")
                              .append("multiboot").append("/")
                              .append(id).append("/")
                              .append("system");
        multislot->cache_path.append(SYSTEM).append("/")
                             .append("multiboot").append("/")
                             .append(id).append("/")
                             .append("cache");
        multislot->data_path.append(DATA).append("/")
                            .append("multiboot").append("/")
                            .append(id).append("/")
                            .append("data");
        roms->push_back(multislot);
    }

    return true;
}

bool mb_roms_add_installed(std::vector<std::shared_ptr<Rom>> *roms)
{
    std::vector<std::shared_ptr<Rom>> all_roms;
    mb_roms_add_builtin(&all_roms);

    struct stat sb;

    while (!all_roms.empty()) {
        auto r = std::move(all_roms[0]);

        std::string raw_bp_path("/raw-");
        raw_bp_path += r->system_path;
        raw_bp_path += "/";
        raw_bp_path += "build.prop";

        std::string bp_path;
        bp_path += r->system_path;
        bp_path += "/";
        bp_path += "build.prop";

        all_roms.erase(all_roms.begin());

        if (stat(raw_bp_path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
            r->use_raw_paths = 1;
            roms->push_back(std::move(r));
        } else if (stat(bp_path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
            r->use_raw_paths = 0;
            roms->push_back(std::move(r));
        } else {
            // Rom object will be deleted once the shared pointer goes out of
            // scope
        }
    }

    return true;
}

std::shared_ptr<Rom> mb_find_rom_by_id(std::vector<std::shared_ptr<Rom>> *roms,
                                       const std::string &id)
{
    for (auto r : *roms) {
        if (r->id == id) {
            return r;
        }
    }

    return std::shared_ptr<Rom>();
}

}