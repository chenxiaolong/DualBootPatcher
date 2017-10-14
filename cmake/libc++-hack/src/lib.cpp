/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <dlfcn.h>

// Hack to fix linking issues with libunwind. We don't actually care if
// libunwind works.
//
// [ 45%] Linking CXX executable mbtool
// /usr/local/google/buildbot/src/android/ndk-r15-release/external/libcxx/../../external/libunwind_llvm/src/AddressSpace.hpp:467: error: undefined reference to 'dladdr'
// clang++: error: linker command failed with exit code 1 (use -v to see invocation)
extern "C"
{

int dladdr(const void *addr, Dl_info *info)
{
    (void) addr;
    (void) info;
    return 0;
}

}
