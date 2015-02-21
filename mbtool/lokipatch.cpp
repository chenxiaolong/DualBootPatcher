/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "lokipatch.h"

#include <unistd.h>

extern "C" {
#include <loki.h>
}

#include "util/copy.h"
#include "util/logging.h"

#define ABOOT_PARTITION "/dev/block/platform/msm_sdcc.1/by-name/aboot"

namespace mb {

bool loki_patch_file(const std::string &source, const std::string &target)
{
    std::string aboot(target);
    aboot += ".aboot";

    if (!util::copy_contents(ABOOT_PARTITION, aboot)) {
        LOGE("Failed to copy aboot partition");
        return false;
    }

    if (loki_patch("boot", aboot.c_str(),
                   source.c_str(), target.c_str()) != 0) {
        LOGE("Failed to run loki");
        return false;
    }

    unlink(aboot.c_str());

    return true;
}

}