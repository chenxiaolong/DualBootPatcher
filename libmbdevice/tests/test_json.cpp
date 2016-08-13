/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <gtest/gtest.h>

#include "mbdevice/device.h"
#include "mbdevice/json.h"

static const char sample_complete[] =
    "{"
        "\"name\": \"Test Device\","
        "\"id\": \"test\","
        "\"codenames\": ["
            "\"test1\","
            "\"test2\","
            "\"test3\","
            "\"test4\""
        "],"
        "\"architecture\": \"arm64-v8a\","
        "\"block_devs\": {"
            "\"base_dirs\": ["
                "\"/dev/block/bootdevice/by-name\""
            "],"
            "\"system\": ["
                "\"/dev/block/bootdevice/by-name/system\","
                "\"/dev/block/sda1\""
            "],"
            "\"cache\": ["
                "\"/dev/block/bootdevice/by-name/cache\","
                "\"/dev/block/sda2\""
            "],"
            "\"data\": ["
                "\"/dev/block/bootdevice/by-name/userdata\","
                "\"/dev/block/sda3\""
            "],"
            "\"boot\": ["
                "\"/dev/block/bootdevice/by-name/boot\","
                "\"/dev/block/sda4\""
            "],"
            "\"recovery\": ["
                "\"/dev/block/bootdevice/by-name/recovery\","
                "\"/dev/block/sda5\""
            "],"
            "\"extra\": ["
                "\"/dev/block/bootdevice/by-name/modem\","
                "\"/dev/block/sda6\""
            "]"
        "},"
        "\"boot_ui\": {"
            "\"supported\": true,"
            "\"flags\": ["
                "\"TW_TOUCHSCREEN_SWAP_XY\","
                "\"TW_TOUCHSCREEN_FLIP_X\","
                "\"TW_TOUCHSCREEN_FLIP_Y\","
                "\"TW_GRAPHICS_FORCE_USE_LINELENGTH\","
                "\"TW_SCREEN_BLANK_ON_BOOT\","
                "\"TW_BOARD_HAS_FLIPPED_SCREEN\","
                "\"TW_IGNORE_MAJOR_AXIS_0\","
                "\"TW_IGNORE_MT_POSITION_0\","
                "\"TW_IGNORE_ABS_MT_TRACKING_ID\","
                "\"TW_NEW_ION_HEAP\","
                "\"TW_NO_SCREEN_BLANK\","
                "\"TW_NO_SCREEN_TIMEOUT\","
                "\"TW_ROUND_SCREEN\","
                "\"TW_NO_CPU_TEMP\","
                "\"TW_QCOM_RTC_FIX\","
                "\"TW_HAS_DOWNLOAD_MODE\","
                "\"TW_PREFER_LCD_BACKLIGHT\""
            "],"
            "\"pixel_format\": \"RGBA_8888\","
            "\"force_pixel_format\": \"RGB_565\","
            "\"overscan_percent\": 10,"
            "\"default_x_offset\": 20,"
            "\"default_y_offset\": 30,"
            "\"brightness_path\": \"/sys/class/backlight\","
            "\"secondary_brightness_path\": \"/sys/class/lcd-backlight\","
            "\"max_brightness\": 255,"
            "\"default_brightness\": 100,"
            "\"battery_path\": \"/sys/class/battery\","
            "\"cpu_temp_path\": \"/sys/class/cputemp\","
            "\"input_blacklist\": \"foo\","
            "\"input_whitelist\": \"bar\","
            "\"graphics_backends\": ["
                "\"overlay_msm_old\","
                "\"fbdev\""
            "],"
            "\"theme\": \"portrait_hdpi\""
        "},"
        "\"crypto\": {"
            "\"supported\": true,"
            "\"header_path\": \"footer\""
        "}"
    "}";

static const char sample_invalid_root[] =
    "["
        "\"foo\","
        "\"bar\""
    "]";

static const char sample_invalid_key[] =
    "{"
        "\"foo\": \"bar\""
    "}";

static const char sample_invalid_tw_flags[] =
    "{"
        "\"boot_ui\": {"
            "\"flags\": ["
                "\"TW_FOO_BAR\""
            "]"
        "}"
    "}";

