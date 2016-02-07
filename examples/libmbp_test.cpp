/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <cassert>

#include <mblog/logging.h>
#include <mbp/patcherconfig.h>
#include <mbp/patcherinterface.h>


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

static void mbp_progress_cb(uint64_t bytes, uint64_t maxBytes, void *userdata)
{
    (void) userdata;
    printf("Current bytes percentage: %.1f\n", 100.0 * bytes / maxBytes);
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <patcher id> <device> <rom id> "
                "<input path> <output path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *patcher_id = argv[1];
    const char *device_name = argv[2];
    const char *rom_id = argv[3];
    const char *input_path = argv[4];
    const char *output_path = argv[5];

    mb::log::log_set_logger(std::make_shared<BasicLogger>());

    mbp::PatcherConfig pc;
    pc.setDataDirectory("data");

    mbp::Device *device = nullptr;

    for (mbp::Device *d : pc.devices()) {
        if (d->id() == device_name) {
            device = d;
            break;
        }
    }

    if (!device) {
        fprintf(stderr, "Invalid device: %s\n", device_name);
        return EXIT_FAILURE;
    }

    mbp::FileInfo fi;
    fi.setDevice(device);
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