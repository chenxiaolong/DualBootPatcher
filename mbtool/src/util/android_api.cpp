/*
 * Copyright (C) 2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "util/android_api.h"

#include "mbcommon/common.h"

#include "mbutil/properties.h"

#include "util/multiboot.h"


namespace mb
{

/*!
 * \brief Get Android SDK version
 *
 * \return Android SDK version or 0 if the version could not be determined
 */
unsigned long get_sdk_version(SdkVersionSource source)
{
    switch (source) {
    case SdkVersionSource::Property:
        return util::property_file_get_num(BUILD_PROP_PATH, PROP_SDK_VERSION, 0ul);
    case SdkVersionSource::BuildProp:
        return util::property_get_num(PROP_SDK_VERSION, 0ul);
    default:
        MB_UNREACHABLE("Invalid source");
    }
}

}
