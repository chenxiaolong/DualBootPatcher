/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbdevice/device.h"

struct DeviceTest : testing::Test
{
    Device *_device;

    DeviceTest()
    {
        _device = mb_device_new();
    }

    virtual ~DeviceTest()
    {
        mb_device_free(_device);
    }
};

static bool string_array_eq(char const * const *a, char const * const *b)
{
    while (*a && *b) {
        if (strcmp(*a, *b) != 0) {
            return false;
        }
        ++a;
        ++b;
    }
    return !*a && !*b;
}

TEST_F(DeviceTest, CheckDefaultValues)
{
    ASSERT_EQ(mb_device_id(_device), nullptr);
    ASSERT_EQ(mb_device_codenames(_device), nullptr);
    ASSERT_EQ(mb_device_name(_device), nullptr);
    ASSERT_EQ(mb_device_architecture(_device), nullptr);
    ASSERT_EQ(mb_device_flags(_device), 0);
    ASSERT_EQ(mb_device_block_dev_base_dirs(_device), nullptr);
    ASSERT_EQ(mb_device_system_block_devs(_device), nullptr);
    ASSERT_EQ(mb_device_cache_block_devs(_device), nullptr);
    ASSERT_EQ(mb_device_data_block_devs(_device), nullptr);
    ASSERT_EQ(mb_device_boot_block_devs(_device), nullptr);
    ASSERT_EQ(mb_device_recovery_block_devs(_device), nullptr);
    ASSERT_EQ(mb_device_extra_block_devs(_device), nullptr);

    /* Boot UI */

    ASSERT_EQ(mb_device_tw_supported(_device), false);
    ASSERT_EQ(mb_device_tw_flags(_device), 0);
    ASSERT_EQ(mb_device_tw_pixel_format(_device), TW_PIXEL_FORMAT_DEFAULT);
    ASSERT_EQ(mb_device_tw_force_pixel_format(_device), TW_FORCE_PIXEL_FORMAT_NONE);
    ASSERT_EQ(mb_device_tw_overscan_percent(_device), 0);
    ASSERT_EQ(mb_device_tw_default_x_offset(_device), 0);
    ASSERT_EQ(mb_device_tw_default_y_offset(_device), 0);
    ASSERT_EQ(mb_device_tw_brightness_path(_device), nullptr);
    ASSERT_EQ(mb_device_tw_secondary_brightness_path(_device), nullptr);
    ASSERT_EQ(mb_device_tw_max_brightness(_device), -1);
    ASSERT_EQ(mb_device_tw_default_brightness(_device), -1);
    ASSERT_EQ(mb_device_tw_battery_path(_device), nullptr);
    ASSERT_EQ(mb_device_tw_cpu_temp_path(_device), nullptr);
    ASSERT_EQ(mb_device_tw_input_blacklist(_device), nullptr);
    ASSERT_EQ(mb_device_tw_input_whitelist(_device), nullptr);
    ASSERT_EQ(mb_device_tw_graphics_backends(_device), nullptr);
    ASSERT_EQ(mb_device_tw_theme(_device), nullptr);
}

