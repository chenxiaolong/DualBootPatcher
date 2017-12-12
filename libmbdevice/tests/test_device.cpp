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

#include "mbdevice/device.h"

using namespace mb::device;

TEST(DeviceTest, CheckDefaultValues)
{
    Device device;

    ASSERT_TRUE(device.id().empty());
    ASSERT_TRUE(device.codenames().empty());
    ASSERT_TRUE(device.name().empty());
    ASSERT_TRUE(device.architecture().empty());
    ASSERT_FALSE(device.flags());
    ASSERT_TRUE(device.block_dev_base_dirs().empty());
    ASSERT_TRUE(device.system_block_devs().empty());
    ASSERT_TRUE(device.cache_block_devs().empty());
    ASSERT_TRUE(device.data_block_devs().empty());
    ASSERT_TRUE(device.boot_block_devs().empty());
    ASSERT_TRUE(device.recovery_block_devs().empty());
    ASSERT_TRUE(device.extra_block_devs().empty());

    /* Boot UI */

    ASSERT_FALSE(device.tw_supported());
    ASSERT_FALSE(device.tw_flags());
    ASSERT_EQ(device.tw_pixel_format(), TwPixelFormat::Default);
    ASSERT_EQ(device.tw_force_pixel_format(), TwForcePixelFormat::None);
    ASSERT_EQ(device.tw_overscan_percent(), 0);
    ASSERT_EQ(device.tw_default_x_offset(), 0);
    ASSERT_EQ(device.tw_default_y_offset(), 0);
    ASSERT_TRUE(device.tw_brightness_path().empty());
    ASSERT_TRUE(device.tw_secondary_brightness_path().empty());
    ASSERT_EQ(device.tw_max_brightness(), -1);
    ASSERT_EQ(device.tw_default_brightness(), -1);
    ASSERT_TRUE(device.tw_battery_path().empty());
    ASSERT_TRUE(device.tw_cpu_temp_path().empty());
    ASSERT_TRUE(device.tw_input_blacklist().empty());
    ASSERT_TRUE(device.tw_input_whitelist().empty());
    ASSERT_TRUE(device.tw_graphics_backends().empty());
    ASSERT_TRUE(device.tw_theme().empty());
}

TEST(DeviceTest, CheckGettersSetters)
{
    Device device;

    device.set_id("test_id");
    ASSERT_EQ(device.id(), "test_id");

    std::vector<std::string> codenames{"a", "b", "c"};
    device.set_codenames(codenames);
    ASSERT_EQ(device.codenames(), codenames);

    device.set_name("test_name");
    ASSERT_EQ(device.name(), "test_name");

    device.set_architecture(ARCH_ARM64_V8A);
    ASSERT_EQ(device.architecture(), ARCH_ARM64_V8A);

    DeviceFlags device_flags = DeviceFlag::HasCombinedBootAndRecovery;
    device.set_flags(device_flags);
    ASSERT_EQ(device.flags(), device_flags);

    std::vector<std::string> base_dirs{"/dev/block/bootdevice/by-name"};
    device.set_block_dev_base_dirs(base_dirs);
    ASSERT_EQ(device.block_dev_base_dirs(), base_dirs);

    std::vector<std::string> system_devs{"/dev/block/bootdevice/by-name/system"};
    device.set_system_block_devs(system_devs);
    ASSERT_EQ(device.system_block_devs(), system_devs);

    std::vector<std::string> cache_devs{"/dev/block/bootdevice/by-name/cache"};
    device.set_cache_block_devs(cache_devs);
    ASSERT_EQ(device.cache_block_devs(), cache_devs);

    std::vector<std::string> data_devs{"/dev/block/bootdevice/by-name/userdata"};
    device.set_data_block_devs(data_devs);
    ASSERT_EQ(device.data_block_devs(), data_devs);

    std::vector<std::string> boot_devs{"/dev/block/bootdevice/by-name/boot"};
    device.set_boot_block_devs(boot_devs);
    ASSERT_EQ(device.boot_block_devs(), boot_devs);

    std::vector<std::string> recovery_devs{"/dev/block/bootdevice/by-name/recovery"};
    device.set_recovery_block_devs(recovery_devs);
    ASSERT_EQ(device.recovery_block_devs(), recovery_devs);

    std::vector<std::string> extra_devs{"/dev/block/bootdevice/by-name/modem"};
    device.set_extra_block_devs(extra_devs);
    ASSERT_EQ(device.extra_block_devs(), extra_devs);

    /* Boot UI */

    device.set_tw_supported(true);
    ASSERT_TRUE(device.tw_supported());

    TwFlags flags = TwFlag::TouchscreenFlipX | TwFlag::TouchscreenFlipY;
    device.set_tw_flags(flags);
    ASSERT_EQ(device.tw_flags(), flags);

    TwPixelFormat pixel_format = TwPixelFormat::Rgba8888;
    device.set_tw_pixel_format(pixel_format);
    ASSERT_EQ(device.tw_pixel_format(), pixel_format);

    TwForcePixelFormat force_pixel_format = TwForcePixelFormat::Rgb565;
    device.set_tw_force_pixel_format(force_pixel_format);
    ASSERT_EQ(device.tw_force_pixel_format(), force_pixel_format);

    device.set_tw_overscan_percent(10);
    ASSERT_EQ(device.tw_overscan_percent(), 10);

    device.set_tw_default_x_offset(20);
    ASSERT_EQ(device.tw_default_x_offset(), 20);

    device.set_tw_default_y_offset(30);
    ASSERT_EQ(device.tw_default_y_offset(), 30);

    device.set_tw_brightness_path("/tmp");
    ASSERT_EQ(device.tw_brightness_path(), "/tmp");

    device.set_tw_secondary_brightness_path("/sys");
    ASSERT_EQ(device.tw_secondary_brightness_path(), "/sys");

    device.set_tw_max_brightness(255);
    ASSERT_EQ(device.tw_max_brightness(), 255);

    device.set_tw_default_brightness(128);
    ASSERT_EQ(device.tw_default_brightness(), 128);

    device.set_tw_battery_path("/sys/module/battery");
    ASSERT_EQ(device.tw_battery_path(), "/sys/module/battery");

    device.set_tw_cpu_temp_path("/sys/module/coretemp");
    ASSERT_EQ(device.tw_cpu_temp_path(), "/sys/module/coretemp");

    device.set_tw_input_blacklist("/blah");
    ASSERT_EQ(device.tw_input_blacklist(), "/blah");

    device.set_tw_input_whitelist("/blah2");
    ASSERT_EQ(device.tw_input_whitelist(), "/blah2");

    std::vector<std::string> graphics_backends{"msm_overlay_old", "fbdev"};
    device.set_tw_graphics_backends(graphics_backends);
    ASSERT_EQ(device.tw_graphics_backends(), graphics_backends);

    device.set_tw_theme("portrait_hdpi");
    ASSERT_EQ(device.tw_theme(), "portrait_hdpi");
}
