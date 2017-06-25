/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "utilities.h"

#include <algorithm>

#include <cstring>

#include <fcntl.h>
#include <getopt.h>
#include <sys/stat.h>
#include <unistd.h>

#include "minizip/zip.h"
#include "minizip/ioandroid.h"

#include "mbcommon/string.h"
#include "mbcommon/version.h"
#include "mbdevice/device.h"
#include "mbdevice/validate.h"
#include "mbdevice/json.h"
#include "mblog/logging.h"
#include "mblog/stdio_logger.h"
#include "mbutil/delete.h"
#include "mbutil/file.h"
#include "mbutil/finally.h"
#include "mbutil/fts.h"
#include "mbutil/path.h"
#include "mbutil/properties.h"
#include "mbutil/string.h"

#include "multiboot.h"
#include "romconfig.h"
#include "roms.h"
#include "switcher.h"
#include "wipe.h"

namespace mb
{

const char *devices_file = nullptr;

static Device * get_device(const char *path)
{
    std::string prop_product_device =
            util::property_get_string("ro.product.device", {});
    std::string prop_build_product =
            util::property_get_string("ro.build.product", {});

    LOGD("ro.product.device = %s", prop_product_device.c_str());
    LOGD("ro.build.product = %s", prop_build_product.c_str());

    std::vector<unsigned char> contents;
    if (!util::file_read_all(path, &contents)) {
        LOGE("%s: Failed to read file: %s", path, strerror(errno));
        return nullptr;
    }
    contents.push_back('\0');

    MbDeviceJsonError error;
    Device **devices = mb_device_new_list_from_json(
            (const char *) contents.data(), &error);
    if (!devices) {
        LOGE("%s: Failed to load devices", path);
        return nullptr;
    }

    Device *device = nullptr;

    for (auto it = devices; *it; ++it) {
        if (mb_device_validate(*it) != 0) {
            LOGW("Skipping invalid device");
            continue;
        }

        auto codenames = mb_device_codenames(*it);

        for (auto it2 = codenames; *it2; ++it2) {
            if (prop_product_device == *it2 || prop_build_product == *it2) {
                device = *it;
                break;
            }
        }

        // Free any devices we don't need
        if (device != *it) {
            mb_device_free(*it);
        }
    }

    free(devices);

    if (!device) {
        LOGE("Unknown device: %s", prop_product_device.c_str());
        return nullptr;
    }

    return device;
}

static bool utilities_switch_rom(const char *rom_id, bool force)
{
    const char *block_dev = nullptr;
    struct stat sb;

    if (!devices_file) {
        LOGE("No device definitions file specified");
        return false;
    }

    Device *device = get_device(devices_file);
    if (!device) {
        LOGE("Failed to detect device");
        return false;
    }

    auto free_device = util::finally([&]{
        mb_device_free(device);
    });

    for (auto it = mb_device_boot_block_devs(device); *it; ++it) {
        if (stat(*it, &sb) == 0 && S_ISBLK(sb.st_mode)) {
            block_dev = *it;
            break;
        }
    }

    if (!block_dev) {
        LOGE("All specified boot partition paths could not be found");
        return false;
    }

    SwitchRomResult ret = switch_rom(
            rom_id, block_dev, mb_device_block_dev_base_dirs(device), force);
    switch (ret) {
    case SwitchRomResult::SUCCEEDED:
        LOGD("SUCCEEDED");
        break;
    case SwitchRomResult::FAILED:
        LOGD("FAILED");
        break;
    case SwitchRomResult::CHECKSUM_INVALID:
        LOGD("CHECKSUM_INVALID");
        break;
    case SwitchRomResult::CHECKSUM_NOT_FOUND:
        LOGD("CHECKSUM_NOT_FOUND");
        break;
    }

    return ret == SwitchRomResult::SUCCEEDED;
}

static bool utilities_wipe_system(const char *rom_id)
{
    auto rom = Roms::create_rom(rom_id);
    if (!rom) {
        return false;
    }

    return wipe_system(rom);
}

static bool utilities_wipe_cache(const char *rom_id)
{
    auto rom = Roms::create_rom(rom_id);
    if (!rom) {
        return false;
    }

    return wipe_cache(rom);
}

static bool utilities_wipe_data(const char *rom_id)
{
    auto rom = Roms::create_rom(rom_id);
    if (!rom) {
        return false;
    }

    return wipe_data(rom);
}

static bool utilities_wipe_dalvik_cache(const char *rom_id)
{
    auto rom = Roms::create_rom(rom_id);
    if (!rom) {
        return false;
    }

    return wipe_dalvik_cache(rom);
}

static bool utilities_wipe_multiboot(const char *rom_id)
{
    auto rom = Roms::create_rom(rom_id);
    if (!rom) {
        return false;
    }

    return wipe_multiboot(rom);
}

static void generate_aroma_config(std::vector<unsigned char> *data)
{
    std::string str_data(data->begin(), data->end());

    std::string rom_menu_items;
    std::string rom_selection_items;

    Roms roms;
    roms.add_installed();

    for (std::size_t i = 0; i < roms.roms.size(); ++i) {
        const std::shared_ptr<Rom> &rom = roms.roms[i];

        std::string config_path = rom->config_path();
        std::string name = rom->id;

        RomConfig config;
        if (config.load_file(config_path) && !config.name.empty()) {
            name = config.name;
        }

        char *menu_item = mb_format("\"%s\", \"\", \"@default\",\n",
                                    name.c_str());
        if (menu_item) {
            rom_menu_items += menu_item;
            free(menu_item);
        }

        char *selection_item = mb_format(
                "if prop(\"operations.prop\", \"selected\") == \"%zu\" then\n"
                "    setvar(\"romid\", \"%s\");\n"
                "    setvar(\"romname\", \"%s\");\n"
                "endif;\n",
                i + 2 + 1, rom->id.c_str(), name.c_str());
        if (selection_item) {
            rom_selection_items += selection_item;
            free(selection_item);
        }
    }

    char *first_index = mb_format("%d", 2 + 1);
    char *last_index = mb_format("%zu", 2 + roms.roms.size());

    util::replace_all(&str_data, "\t", "\\t");
    util::replace_all(&str_data, "@MBTOOL_VERSION@", version());
    util::replace_all(&str_data, "@ROM_MENU_ITEMS@", rom_menu_items);
    util::replace_all(&str_data, "@ROM_SELECTION_ITEMS@", rom_selection_items);
    if (first_index) {
        util::replace_all(&str_data, "@FIRST_INDEX@", first_index);
        free(first_index);
    }
    if (last_index) {
        util::replace_all(&str_data, "@LAST_INDEX@", last_index);
        free(last_index);
    }

    util::replace_all(&str_data, "@SYSTEM_MOUNT_POINT@", Roms::get_system_partition());
    util::replace_all(&str_data, "@CACHE_MOUNT_POINT@", Roms::get_cache_partition());
    util::replace_all(&str_data, "@DATA_MOUNT_POINT@", Roms::get_data_partition());
    util::replace_all(&str_data, "@EXTSD_MOUNT_POINT@", Roms::get_extsd_partition());

    data->assign(str_data.begin(), str_data.end());
}

class AromaGenerator : public util::FTSWrapper
{
public:
    AromaGenerator(std::string path, std::string zippath)
        : FTSWrapper(std::move(path), FTS_GroupSpecialFiles),
        _zippath(std::move(zippath))
    {
    }

