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

#include <algorithm>

#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <mntent.h>
#include <sys/stat.h>

#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/autoclose/file.h"
#include "mbutil/finally.h"
#include "mbutil/mount.h"
#include "mbutil/properties.h"
#include "mbutil/string.h"

#include "multiboot.h"

#define BUILD_PROP "build.prop"

static std::vector<std::string> extsd_mount_points{
    "/raw/extsd",
    "/external_sd",
    "/external_sdcard",
    "/microSD",
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
    std::string result;

    char *path = mb_format(MULTIBOOT_DIR "/%s/boot.img", id.c_str());
    if (path) {
        result = get_raw_path(path);
        free(path);
    }

    return result;
}

std::string Rom::config_path()
{
    std::string result;

    char *path = mb_format(MULTIBOOT_DIR "/%s/config.json", id.c_str());
    if (path) {
        result = get_raw_path(path);
        free(path);
    }

    return result;
}

std::string Rom::thumbnail_path()
{
    std::string result;

    char *path = mb_format(MULTIBOOT_DIR "/%s/thumbnail.webp", id.c_str());
    if (path) {
        result = get_raw_path(path);
        free(path);
    }

    return result;
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
    char id[64];
    snprintf(id, sizeof(id), "multi-slot-%u", num);

    std::shared_ptr<Rom> rom(new Rom());
    rom->id = id;
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

static bool cmp_rom_id(const std::shared_ptr<Rom> &a,
                       const std::shared_ptr<Rom> &b)
{
    return a->id < b->id;
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

    std::vector<std::shared_ptr<Rom>> temp_roms;

    struct dirent *ent;
    while ((ent = readdir(dp))) {
        if (strcmp(ent->d_name, "data-slot-") == 0
                || !mb_starts_with(ent->d_name, "data-slot-")) {
            continue;
        }

        std::string fullpath(system);
        fullpath += "/";
        fullpath += ent->d_name;

        if (stat(fullpath.c_str(), &sb) == 0 && S_ISDIR(sb.st_mode)) {
            temp_roms.push_back(create_rom_data_slot(ent->d_name + 10));
        }
    }

    // Sort by ID
    std::sort(temp_roms.begin(), temp_roms.end(), &cmp_rom_id);
    std::move(temp_roms.begin(), temp_roms.end(), std::back_inserter(roms));
}

void Roms::add_extsd_roms()
{
    std::string mount_point = get_extsd_partition();
    std::string search_dir;
    bool is_boot;

    if (mount_point.empty()) {
        search_dir = get_raw_path(MULTIBOOT_DIR);
        is_boot = true;
    } else {
        search_dir = mount_point;
        search_dir += "/multiboot";
        is_boot = false;
    }

    DIR *dp = opendir(search_dir.c_str());
    if (!dp) {
        return;
    }

    auto close_dp = util::finally([&]{
        closedir(dp);
    });

    struct stat sb;

    std::vector<std::shared_ptr<Rom>> temp_roms;

    struct dirent *ent;
    while ((ent = readdir(dp))) {
        if (strcmp(ent->d_name, "extsd-slot-") == 0
                || !mb_starts_with(ent->d_name, "extsd-slot-")) {
            continue;
        }

        std::string image(search_dir);
        image += "/";
        image += ent->d_name;
        if (is_boot) {
            image += "/boot.img";
        } else {
            image += "/system.img";
        }

        if (stat(image.c_str(), &sb) == 0 && S_ISREG(sb.st_mode)) {
            temp_roms.push_back(create_rom_extsd_slot(ent->d_name + 11));
        }
    }

    std::sort(temp_roms.begin(), temp_roms.end(), &cmp_rom_id);
    std::move(temp_roms.begin(), temp_roms.end(), std::back_inserter(roms));
}

void Roms::add_installed()
{
    Roms all_roms;
    all_roms.add_builtin();
    all_roms.add_data_roms();
    all_roms.add_extsd_roms();

    struct stat sb;

    for (auto rom : all_roms.roms) {
        std::string boot_path = get_raw_path(rom->boot_image_path());
        std::string system_path = rom->full_system_path();

        if (stat(boot_path.c_str(), &sb) == 0) {
            // If boot image exists, assume that the ROM is installed
            roms.push_back(rom);
        } else if (rom->system_is_image) {
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
    std::string prop_id = util::property_get_string(PROP_MULTIBOOT_ROM_ID, {});
    // This is necessary for the daemon to get a correct result before Android
    // boots (eg. for the boot UI)
    if (prop_id.empty()) {
        prop_id = util::property_file_get_string(
                DEFAULT_PROP_PATH, PROP_MULTIBOOT_ROM_ID, {});
    }

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
        util::property_set(PROP_MULTIBOOT_ROM_ID, "primary");

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
                util::property_set(PROP_MULTIBOOT_ROM_ID, rom->id.c_str());

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
    } else if (mb_starts_with(id.c_str(), "multi-slot-")) {
        unsigned int num;
        if (sscanf(id.c_str(), "multi-slot-%u", &num) == 1) {
            return create_rom_multi_slot(num);
        }
    } else if (mb_starts_with(id.c_str(), "data-slot-")
            && id != "data-slot-") {
        return create_rom_data_slot(id.substr(10));
    } else if (mb_starts_with(id.c_str(), "extsd-slot-")
            && id != "extsd-slot-") {
        return create_rom_extsd_slot(id.substr(11));
    }

    return std::shared_ptr<Rom>();
}

bool Roms::is_valid(const std::string &id)
{
    return id == "primary"
            || id == "dual"
            || (id != "multi-slot-" && mb_starts_with(id.c_str(), "multi-slot-"))
            || (id != "data-slot-" && mb_starts_with(id.c_str(), "data-slot-"))
            || (id != "extsd-slot-" && mb_starts_with(id.c_str(), "extsd-slot-"));
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
    // Try hard-coded mount points first
    struct stat sb;
    for (const std::string &mount_point : extsd_mount_points) {
        if (stat(mount_point.c_str(), &sb) == 0) {
            if (util::is_mounted(mount_point)) {
                return mount_point;
            }
        }
    }

    static const char *prefix_mnt = "/mnt/media_rw/";
    static const char *prefix_storage = "/storage/";

    // Look for mounted MMC partitions
    autoclose::file fp(setmntent("/proc/mounts", "r"), endmntent);
    if (fp) {
        struct mntent ent;
        char buf[1024];
        struct stat sb;

        while (getmntent_r(fp.get(), &ent, buf, sizeof(buf))) {
            // Skip useless mounts
            if (!mb_starts_with(ent.mnt_dir, prefix_mnt)) {
                continue;
            }

            if (stat(ent.mnt_fsname, &sb) < 0) {
                LOGW("%s: Failed to stat: %s", ent.mnt_fsname, strerror(errno));
                continue;
            }

            if (major(sb.st_rdev) == 179) {
                std::string path(prefix_storage);
                path += ent.mnt_dir + strlen(prefix_mnt);

                if (util::is_mounted(path)) {
                    return path;
                }
            }
        }
    }

    return std::string();
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
    std::string result;

    // This is faster than doing util::path_split()...
    if (path == "/system" || mb_starts_with(path.c_str(), "/system/")) {
        result = Roms::get_system_partition();
        result += path.substr(7);
    } else if (path == "/cache" || mb_starts_with(path.c_str(), "/cache/")) {
        result = Roms::get_cache_partition();
        result += path.substr(6);
    } else if (path == "/data" || mb_starts_with(path.c_str(), "/data/")) {
        result = Roms::get_data_partition();
        result += path.substr(5);
    } else {
        result = path;
    }

    return result;
}

}
