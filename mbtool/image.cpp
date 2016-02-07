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

#include "image.h"

#include <inttypes.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "mblog/logging.h"
#include "mbutil/command.h"
#include "mbutil/directory.h"
#include "mbutil/mount.h"
#include "mbutil/path.h"
#include "mbutil/string.h"

namespace mb
{

static void output_cb(const std::string &msg, void *data)
{
    std::vector<std::string> *args = (std::vector<std::string> *) data;
    LOGV("%s: %s", (*args)[0].c_str(), msg.c_str());
}

CreateImageResult create_ext4_image(const std::string &path, uint64_t size)
{
    // Ensure we have enough space since we're creating a sparse file that may
    // get bigger
    uint64_t avail = util::mount_get_avail_size(util::dir_name(path).c_str());
    if (avail < size) {
        LOGE("There is not enough space to create %s", path.c_str());
        LOGE("- Needed:    %" PRIu64 " bytes", size);
        LOGE("- Available: %" PRIu64 " bytes", avail);
        return CreateImageResult::NOT_ENOUGH_SPACE;
    }

    struct stat sb;
    if (stat(path.c_str(), &sb) < 0) {
        if (errno != ENOENT) {
            LOGE("%s: Failed to stat: %s", path.c_str(), strerror(errno));
            return CreateImageResult::FAILED;
        } else {
            std::string size_str = util::format("%" PRIu64, size);

            LOGD("%s: Creating new %s ext4 image", path.c_str(), size_str.c_str());

            // Create new image
            std::vector<std::string> args{ "make_ext4fs", "-l", size_str, path };
            int ret = util::run_command_cb(args, output_cb, nullptr);
            if (ret < 0 || WEXITSTATUS(ret) != 0) {
                LOGE("%s: Failed to create image", path.c_str());
                return CreateImageResult::FAILED;
            }
            return CreateImageResult::SUCCEEDED;
        }
    }

    LOGE("%s: File already exists", path.c_str());
    return CreateImageResult::IMAGE_EXISTS;
}

bool fsck_ext4_image(const std::string &image)
{
    std::vector<std::string> args{ "e2fsck", "-f", "-y", image };
    int ret = util::run_command_cb(args, output_cb, &args);
    if (ret < 0 || (WEXITSTATUS(ret) != 0 && WEXITSTATUS(ret) != 1)) {
        LOGE("%s: Failed to e2fsck", image.c_str());
        return false;
    }
    return true;
}

}