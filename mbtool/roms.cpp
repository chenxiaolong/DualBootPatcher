/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include <dirent.h>
#include <sys/stat.h>

#include "mblog/logging.h"

#include "multiboot.h"
#include "util/finally.h"
#include "util/mount.h"
#include "util/properties.h"
#include "util/string.h"

#define BUILD_PROP "build.prop"

static std::vector<std::string> extsd_mount_points{
    "/raw/extsd",
    "/external_sd",
    "/external_sdcard",
    "/extSdCard",
    "/storage/sdcard1",
    "/storage/extSdCard",
    "/storage/external_SD",
    "/storage/MicroSD"
};

namespace mb
{

std::string Rom::full_system_path()
{
    std::string path = Roms::get_mountpoint(system_source);
    if (!path.empty()) {
        path += system_path;
    }
    return path;
}

std::string Rom::full_cache_path()
{
    std::string path = Roms::get_mountpoint(cache_source);
    if (!path.empty()) {
        path += cache_path;
    }
    return path;
}

std::string Rom::full_data_path()
{
    std::string path = Roms::get_mountpoint(data_source);
    if (!path.empty()) {
        path += data_path;
    }
    return path;
}

std::string Rom::boot_image_path()
{
    return util::format(MULTIBOOT_DIR "/%s/boot.img", id.c_str());
}

std::string Rom::config_path()
{
    return util::format(MULTIBOOT_DIR "/%s/config.json", id.c_str());
}

std::string Rom::thumbnail_path()
{
    return util::format(MULTIBOOT_DIR "/%s/thumbnail.webp", id.c_str());
}

std::shared_ptr<Rom> Roms::create_rom_primary()
{
    std::shared_ptr<Rom> rom(new Rom());
    rom->id = "primary";
    rom->system_source = Rom::Source::SYSTEM;
    rom->cache_source = Rom::Source::CACHE;
    rom->data_source = Rom::Source::DATA;
    rom->system_path = std::string();
    rom->cache_path = std::string();
    rom->data_path = std::string();
    rom->system_is_image = false;
    rom->cache_is_image = false;
    rom->data_is_image = false;
    return rom;
}

std::shared_ptr<Rom> Roms::create_rom_dual()
{
    std::shared_ptr<Rom> rom(new Rom());
    rom->id = "dual";
    rom->system_source = Rom::Source::SYSTEM;
    rom->cache_source = Rom::Source::CACHE;
    rom->data_source = Rom::Source::DATA;
    rom->system_path = "/multiboot/dual/system";
    rom->cache_path = "/multiboot/dual/cache";
    rom->data_path = "/multiboot/dual/data";
    rom->system_is_image = false;
    rom->cache_is_image = false;
    rom->data_is_image = false;
    return rom;
}

std::shared_ptr<Rom> Roms::create_rom_multi_slot(unsigned int num)
{
    std::shared_ptr<Rom> rom(new Rom());
    rom->id = util::format("multi-slot-%d", num);
    rom->system_source = Rom::Source::CACHE;
    rom->cache_source = Rom::Source::SYSTEM;
    rom->data_source = Rom::Source::DATA;
    rom->system_path.append("/multiboot/").append(rom->id).append("/system");
    rom->cache_path.append("/multiboot/").append(rom->id).append("/cache");
    rom->data_path.append("/multiboot/").append(rom->id).append("/data");
    rom->system_is_image = false;
    rom->cache_is_image = false;
    rom->data_is_image = false;
    return rom;
}

std::shared_ptr<Rom> Roms::create_rom_data_slot(const std::string &id)
{
    std::shared_ptr<Rom> rom(new Rom());
    rom->id.append("data-slot-").append(id);
    rom->system_source = Rom::Source::DATA;
    rom->cache_source = Rom::Source::CACHE;
    rom->data_source = Rom::Source::DATA;
    rom->system_path.append("/multiboot/").append(rom->id).append("/system");
    rom->cache_path.append("/multiboot/").append(rom->id).append("/cache");
    rom->data_path.append("/multiboot/").append(rom->id).append("/data");
    rom->system_is_image = false;
    rom->cache_is_image = false;
    rom->data_is_image = false;
    return rom;
}

std::shared_ptr<Rom> Roms::create_rom_extsd_slot(const std::string &id)
{
    std::shared_ptr<Rom> rom(new Rom());
    rom->id.append("extsd-slot-").append(id);
    rom->system_source = Rom::Source::EXTERNAL_SD;
    rom->cache_source = Rom::Source::CACHE;
    rom->data_source = Rom::Source::DATA;
    rom->system_path.append("/multiboot/").append(rom->id).append("/system.img");
    rom->cache_path.append("/multiboot/").append(rom->id).append("/cache");
    rom->data_path.append("/multiboot/").append(rom->id).append("/data");
    rom->system_is_image = true;
    rom->cache_is_image = false;
    rom->data_is_image = false;
    return rom;
}

void Roms::add_builtin()
{
    roms.push_back(create_rom_primary());
    roms.push_back(create_rom_dual());
    for (int i = 1; i <= 3; ++i) {
        roms.push_back(create_rom_multi_slot(i));
    }
}

void Roms::add_data_roms()
{
    std::string system = get_raw_path("/data/multiboot");

    DIR *dp = opendir(system.c_str());
    if (!dp ) {
        return;
    }

    auto close_dp = util::finally([&]{
        closedir(dp);
    });

    struct stat sb;

    struct dirent *ent;
    while ((ent = readdir(dp))) {
        if (strcmp(ent->d_name, "data-slot-") == 0
                || !util::starts_with(ent->d_name, "data-slot-")) {
            continue;
        }

        std::string fullpath(system);
        fullpath += "/";
        fullpath += ent->d_name;

        if (stat(fullpath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
            roms.push_back(create_rom_data_slot(ent->d_name + 10));
        }
    }
}

void Roms::add_extsd_roms()
{
    for (const std::string &mount_point : extsd_mount_points) {
        std::string search_dir(mount_point);
        search_dir += "/multiboot";

        DIR *dp = opendir(search_dir.c_str());
        if (!dp ) {
            continue;
        }

        auto close_dp = util::finally([&]{
            closedir(dp);
        });

        struct stat sb;

        struct dirent *ent;
        while ((ent = readdir(dp))) {
            if (strcmp(ent->d_name, "extsd-slot-") == 0
                    || !util::starts_with(ent->d_name, "extsd-slot-")) {
                continue;
            }

            std::string image(search_dir);
            image += "/";
            image += ent->d_name;
            image += "/system.img";

            if (stat(image.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
                roms.push_back(create_rom_extsd_slot(ent->d_name + 11));
            }
        }

        break;
    }
}

void Roms::add_installed()
{
    Roms all_roms;
    all_roms.add_builtin();
    all_roms.add_data_roms();
    all_roms.add_extsd_roms();

    struct stat sb;

    for (auto rom : all_roms.roms) {
        std::string system_path = rom->full_system_path();

        if (rom->system_is_image) {
            // If /system is on an ext4 image, check if the image exists
            if (stat(system_path.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
                roms.push_back(rom);
            }
        } else {
            // If /system is bind-mounted, check if build.prop exists
            std::string build_prop(system_path);
            build_prop += "/build.prop";

            if (stat(build_prop.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
                roms.push_back(rom);
            }
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
            // We can't check roms that use images since they aren't mounted
            if (rom->system_is_image) {
                continue;
            }

            std::string path = rom->full_system_path();
            if (path.empty()) {
                continue;
            }

            path += "/build.prop";

            struct stat sb2;
            if (stat(path.c_str(), &sb2) == 0
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

std::shared_ptr<Rom> Roms::create_rom(const std::string &id)
{
    if (id == "primary") {
        return create_rom_primary();
    } else if (id == "dual") {
        return create_rom_dual();
    } else if (util::starts_with(id, "multi-slot-")) {
        unsigned int num;
        if (sscanf(id.c_str(), "multi-slot-%u", &num) == 1) {
            return create_rom_multi_slot(num);
        }
    } else if (util::starts_with(id, "data-slot-") && id != "data-slot-") {
        return create_rom_data_slot(id.substr(10));
    } else if (util::starts_with(id, "extsd-slot-") && id != "extsd-slot-") {
        return create_rom_extsd_slot(id.substr(11));
    }

    return std::shared_ptr<Rom>();
}

bool Roms::is_valid(const std::string &id)
{
    return id == "primary"
            || id == "dual"
            || (id != "multi-slot-" && util::starts_with(id, "multi-slot-"))
            || (id != "data-slot-" && util::starts_with(id, "data-slot-"))
            || (id != "extsd-slot-" && util::starts_with(id, "extsd-slot-"));
}

std::string Roms::get_system_partition()
{
    struct stat sb;
    if (stat("/raw/system", &sb) == 0) {
        return "/raw/system";
    } else if (stat("/raw-system", &sb) == 0) {
        return "/raw-system";
    } else if (stat("/system", &sb) == 0) {
        return "/system";
    } else {
        return std::string();
    }
}

std::string Roms::get_cache_partition()
{
    struct stat sb;
    if (stat("/raw/cache", &sb) == 0) {
        return "/raw/cache";
    } else if (stat("/raw-cache", &sb) == 0) {
        return "/raw-cache";
    } else if (stat("/cache", &sb) == 0) {
        return "/cache";
    } else {
        return std::string();
    }
}

std::string Roms::get_data_partition()
{
    struct stat sb;
    if (stat("/raw/data", &sb) == 0) {
        return "/raw/data";
    } else if (stat("/raw-data", &sb) == 0) {
        return "/raw-data";
    } else if (stat("/data", &sb) == 0) {
        return "/data";
    } else {
        return std::string();
    }
}

std::string Roms::get_extsd_partition()
{
    // Loop through the list of paths. If the path is a mount point, then select
    // that path. Otherwise, choose the first path that exists.
    std::vector<std::string> maybe;

    struct stat sb;
    for (const std::string &mount_point : extsd_mount_points) {
        if (stat(mount_point.c_str(), &sb) == 0) {
            if (util::is_mounted(mount_point)) {
                return mount_point;
            } else {
                maybe.push_back(mount_point);
            }
        }
    }

    if (maybe.empty()) {
        return std::string();
    } else {
        return maybe[0];
    }
}

std::string Roms::get_mountpoint(Rom::Source source)
{
    switch (source) {
    case Rom::Source::SYSTEM:
        return get_system_partition();
    case Rom::Source::CACHE:
        return get_cache_partition();
    case Rom::Source::DATA:
        return get_data_partition();
    case Rom::Source::EXTERNAL_SD:
        return get_extsd_partition();
    default:
        return std::string();
    }
}

// TODO: Remove this. Callers should build the paths themselves using
//       get_system_partition(), get_cache_partition(), and get_data_partition()
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