static const char sample_invalid_tw_pixel_format[] =
    "{"
        "\"boot_ui\": {"
            "\"pixel_format\": \"FOO_BAR\""
        "}"
    "}";

static const char sample_invalid_tw_force_pixel_format[] =
    "{"
        "\"boot_ui\": {"
            "\"force_pixel_format\": \"FOO_BAR\""
        "}"
    "}";

static const char sample_invalid_type[] =
    "{"
        "\"boot_ui\": \"FOO_BAR\""
    "}";

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

TEST_F(DeviceTest, TestJsonComplete)
{
    MbDeviceJsonError error;
    ASSERT_EQ(mb_device_load_json(_device, sample_complete, &error),
              MB_DEVICE_OK);

    ASSERT_STREQ(mb_device_id(_device), "test");

    const char *codenames[] = { "test1", "test2", "test3", "test4", nullptr };
    ASSERT_TRUE(string_array_eq(mb_device_codenames(_device), codenames));

    ASSERT_STREQ(mb_device_name(_device), "Test Device");
    ASSERT_STREQ(mb_device_architecture(_device), "arm64-v8a");

    const char *base_dirs[] = { "/dev/block/bootdevice/by-name", nullptr };
    ASSERT_TRUE(string_array_eq(mb_device_block_dev_base_dirs(_device), base_dirs));

    const char *system_devs[] = {
        "/dev/block/bootdevice/by-name/system",
        "/dev/block/sda1",
        nullptr
    };
    ASSERT_TRUE(string_array_eq(mb_device_system_block_devs(_device), system_devs));

    const char *cache_devs[] = {
        "/dev/block/bootdevice/by-name/cache",
        "/dev/block/sda2",
        nullptr
    };
    ASSERT_TRUE(string_array_eq(mb_device_cache_block_devs(_device), cache_devs));

    const char *data_devs[] = {
        "/dev/block/bootdevice/by-name/userdata",
        "/dev/block/sda3",
        nullptr
    };
    ASSERT_TRUE(string_array_eq(mb_device_data_block_devs(_device), data_devs));

    const char *boot_devs[] = {
        "/dev/block/bootdevice/by-name/boot",
        "/dev/block/sda4",
        nullptr
    };
    ASSERT_TRUE(string_array_eq(mb_device_boot_block_devs(_device), boot_devs));

    const char *recovery_devs[] = {
        "/dev/block/bootdevice/by-name/recovery",
        "/dev/block/sda5",
        nullptr
    };
    ASSERT_TRUE(string_array_eq(mb_device_recovery_block_devs(_device), recovery_devs));

    const char *extra_devs[] = {
        "/dev/block/bootdevice/by-name/modem",
        "/dev/block/sda6",
        nullptr
    };
    ASSERT_TRUE(string_array_eq(mb_device_extra_block_devs(_device), extra_devs));

    /* Boot UI */

    ASSERT_EQ(mb_device_tw_supported(_device), true);

    uint64_t flags =
            FLAG_TW_TOUCHSCREEN_SWAP_XY
            | FLAG_TW_TOUCHSCREEN_FLIP_X
            | FLAG_TW_TOUCHSCREEN_FLIP_Y
            | FLAG_TW_GRAPHICS_FORCE_USE_LINELENGTH
            | FLAG_TW_SCREEN_BLANK_ON_BOOT
            | FLAG_TW_BOARD_HAS_FLIPPED_SCREEN
            | FLAG_TW_IGNORE_MAJOR_AXIS_0
            | FLAG_TW_IGNORE_MT_POSITION_0
            | FLAG_TW_IGNORE_ABS_MT_TRACKING_ID
            | FLAG_TW_NEW_ION_HEAP
            | FLAG_TW_NO_SCREEN_BLANK
            | FLAG_TW_NO_SCREEN_TIMEOUT
            | FLAG_TW_ROUND_SCREEN
            | FLAG_TW_NO_CPU_TEMP
            | FLAG_TW_QCOM_RTC_FIX
            | FLAG_TW_HAS_DOWNLOAD_MODE
            | FLAG_TW_PREFER_LCD_BACKLIGHT;
    ASSERT_EQ(mb_device_tw_flags(_device), flags);

    ASSERT_EQ(mb_device_tw_pixel_format(_device), TW_PIXEL_FORMAT_RGBA_8888);
    ASSERT_EQ(mb_device_tw_force_pixel_format(_device), TW_FORCE_PIXEL_FORMAT_RGB_565);
    ASSERT_EQ(mb_device_tw_overscan_percent(_device), 10);
    ASSERT_EQ(mb_device_tw_default_x_offset(_device), 20);
    ASSERT_EQ(mb_device_tw_default_y_offset(_device), 30);
    ASSERT_STREQ(mb_device_tw_brightness_path(_device), "/sys/class/backlight");
    ASSERT_STREQ(mb_device_tw_secondary_brightness_path(_device), "/sys/class/lcd-backlight");
    ASSERT_EQ(mb_device_tw_max_brightness(_device), 255);
    ASSERT_EQ(mb_device_tw_default_brightness(_device), 100);
    ASSERT_STREQ(mb_device_tw_battery_path(_device), "/sys/class/battery");
    ASSERT_STREQ(mb_device_tw_cpu_temp_path(_device), "/sys/class/cputemp");
    ASSERT_STREQ(mb_device_tw_input_blacklist(_device), "foo");
    ASSERT_STREQ(mb_device_tw_input_whitelist(_device), "bar");

    const char *graphics_backends[] = {
        "overlay_msm_old",
        "fbdev",
        nullptr
    };
    ASSERT_TRUE(string_array_eq(mb_device_tw_graphics_backends(_device), graphics_backends));

    ASSERT_STREQ(mb_device_tw_theme(_device), "portrait_hdpi");

    /* Crypto */

    ASSERT_EQ(mb_device_crypto_supported(_device), true);
    ASSERT_STREQ(mb_device_crypto_header_path(_device), "footer");
}

