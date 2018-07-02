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

#include "recovery/utilities.h"

#include <algorithm>

#include <cstring>

#include <fcntl.h>
#include <getopt.h>
#include <sys/stat.h>
#include <unistd.h>

#include "mz.h"
#include "mz_strm_android.h"
#include "mz_strm_buf.h"
#include "mz_zip.h"

#include "mbcommon/finally.h"
#include "mbcommon/string.h"
#include "mbcommon/version.h"
#include "mbdevice/device.h"
#include "mbdevice/json.h"
#include "mblog/logging.h"
#include "mblog/stdio_logger.h"
#include "mbutil/delete.h"
#include "mbutil/file.h"
#include "mbutil/fts.h"
#include "mbutil/path.h"
#include "mbutil/properties.h"
#include "mbutil/string.h"

#include "util/multiboot.h"
#include "util/romconfig.h"
#include "util/roms.h"
#include "util/switcher.h"
#include "util/wipe.h"

#define LOG_TAG "mbtool/recovery/utilities"

using namespace mb::device;

namespace mb
{

static const char *devices_file = nullptr;

static bool get_device(const char *path, Device &device)
{
    std::string prop_product_device =
            util::property_get_string("ro.product.device", {});
    std::string prop_build_product =
            util::property_get_string("ro.build.product", {});

    LOGD("ro.product.device = %s", prop_product_device.c_str());
    LOGD("ro.build.product = %s", prop_build_product.c_str());

    auto contents = util::file_read_all(path);
    if (!contents) {
        LOGE("%s: Failed to read file: %s", path,
             contents.error().message().c_str());
        return false;
    }

    std::vector<Device> devices;
    JsonError error;

    if (!device_list_from_json(contents.value(), devices, error)) {
        LOGE("%s: Failed to load devices", path);
        return false;
    }

    for (auto &d : devices) {
        if (d.validate()) {
            LOGW("Skipping invalid device");
            continue;
        }

        auto const &codenames = d.codenames();
        auto it = std::find_if(codenames.begin(), codenames.end(),
                               [&](const std::string &item) {
            return item == prop_product_device || item == prop_build_product;
        });

        if (it != codenames.end()) {
            device = std::move(d);
            return true;
        }
    }

    LOGE("Unknown device: %s", prop_product_device.c_str());
    return false;
}

static bool utilities_switch_rom(const char *rom_id, bool force)
{
    Device device;

    if (!devices_file) {
        LOGE("No device definitions file specified");
        return false;
    }

    if (!get_device(devices_file, device)) {
        LOGE("Failed to detect device");
        return false;
    }

    auto const &boot_devs = device.boot_block_devs();
    auto it = std::find_if(boot_devs.begin(), boot_devs.end(),
                           [&](const std::string &path) {
        struct stat sb;
        return stat(path.c_str(), &sb) == 0 && S_ISBLK(sb.st_mode);
    });

    if (it == boot_devs.end()) {
        LOGE("All specified boot partition paths could not be found");
        return false;
    }

    SwitchRomResult ret = switch_rom(
            rom_id, *it, device.block_dev_base_dirs(), force);
    switch (ret) {
    case SwitchRomResult::Succeeded:
        LOGD("SUCCEEDED");
        break;
    case SwitchRomResult::Failed:
        LOGD("FAILED");
        break;
    case SwitchRomResult::ChecksumInvalid:
        LOGD("CHECKSUM_INVALID");
        break;
    case SwitchRomResult::ChecksumNotFound:
        LOGD("CHECKSUM_NOT_FOUND");
        break;
    }

    return ret == SwitchRomResult::Succeeded;
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

static void generate_aroma_config(std::string &data)
{
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

        rom_menu_items += format("\"%s\", \"\", \"@default\",\n", name.c_str());

        rom_selection_items += format(
                "if prop(\"operations.prop\", \"selected\") == \"%zu\" then\n"
                "    setvar(\"romid\", \"%s\");\n"
                "    setvar(\"romname\", \"%s\");\n"
                "endif;\n",
                i + 2 + 1, rom->id.c_str(), name.c_str());
    }

    std::string first_index = format("%d", 2 + 1);
    std::string last_index = format("%zu", 2 + roms.roms.size());

    util::replace_all(data, "\t", "\\t");
    util::replace_all(data, "@MBTOOL_VERSION@", version());
    util::replace_all(data, "@ROM_MENU_ITEMS@", rom_menu_items);
    util::replace_all(data, "@ROM_SELECTION_ITEMS@", rom_selection_items);
    util::replace_all(data, "@FIRST_INDEX@", first_index);
    util::replace_all(data, "@LAST_INDEX@", last_index);

    util::replace_all(data, "@SYSTEM_MOUNT_POINT@", Roms::get_system_partition());
    util::replace_all(data, "@CACHE_MOUNT_POINT@", Roms::get_cache_partition());
    util::replace_all(data, "@DATA_MOUNT_POINT@", Roms::get_data_partition());
    util::replace_all(data, "@EXTSD_MOUNT_POINT@", Roms::get_extsd_partition());
}

class AromaGenerator : public util::FtsWrapper
{
public:
    AromaGenerator(std::string path, std::string zippath)
        : FtsWrapper(std::move(path), util::FtsFlag::GroupSpecialFiles),
        _zippath(std::move(zippath))
    {
    }

