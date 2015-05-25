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

#pragma once

#include "libmbpio/private/common.h"

#if IO_PLATFORM_WINDOWS
#include "libmbpio/win32/file.h"
#elif IO_PLATFORM_ANDROID
#include "libmbpio/android/file.h"
#elif IO_PLATFORM_POSIX
#include "libmbpio/posix/file.h"
#endif

namespace io
{

#if IO_PLATFORM_WINDOWS
typedef win32::FileWin32 File;
#elif IO_PLATFORM_ANDROID
typedef android::FileAndroid File;
#elif IO_PLATFORM_POSIX
typedef posix::FilePosix File;
#endif

}