    virtual bool on_pre_execute() override
    {
        zlib_filefunc64_def zFunc;
        memset(&zFunc, 0, sizeof(zFunc));
        fill_android_filefunc64(&zFunc);
        _zf = zipOpen2_64(_zippath.c_str(), 0, nullptr, &zFunc);
        if (!_zf) {
            LOGE("%s: Failed to open for writing", _zippath.c_str());
            return false;
        }

        return true;
    }

    virtual bool on_post_execute(bool success) override
    {
        (void) success;
        return zipClose(_zf, nullptr) == ZIP_OK;
    }

    virtual int on_reached_file() override
    {
        std::string name = std::string(_curr->fts_path).substr(_path.size() + 1);
        LOGD("%s -> %s", _curr->fts_path, name.c_str());

        if (name == "META-INF/com/google/android/aroma-config.in") {
            std::vector<unsigned char> data;
            if (!util::file_read_all(_curr->fts_accpath, &data)) {
                LOGE("Failed to read: %s", _curr->fts_path);
                return false;
            }

            generate_aroma_config(&data);

            name = "META-INF/com/google/android/aroma-config";
            bool ret = add_file(name, data);
            return ret ? Action::FTS_OK : Action::FTS_Fail;
        } else {
            bool ret = add_file(name, _curr->fts_accpath);
            return ret ? Action::FTS_OK : Action::FTS_Fail;
        }
    }

    virtual int on_reached_symlink() override
    {
        LOGW("Ignoring symlink when creating zip: %s", _curr->fts_path);
        return Action::FTS_OK;
    }

    virtual int on_reached_special_file() override
    {
        LOGW("Ignoring special file when creating zip: %s", _curr->fts_path);
        return Action::FTS_OK;
    }

private:
    zipFile _zf;
    std::string _zippath;

    bool add_file(const std::string &name,
                  const std::vector<unsigned char> &contents)
    {
        // Obviously never true, but we'll keep it here just in case
        bool zip64 = (uint64_t) contents.size() >= ((1ull << 32) - 1);

        zip_fileinfo zi;
        memset(&zi, 0, sizeof(zi));

        int ret = zipOpenNewFileInZip2_64(
            _zf,                    // file
            name.c_str(),           // filename
            &zi,                    // zip_fileinfo
            nullptr,                // extrafield_local
            0,                      // size_extrafield_local
            nullptr,                // extrafield_global
            0,                      // size_extrafield_global
            nullptr,                // comment
            Z_DEFLATED,             // method
            Z_DEFAULT_COMPRESSION,  // level
            0,                      // raw
            zip64                   // zip64
        );

        if (ret != ZIP_OK) {
            LOGW("minizip: Failed to add file (error code: %d): [memory]", ret);

            return false;
        }

        // Write data to file
        ret = zipWriteInFileInZip(_zf, contents.data(), contents.size());
        if (ret != ZIP_OK) {
            LOGW("minizip: Failed to write data (error code: %d): [memory]", ret);
            zipCloseFileInZip(_zf);

            return false;
        }

        zipCloseFileInZip(_zf);

        return true;
    }