    bool on_pre_execute() override
    {
        if (!mz_stream_android_create(&_stream)) {
            LOGE("Failed to create base stream");
            return false;
        }

        auto destroy_stream = finally([&] {
            mz_stream_delete(&_stream);
        });

        if (!mz_stream_buffered_create(&_buf_stream)) {
            LOGE("Failed to create buffered stream");
            return false;
        }

        auto destroy_buf_stream = finally([&] {
            mz_stream_delete(&_buf_stream);
        });

        if (mz_stream_set_base(_buf_stream, _stream) != MZ_OK) {
            LOGD("Failed to set base stream for buffered stream");
            return false;
        }

        if (mz_stream_open(_buf_stream, _zippath.c_str(),
                           MZ_OPEN_MODE_READWRITE | MZ_OPEN_MODE_CREATE) != MZ_OK) {
            LOGE("%s: Failed to open stream", _zippath.c_str());
            return false;
        }

        auto close_stream = finally([&] {
            mz_stream_close(_buf_stream);
        });

        _handle = mz_zip_open(_buf_stream, MZ_OPEN_MODE_WRITE);
        if (!_handle) {
            LOGE("%s: Failed to open zip", _zippath.c_str());
            return false;
        }

        auto close_zip = finally([&]{
            mz_zip_close(_handle);
        });

        close_zip.dismiss();
        close_stream.dismiss();
        destroy_buf_stream.dismiss();
        destroy_stream.dismiss();

        return true;
    }

    bool on_post_execute(bool success) override
    {
        (void) success;
        bool ret = mz_zip_close(_handle) == MZ_OK
                && mz_stream_close(_buf_stream) == MZ_OK;
        mz_stream_delete(&_buf_stream);
        mz_stream_delete(&_stream);
        return ret;
    }

    Actions on_reached_file() override
    {
        std::string name = std::string(_curr->fts_path).substr(_path.size() + 1);
        LOGD("%s -> %s", _curr->fts_path, name.c_str());

        if (name == "META-INF/com/google/android/aroma-config.in") {
            auto data = util::file_read_all(_curr->fts_accpath);
            if (!data) {
                LOGE("Failed to read: %s: %s", _curr->fts_path,
                     data.error().message().c_str());
                return Action::Fail;
            }

            generate_aroma_config(data.value());

            name = "META-INF/com/google/android/aroma-config";
            bool ret = add_file_from_data(name, data.value());
            return ret ? Action::Ok : Action::Fail;
        } else {
            bool ret = add_file_from_path(name, _curr->fts_accpath);
            return ret ? Action::Ok : Action::Fail;
        }
    }

    Actions on_reached_symlink() override
    {
        LOGW("Ignoring symlink when creating zip: %s", _curr->fts_path);
        return Action::Ok;
    }

    Actions on_reached_special_file() override
    {
        LOGW("Ignoring special file when creating zip: %s", _curr->fts_path);
        return Action::Ok;
    }

private:
    void *_stream;
    void *_buf_stream;
    void *_handle;
    std::string _zippath;

    bool add_file_from_data(const std::string &name, const std::string &data)
    {
        mz_zip_file file_info = {};
        file_info.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
        file_info.filename = name.c_str();
        file_info.filename_size = static_cast<uint16_t>(name.size());

        int ret = mz_zip_entry_write_open(_handle, &file_info,
                                          MZ_COMPRESS_LEVEL_DEFAULT, nullptr);
        if (ret != MZ_OK) {
            LOGW("minizip: Failed to add file (error code: %d): [memory]", ret);
            return false;
        }

        auto close_inner_write = finally([&] {
            mz_zip_entry_close(_handle);
        });

        // Write data to file
        int n = mz_zip_entry_write(_handle, data.data(),
                                   static_cast<uint32_t>(data.size()));
        if (n < 0 || static_cast<size_t>(n) != data.size()) {
            LOGW("minizip: Failed to write data (error code: %d): [memory]", ret);
            return false;
        }

        close_inner_write.dismiss();

        ret = mz_zip_entry_close(_handle);
        if (ret != MZ_OK) {
            LOGE("minizip: Failed to close inner file (error code: %d): [memory]", ret);
            return false;
        }

        return true;
    }

    bool add_file_from_path(const std::string &name, const std::string &path)
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

        mz_zip_file file_info = {};
        file_info.compression_method = MZ_COMPRESS_METHOD_DEFLATE;
        file_info.filename = name.c_str();
        file_info.filename_size = static_cast<uint16_t>(name.size());
        file_info.external_fa = (sb.st_mode & 0777) << 16;

        int  ret = mz_zip_entry_write_open(_handle, &file_info,
                                           MZ_COMPRESS_LEVEL_DEFAULT, nullptr);
        if (ret != MZ_OK) {
            LOGW("minizip: Failed to add file (error code: %d): %s",
                 ret, path.c_str());
            return false;
        }

        auto close_inner_write = finally([&] {
            mz_zip_entry_close(_handle);
        });

        // Write data to file
        char buf[UINT16_MAX];
        ssize_t n;

        while ((n = read(fd, buf, sizeof(buf))) > 0) {
            auto n_written = mz_zip_entry_write(
                    _handle, buf, static_cast<uint32_t>(n));
            if (static_cast<int>(n) != n_written) {
                LOGW("minizip: Failed to write data: %s", path.c_str());
                return false;
            }
        }

        if (n < 0) {
            LOGE("%s: Failed to read file: %s",
                 path.c_str(), strerror(errno));
            return false;
        }

        close_inner_write.dismiss();

        ret = mz_zip_entry_close(_handle);
        if (ret != MZ_OK) {
            LOGE("minizip: Failed to close inner file: %s", path.c_str());
            return false;
        }

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

    log::set_logger(std::make_shared<log::StdioLogger>(stdout));

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
