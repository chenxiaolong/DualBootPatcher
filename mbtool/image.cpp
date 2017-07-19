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

#include "image.h"

#include <inttypes.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "mbcommon/string.h"
#include "mblog/logging.h"
#include "mbutil/command.h"
#include "mbutil/directory.h"
#include "mbutil/mount.h"
#include "mbutil/path.h"
#include "mbutil/string.h"

namespace mb
{

static void output_cb(const char *line, bool error, void *userdata)
{
    (void) error;

    const char **args = static_cast<const char **>(userdata);
    LOGV("%s: %s", args[0], line);
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
            char size_str[64];
            snprintf(size_str, sizeof(size_str), "%" PRIu64, size);

            LOGD("%s: Creating new %s ext4 image", path.c_str(), size_str);

            // Create new image
            const char *argv[] =
                    { "make_ext4fs", "-l", size_str, path.c_str(), nullptr };
            int ret = util::run_command(argv[0], argv, nullptr, nullptr,
                                        &output_cb, argv);
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
    const char *argv[] = { "e2fsck", "-f", "-y", image.c_str(), nullptr };
    int ret = util::run_command(argv[0], argv, nullptr, nullptr,
                                &output_cb, argv);
    if (ret < 0 || (WEXITSTATUS(ret) != 0 && WEXITSTATUS(ret) != 1)) {
        LOGE("%s: Failed to e2fsck", image.c_str());
        return false;
    }
    return true;
}

}