    bool add_file(const std::string &name, const std::string &path)
    {
        // Copy file into archive
        int fd = open64(path.c_str(), O_RDONLY);
        if (fd < 0) {
            LOGE("%s: Failed to open for reading: %s",
                 path.c_str(), strerror(errno));
            return false;
        }

        struct stat sb;
        if (fstat(fd, &sb) < 0) {
            LOGE("%s: Failed to stat: %s",
                 path.c_str(), strerror(errno));
            return false;
        }

        uint64_t size;
        lseek64(fd, 0, SEEK_END);
        size = lseek64(fd, 0, SEEK_CUR);
        lseek64(fd, 0, SEEK_SET);

        bool zip64 = size >= ((1ull << 32) - 1);

        zip_fileinfo zi;
        memset(&zi, 0, sizeof(zi));
        zi.external_fa = (sb.st_mode & 0777) << 16;

        int ret = zipOpenNewFileInZip2_64(
            _zf,                    // file
            name.c_str(),           // filename
            &zi,                    // zip_fileinfo
            nullptr,                // extrafield_local
            0,                      // size_extrafield_local
            nullptr,                // extrafield_global
            0,                      // size_extrafield_global
            nullptr,                // comment
            Z_DEFLATED,             // method
            Z_DEFAULT_COMPRESSION,  // level
            0,                      // raw
            zip64                   // zip64
        );

        if (ret != ZIP_OK) {
            LOGW("minizip: Failed to add file (error code: %d): %s",
                 ret, path.c_str());
            return false;
        }

        // Write data to file
        char buf[32768];
        ssize_t n;

        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            ret = zipWriteInFileInZip(_zf, buf, n);
            if (ret != ZIP_OK) {
                LOGW("minizip: Failed to write data (error code: %d): %s",
                     ret, path.c_str());
                zipCloseFileInZip(_zf);

                return false;
            }
        }

        if (n < 0) {
            zipCloseFileInZip(_zf);

            LOGE("%s: Failed to read file: %s",
                 path.c_str(), strerror(errno));
            return false;
        }

        zipCloseFileInZip(_zf);

        return true;
    }
};

static void utilities_usage(bool error)
{
    FILE *stream = error ? stderr : stdout;

    fprintf(stream,
            "Usage: utilities [opt...] generate [template dir] [output file]\n"
            "   OR: utilities [opt...] switch [ROM ID] [--force]\n"
            "   OR: utilities [opt...] wipe-system [ROM ID]\n"
            "   OR: utilities [opt...] wipe-cache [ROM ID]\n"
            "   OR: utilities [opt...] wipe-data [ROM ID]\n"
            "   OR: utilities [opt...] wipe-dalvik-cache [ROM ID]\n"
            "   OR: utilities [opt...] wipe-multiboot [ROM ID]\n"
            "\n"
            "Options:\n"
            "  -f, --force      Force (only for 'switch' action)\n"
            "  -d, --devices    Path to device defintions file\n");
}

int utilities_main(int argc, char *argv[])
{
    // Make stdout unbuffered
    setvbuf(stdout, nullptr, _IONBF, 0);

    log::log_set_logger(std::make_shared<log::StdioLogger>(stdout, false));

    bool force = false;

    int opt;

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"force", no_argument, 0, 'f'},
        {"devices", required_argument, 0, 'd'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, "hfd:",
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'f':
            force = true;
            break;

        case 'd':
            devices_file = optarg;
            break;

        case 'h':
            utilities_usage(false);
            return EXIT_SUCCESS;

        default:
            utilities_usage(true);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind == 0) {
        utilities_usage(true);
        return EXIT_FAILURE;
    }

    const std::string action = argv[optind];
    if ((action == "generate" && argc - optind != 3)
            || (action != "generate" && argc - optind != 2)) {
        utilities_usage(true);
        return EXIT_FAILURE;
    }

    if (force && action != "switch") {
        utilities_usage(true);
        return EXIT_FAILURE;
    }

    bool ret = false;

    if (action == "generate") {
        AromaGenerator gen(argv[optind + 1], argv[optind + 2]);
        ret = gen.run();
    } else if (action == "switch") {
        ret = utilities_switch_rom(argv[optind + 1], force);
    } else if (action == "wipe-system") {
        ret = utilities_wipe_system(argv[optind + 1]);
    } else if (action == "wipe-cache") {
        ret = utilities_wipe_cache(argv[optind + 1]);
    } else if (action == "wipe-data") {
        ret = utilities_wipe_data(argv[optind + 1]);
    } else if (action == "wipe-dalvik-cache") {
        ret = utilities_wipe_dalvik_cache(argv[optind + 1]);
    } else if (action == "wipe-multiboot") {
        ret = utilities_wipe_multiboot(argv[optind + 1]);
    } else {
        LOGE("Unknown action: %s", action.c_str());
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}

}