TEST_F(DeviceTest, TestJsonInvalidKey)
{
    MbDeviceJsonError error;

    ASSERT_EQ(mb_device_load_json(_device, sample_invalid_key, &error),
              MB_DEVICE_ERROR_JSON);
    ASSERT_EQ(error.type, MB_DEVICE_JSON_UNKNOWN_KEY);
    ASSERT_STREQ(error.context, ".foo");
}

TEST_F(DeviceTest, TestJsonInvalidValue)
{
    MbDeviceJsonError error;

    ASSERT_EQ(mb_device_load_json(_device, sample_invalid_tw_flags, &error),
              MB_DEVICE_ERROR_JSON);
    ASSERT_EQ(error.type, MB_DEVICE_JSON_UNKNOWN_VALUE);
    ASSERT_STREQ(error.context, ".boot_ui.flags[0]");

    ASSERT_EQ(mb_device_load_json(_device, sample_invalid_tw_pixel_format, &error),
              MB_DEVICE_ERROR_JSON);
    ASSERT_EQ(error.type, MB_DEVICE_JSON_UNKNOWN_VALUE);
    ASSERT_STREQ(error.context, ".boot_ui.pixel_format");

    ASSERT_EQ(mb_device_load_json(_device, sample_invalid_tw_force_pixel_format, &error),
              MB_DEVICE_ERROR_JSON);
    ASSERT_EQ(error.type, MB_DEVICE_JSON_UNKNOWN_VALUE);
    ASSERT_STREQ(error.context, ".boot_ui.force_pixel_format");
}

TEST_F(DeviceTest, TestJsonInvalidType)
{
    MbDeviceJsonError error;

    ASSERT_EQ(mb_device_load_json(_device, sample_invalid_root, &error),
              MB_DEVICE_ERROR_JSON);
    ASSERT_EQ(error.type, MB_DEVICE_JSON_MISMATCHED_TYPE);
    ASSERT_STREQ(error.context, ".");
    ASSERT_STREQ(error.actual_type, "array");
    ASSERT_STREQ(error.expected_type, "object");

    ASSERT_EQ(mb_device_load_json(_device, sample_invalid_type, &error),
              MB_DEVICE_ERROR_JSON);
    ASSERT_EQ(error.type, MB_DEVICE_JSON_MISMATCHED_TYPE);
    ASSERT_STREQ(error.context, ".boot_ui");
    ASSERT_STREQ(error.actual_type, "string");
    ASSERT_STREQ(error.expected_type, "object");
}

int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
