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

#include <memory>

#include <cstring>

#include <mbdevice/json.h>
#include <mblog/base_logger.h>
#include <mblog/logging.h>
#include <mbpatcher/patcherconfig.h>
#include <mbpatcher/patcherinterface.h>


class BasicLogger : public mb::log::BaseLogger
{
public:
    virtual void log(const mb::log::LogRecord &rec) override
    {
        printf("%s\n", rec.fmt_msg.c_str());
    }

    virtual bool formatted() override
    {
        return true;
    }
};

static bool file_read_all(const std::string &path,
                          std::vector<unsigned char> &data_out)
{
    FILE *fp = fopen(path.c_str(), "rb");
    if (!fp) {
        return false;
    }

    if (fseeko64(fp, 0, SEEK_END) < 0) {
        return false;
    }
    auto size = ftello64(fp);
    if (size < 0 || std::make_unsigned_t<decltype(size)>(size) > SIZE_MAX) {
        return false;
    }
    if (fseeko64(fp, 0, SEEK_SET) < 0) {
        return false;
    }

    std::vector<unsigned char> data(static_cast<size_t>(size));
    if (fread(data.data(), data.size(), 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    data_out.swap(data);

    fclose(fp);
    return true;
}

static bool get_device(const char *path, mb::device::Device &device)
{
    std::vector<unsigned char> contents;
    if (!file_read_all(path, contents)) {
        fprintf(stderr, "%s: Failed to read file: %s\n", path, strerror(errno));
        return false;
    }
    contents.push_back('\0');

    mb::device::JsonError error;

    if (!mb::device::device_from_json(
            reinterpret_cast<const char *>(contents.data()), device, error)) {
        fprintf(stderr, "%s: Failed to load devices\n", path);
        return false;
    }

    if (device.validate() != 0) {
        fprintf(stderr, "%s: Validation failed\n", path);
        return false;
    }

    return true;
}

static void mbp_progress_cb(uint64_t bytes, uint64_t maxBytes, void *userdata)
{
    (void) userdata;
    printf("Current bytes percentage: %.1f\n",
           100.0 * static_cast<double>(bytes) / static_cast<double>(maxBytes));
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <patcher id> <device file> <rom id> "
                "<input path> <output path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *patcher_id = argv[1];
    const char *device_file = argv[2];
    const char *rom_id = argv[3];
    const char *input_path = argv[4];
    const char *output_path = argv[5];

    mb::log::set_logger(std::make_shared<BasicLogger>());

    mb::device::Device device;

    if (!get_device(device_file, device)) {
        return EXIT_FAILURE;
    }

    mb::patcher::PatcherConfig pc;
    pc.set_data_directory("data");

    mb::patcher::FileInfo fi;
    fi.set_device(std::move(device));
    fi.set_input_path(input_path);
    fi.set_output_path(output_path);
    fi.set_rom_id(rom_id);

    auto *patcher = pc.create_patcher(patcher_id);
    if (!patcher) {
        fprintf(stderr, "Invalid patcher ID: %s\n", patcher_id);
        return EXIT_FAILURE;
    }

    patcher->set_file_info(&fi);
    bool ret = patcher->patch_file(&mbp_progress_cb, nullptr, nullptr, nullptr);

    if (!ret) {
        fprintf(stderr, "Error: %d\n", static_cast<int>(patcher->error()));
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
