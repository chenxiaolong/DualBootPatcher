/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "image.h"

#include <cerrno>
#include <cinttypes>
#include <cstring>

#include <sys/stat.h>
#include <sys/wait.h>

#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/command.h"
#include "mbutil/directory.h"
#include "mbutil/mount.h"
#include "mbutil/path.h"
#include "mbutil/string.h"

#define LOG_TAG "mbtool/image"

namespace mb
{

static void output_cb(const char *line, bool error, void *userdata)
{
    (void) error;

    auto *args = static_cast<std::vector<std::string> *>(userdata);
    LOGV("%s: %s", (*args)[0].c_str(), line);
}

CreateImageResult create_ext4_image(const std::string &path, uint64_t size)
{
    // Ensure we have enough space since we're creating a sparse file that may
    // get bigger
    if (auto avail = util::mount_get_avail_size(util::dir_name(path)); !avail) {
        LOGE("%s: Failed to get available space: %s", path.c_str(),
             avail.error().message().c_str());
        return CreateImageResult::Failed;
    } else if (avail.value() < size) {
        LOGE("There is not enough space to create %s", path.c_str());
        LOGE("- Needed:    %" PRIu64 " bytes", size);
        LOGE("- Available: %" PRIu64 " bytes", avail.value());
        return CreateImageResult::NotEnoughSpace;
    }

    struct stat sb;
    if (stat(path.c_str(), &sb) < 0) {
        if (errno != ENOENT) {
            LOGE("%s: Failed to stat: %s", path.c_str(), strerror(errno));
            return CreateImageResult::Failed;
        } else {
            char size_str[64];
            snprintf(size_str, sizeof(size_str), "%" PRIu64, size);

            LOGD("%s: Creating new %s ext4 image", path.c_str(), size_str);

            // Create new image
            std::vector<std::string> argv{
                "make_ext4fs", "-l", size_str, path
            };
            int ret = util::run_command(argv[0], argv, {}, {}, &output_cb,
                                        &argv);
            if (ret < 0 || WEXITSTATUS(ret) != 0) {
                LOGE("%s: Failed to create image", path.c_str());
                return CreateImageResult::Failed;
            }
            return CreateImageResult::Succeeded;
        }
    }

    LOGE("%s: File already exists", path.c_str());
    return CreateImageResult::ImageExists;
}

bool fsck_ext4_image(const std::string &image)
{
    std::vector<std::string> argv{ "e2fsck", "-f", "-y", image };
    int ret = util::run_command(argv[0], argv, {}, {}, &output_cb, &argv);
    if (ret < 0 || (WEXITSTATUS(ret) != 0 && WEXITSTATUS(ret) != 1)) {
        LOGE("%s: Failed to e2fsck", image.c_str());
        return false;
    }
    return true;
}

}