TEST_F(DeviceTest, CheckGettersSetters)
{
    ASSERT_EQ(mb_device_set_id(_device, "test_id"), MB_DEVICE_OK);
    ASSERT_STREQ(mb_device_id(_device), "test_id");
    ASSERT_EQ(mb_device_set_id(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_id(_device), nullptr);

    const char *codenames[] = { "a", "b", "c", nullptr };
    ASSERT_EQ(mb_device_set_codenames(_device, codenames), MB_DEVICE_OK);
    ASSERT_TRUE(string_array_eq(mb_device_codenames(_device), codenames));
    ASSERT_EQ(mb_device_set_codenames(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_codenames(_device), nullptr);

    ASSERT_EQ(mb_device_set_name(_device, "test_name"), MB_DEVICE_OK);
    ASSERT_STREQ(mb_device_name(_device), "test_name");
    ASSERT_EQ(mb_device_set_name(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_name(_device), nullptr);

    ASSERT_EQ(mb_device_set_architecture(_device, "arm64-v8a"), MB_DEVICE_OK);
    ASSERT_STREQ(mb_device_architecture(_device), "arm64-v8a");
    ASSERT_EQ(mb_device_set_architecture(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_architecture(_device), nullptr);

    uint64_t device_flags = FLAG_HAS_COMBINED_BOOT_AND_RECOVERY;
    ASSERT_EQ(mb_device_set_flags(_device, device_flags), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_flags(_device), device_flags);

    const char *base_dirs[] = { "/dev/block/bootdevice/by-name", nullptr };
    ASSERT_EQ(mb_device_set_block_dev_base_dirs(_device, base_dirs), MB_DEVICE_OK);
    ASSERT_TRUE(string_array_eq(mb_device_block_dev_base_dirs(_device), base_dirs));
    ASSERT_EQ(mb_device_set_block_dev_base_dirs(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_block_dev_base_dirs(_device), nullptr);

    const char *system_devs[] = { "/dev/block/bootdevice/by-name/system", nullptr };
    ASSERT_EQ(mb_device_set_system_block_devs(_device, system_devs), MB_DEVICE_OK);
    ASSERT_TRUE(string_array_eq(mb_device_system_block_devs(_device), system_devs));
    ASSERT_EQ(mb_device_set_system_block_devs(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_system_block_devs(_device), nullptr);

    const char *cache_devs[] = { "/dev/block/bootdevice/by-name/cache", nullptr };
    ASSERT_EQ(mb_device_set_cache_block_devs(_device, cache_devs), MB_DEVICE_OK);
    ASSERT_TRUE(string_array_eq(mb_device_cache_block_devs(_device), cache_devs));
    ASSERT_EQ(mb_device_set_cache_block_devs(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_cache_block_devs(_device), nullptr);

    const char *data_devs[] = { "/dev/block/bootdevice/by-name/userdata", nullptr };
    ASSERT_EQ(mb_device_set_data_block_devs(_device, data_devs), MB_DEVICE_OK);
    ASSERT_TRUE(string_array_eq(mb_device_data_block_devs(_device), data_devs));
    ASSERT_EQ(mb_device_set_data_block_devs(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_data_block_devs(_device), nullptr);

    const char *boot_devs[] = { "/dev/block/bootdevice/by-name/boot", nullptr };
    ASSERT_EQ(mb_device_set_boot_block_devs(_device, boot_devs), MB_DEVICE_OK);
    ASSERT_TRUE(string_array_eq(mb_device_boot_block_devs(_device), boot_devs));
    ASSERT_EQ(mb_device_set_boot_block_devs(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_boot_block_devs(_device), nullptr);

    const char *recovery_devs[] = { "/dev/block/bootdevice/by-name/recovery", nullptr };
    ASSERT_EQ(mb_device_set_recovery_block_devs(_device, recovery_devs), MB_DEVICE_OK);
    ASSERT_TRUE(string_array_eq(mb_device_recovery_block_devs(_device), recovery_devs));
    ASSERT_EQ(mb_device_set_recovery_block_devs(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_recovery_block_devs(_device), nullptr);

    const char *extra_devs[] = { "/dev/block/bootdevice/by-name/modem", nullptr };
    ASSERT_EQ(mb_device_set_extra_block_devs(_device, extra_devs), MB_DEVICE_OK);
    ASSERT_TRUE(string_array_eq(mb_device_extra_block_devs(_device), extra_devs));
    ASSERT_EQ(mb_device_set_extra_block_devs(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_extra_block_devs(_device), nullptr);

    /* Boot UI */

    ASSERT_EQ(mb_device_set_tw_supported(_device, true), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_supported(_device), true);

    uint64_t flags = FLAG_TW_TOUCHSCREEN_FLIP_X
            | FLAG_TW_TOUCHSCREEN_FLIP_Y;
    ASSERT_EQ(mb_device_set_tw_flags(_device, flags), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_flags(_device), flags);

    enum TwPixelFormat pixel_format = TW_PIXEL_FORMAT_RGBA_8888;
    ASSERT_EQ(mb_device_set_tw_pixel_format(_device, pixel_format), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_pixel_format(_device), pixel_format);

    enum TwForcePixelFormat force_pixel_format = TW_FORCE_PIXEL_FORMAT_RGB_565;
    ASSERT_EQ(mb_device_set_tw_force_pixel_format(_device, force_pixel_format), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_force_pixel_format(_device), force_pixel_format);

    ASSERT_EQ(mb_device_set_tw_overscan_percent(_device, 10), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_overscan_percent(_device), 10);

    ASSERT_EQ(mb_device_set_tw_default_x_offset(_device, 20), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_default_x_offset(_device), 20);

    ASSERT_EQ(mb_device_set_tw_default_y_offset(_device, 30), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_default_y_offset(_device), 30);

    ASSERT_EQ(mb_device_set_tw_brightness_path(_device, "/tmp"), MB_DEVICE_OK);
    ASSERT_STREQ(mb_device_tw_brightness_path(_device), "/tmp");
    ASSERT_EQ(mb_device_set_tw_brightness_path(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_brightness_path(_device), nullptr);

    ASSERT_EQ(mb_device_set_tw_secondary_brightness_path(_device, "/sys"), MB_DEVICE_OK);
    ASSERT_STREQ(mb_device_tw_secondary_brightness_path(_device), "/sys");
    ASSERT_EQ(mb_device_set_tw_secondary_brightness_path(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_secondary_brightness_path(_device), nullptr);

    ASSERT_EQ(mb_device_set_tw_max_brightness(_device, 255), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_max_brightness(_device), 255);

    ASSERT_EQ(mb_device_set_tw_default_brightness(_device, 128), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_default_brightness(_device), 128);

    ASSERT_EQ(mb_device_set_tw_battery_path(_device, "/sys/module/battery"), MB_DEVICE_OK);
    ASSERT_STREQ(mb_device_tw_battery_path(_device), "/sys/module/battery");
    ASSERT_EQ(mb_device_set_tw_battery_path(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_battery_path(_device), nullptr);

    ASSERT_EQ(mb_device_set_tw_cpu_temp_path(_device, "/sys/module/coretemp"), MB_DEVICE_OK);
    ASSERT_STREQ(mb_device_tw_cpu_temp_path(_device), "/sys/module/coretemp");
    ASSERT_EQ(mb_device_set_tw_cpu_temp_path(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_cpu_temp_path(_device), nullptr);

    ASSERT_EQ(mb_device_set_tw_input_blacklist(_device, "/blah"), MB_DEVICE_OK);
    ASSERT_STREQ(mb_device_tw_input_blacklist(_device), "/blah");
    ASSERT_EQ(mb_device_set_tw_input_blacklist(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_input_blacklist(_device), nullptr);

    ASSERT_EQ(mb_device_set_tw_input_whitelist(_device, "/blah2"), MB_DEVICE_OK);
    ASSERT_STREQ(mb_device_tw_input_whitelist(_device), "/blah2");
    ASSERT_EQ(mb_device_set_tw_input_whitelist(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_input_whitelist(_device), nullptr);

    const char *graphics_backends[] = { "msm_overlay_old", "fbdev", nullptr };
    ASSERT_EQ(mb_device_set_tw_graphics_backends(_device, graphics_backends), MB_DEVICE_OK);
    ASSERT_TRUE(string_array_eq(mb_device_tw_graphics_backends(_device), graphics_backends));
    ASSERT_EQ(mb_device_set_tw_graphics_backends(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_graphics_backends(_device), nullptr);

    ASSERT_EQ(mb_device_set_tw_theme(_device, "portrait_hdpi"), MB_DEVICE_OK);
    ASSERT_STREQ(mb_device_tw_theme(_device), "portrait_hdpi");
    ASSERT_EQ(mb_device_set_tw_theme(_device, nullptr), MB_DEVICE_OK);
    ASSERT_EQ(mb_device_tw_theme(_device), nullptr);
}
