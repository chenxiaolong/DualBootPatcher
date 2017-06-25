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
#include <mbdevice/validate.h>
#include <mblog/logging.h>
#include <mbp/patcherconfig.h>
#include <mbp/patcherinterface.h>


typedef std::unique_ptr<Device, void (*)(Device *)> ScopedDevice;

class BasicLogger : public mb::log::BaseLogger
{
public:
    virtual void log(mb::log::LogLevel prio, const char *fmt, va_list ap) override
    {
        (void) prio;
        vprintf(fmt, ap);
        printf("\n");
    }
};

static bool file_read_all(const std::string &path,
                          std::vector<unsigned char> *data_out)
{
    FILE *fp = fopen(path.c_str(), "rb");
    if (!fp) {
        return false;
    }

    fseek(fp, 0, SEEK_END);
    auto size = ftell(fp);
    rewind(fp);

    std::vector<unsigned char> data(size);
    if (fread(data.data(), size, 1, fp) != 1) {
        fclose(fp);
        return false;
    }

    data_out->swap(data);

    fclose(fp);
    return true;
}

static Device * get_device(const char *path)
{
    std::vector<unsigned char> contents;
    if (!file_read_all(path, &contents)) {
        fprintf(stderr, "%s: Failed to read file: %s\n", path, strerror(errno));
        return nullptr;
    }
    contents.push_back('\0');

    MbDeviceJsonError error;
    Device *device = mb_device_new_from_json(
            (const char *) contents.data(), &error);
    if (!device) {
        fprintf(stderr, "%s: Failed to load devices\n", path);
        return nullptr;
    }

    if (mb_device_validate(device) != 0) {
        fprintf(stderr, "%s: Validation failed\n", path);
        mb_device_free(device);
        return nullptr;
    }

    return device;
}

static void mbp_progress_cb(uint64_t bytes, uint64_t maxBytes, void *userdata)
{
    (void) userdata;
    printf("Current bytes percentage: %.1f\n", 100.0 * bytes / maxBytes);
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

    mb::log::log_set_logger(std::make_shared<BasicLogger>());

    ScopedDevice device(get_device(device_file), mb_device_free);
    if (!device) {
        return EXIT_FAILURE;
    }

    mbp::PatcherConfig pc;
    pc.setDataDirectory("data");

    mbp::FileInfo fi;
    fi.setDevice(device.get());
    fi.setInputPath(input_path);
    fi.setOutputPath(output_path);
    fi.setRomId(rom_id);

    mbp::Patcher *patcher = pc.createPatcher(patcher_id);
    if (!patcher) {
        fprintf(stderr, "Invalid patcher ID: %s\n", patcher_id);
        return EXIT_FAILURE;
    }

    patcher->setFileInfo(&fi);
    bool ret = patcher->patchFile(&mbp_progress_cb, nullptr, nullptr, nullptr);

    if (!ret) {
        fprintf(stderr, "Error: %d\n", static_cast<int>(patcher->error()));
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
