/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <gtest/gtest.h>

#include "mbdevice/flags.h"

#include "mbdevice/capi/flags.h"

#define TO_U(TYPE, VALUE) \
    static_cast<std::underlying_type_t<TYPE>>(TYPE::VALUE)

using namespace mb::device;

TEST(FlagsTest, CheckCapiFlagsEqual)
{
    ASSERT_STREQ(ARCH_ARMEABI_V7A, MB_DEVICE_ARCH_ARMEABI_V7A);
    ASSERT_STREQ(ARCH_ARM64_V8A, MB_DEVICE_ARCH_ARM64_V8A);
    ASSERT_STREQ(ARCH_X86, MB_DEVICE_ARCH_X86);
    ASSERT_STREQ(ARCH_X86_64, MB_DEVICE_ARCH_X86_64);

    ASSERT_EQ(TO_U(DeviceFlag, HasCombinedBootAndRecovery),
              MB_DEVICE_FLAG_HAS_COMBINED_BOOT_AND_RECOVERY);
    ASSERT_EQ(TO_U(DeviceFlag, FstabSkipSdcard0),
              MB_DEVICE_FLAG_FSTAB_SKIP_SDCARD0);
    ASSERT_EQ(DEVICE_FLAG_MASK, MB_DEVICE_FLAG_MASK);

    ASSERT_EQ(TO_U(TwPixelFormat, Default),
              MB_DEVICE_TW_PXFMT_DEFAULT);
    ASSERT_EQ(TO_U(TwPixelFormat, Abgr8888),
              MB_DEVICE_TW_PXFMT_ABGR_8888);
    ASSERT_EQ(TO_U(TwPixelFormat, Rgbx8888),
              MB_DEVICE_TW_PXFMT_RGBX_8888);
    ASSERT_EQ(TO_U(TwPixelFormat, Bgra8888),
              MB_DEVICE_TW_PXFMT_BGRA_8888);
    ASSERT_EQ(TO_U(TwPixelFormat, Rgba8888),
              MB_DEVICE_TW_PXFMT_RGBA_8888);

    ASSERT_EQ(TO_U(ValidateFlag, MissingId),
              MB_DEVICE_V_FLAG_MISSING_ID);
    ASSERT_EQ(TO_U(ValidateFlag, MissingCodenames),
              MB_DEVICE_V_FLAG_MISSING_CODENAMES);
    ASSERT_EQ(TO_U(ValidateFlag, MissingName),
              MB_DEVICE_V_FLAG_MISSING_NAME);
    ASSERT_EQ(TO_U(ValidateFlag, MissingArchitecture),
              MB_DEVICE_V_FLAG_MISSING_ARCHITECTURE);
    ASSERT_EQ(TO_U(ValidateFlag, MissingSystemBlockDevs),
              MB_DEVICE_V_FLAG_MISSING_SYSTEM_BLOCK_DEVS);
    ASSERT_EQ(TO_U(ValidateFlag, MissingCacheBlockDevs),
              MB_DEVICE_V_FLAG_MISSING_CACHE_BLOCK_DEVS);
    ASSERT_EQ(TO_U(ValidateFlag, MissingDataBlockDevs),
              MB_DEVICE_V_FLAG_MISSING_DATA_BLOCK_DEVS);
    ASSERT_EQ(TO_U(ValidateFlag, MissingBootBlockDevs),
              MB_DEVICE_V_FLAG_MISSING_BOOT_BLOCK_DEVS);
    ASSERT_EQ(TO_U(ValidateFlag, MissingRecoveryBlockDevs),
              MB_DEVICE_V_FLAG_MISSING_RECOVERY_BLOCK_DEVS);
    ASSERT_EQ(TO_U(ValidateFlag, InvalidArchitecture),
              MB_DEVICE_V_FLAG_INVALID_ARCHITECTURE);
    ASSERT_EQ(TO_U(ValidateFlag, InvalidFlags),
              MB_DEVICE_V_FLAG_INVALID_FLAGS);
}
