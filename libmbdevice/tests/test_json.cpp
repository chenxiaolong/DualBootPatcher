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
        "\"flags\": ["
            "\"HAS_COMBINED_BOOT_AND_RECOVERY\""
        "],"
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

static const char sample_invalid_device_flags[] =
    "{"
        "\"flags\": ["
            "\"FOO_BAR\""
        "]"
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

static const char sample_malformed[] =
    "{";

static const char sample_multiple[] =
    "["
        "{"
            "\"id\": \"test1\""
        "},"
        "{"
            "\"id\": \"test2\""
        "}"
    "]";


struct ScopedDevice
{
    Device *device;
    MbDeviceJsonError error;

    ScopedDevice(const char *json)
    {
        device = mb_device_new_from_json(json, &error);
    }

    ~ScopedDevice()
    {
        mb_device_free(device);
    }
};

struct ScopedDevices
{
    Device **devices;
    MbDeviceJsonError error;

    ScopedDevices(const char *json)
    {
        devices = mb_device_new_list_from_json(json, &error);
    }

    ~ScopedDevices()
    {
        if (devices) {
            for (struct Device **iter = devices; *iter; ++iter) {
                mb_device_free(*iter);
            }
            free(devices);
        }
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

TEST(JsonTest, LoadCompleteDefinition)
{
    ScopedDevice sd(sample_complete);

    ASSERT_NE(sd.device, nullptr);

    ASSERT_STREQ(mb_device_id(sd.device), "test");

    const char *codenames[] = { "test1", "test2", "test3", "test4", nullptr };
    ASSERT_TRUE(string_array_eq(mb_device_codenames(sd.device), codenames));

    ASSERT_STREQ(mb_device_name(sd.device), "Test Device");
    ASSERT_STREQ(mb_device_architecture(sd.device), "arm64-v8a");

    uint64_t device_flags = FLAG_HAS_COMBINED_BOOT_AND_RECOVERY;
    ASSERT_EQ(mb_device_flags(sd.device), device_flags);

    const char *base_dirs[] = { "/dev/block/bootdevice/by-name", nullptr };
    ASSERT_TRUE(string_array_eq(mb_device_block_dev_base_dirs(sd.device), base_dirs));

    const char *system_devs[] = {
        "/dev/block/bootdevice/by-name/system",
        "/dev/block/sda1",
        nullptr
    };
    ASSERT_TRUE(string_array_eq(mb_device_system_block_devs(sd.device), system_devs));

    const char *cache_devs[] = {
        "/dev/block/bootdevice/by-name/cache",
        "/dev/block/sda2",
        nullptr
    };
    ASSERT_TRUE(string_array_eq(mb_device_cache_block_devs(sd.device), cache_devs));

    const char *data_devs[] = {
        "/dev/block/bootdevice/by-name/userdata",
        "/dev/block/sda3",
        nullptr
    };
    ASSERT_TRUE(string_array_eq(mb_device_data_block_devs(sd.device), data_devs));

    const char *boot_devs[] = {
        "/dev/block/bootdevice/by-name/boot",
        "/dev/block/sda4",
        nullptr
    };
    ASSERT_TRUE(string_array_eq(mb_device_boot_block_devs(sd.device), boot_devs));

    const char *recovery_devs[] = {
        "/dev/block/bootdevice/by-name/recovery",
        "/dev/block/sda5",
        nullptr
    };
    ASSERT_TRUE(string_array_eq(mb_device_recovery_block_devs(sd.device), recovery_devs));

    const char *extra_devs[] = {
        "/dev/block/bootdevice/by-name/modem",
        "/dev/block/sda6",
        nullptr
    };
    ASSERT_TRUE(string_array_eq(mb_device_extra_block_devs(sd.device), extra_devs));

    /* Boot UI */

    ASSERT_EQ(mb_device_tw_supported(sd.device), true);

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
    ASSERT_EQ(mb_device_tw_flags(sd.device), flags);

    ASSERT_EQ(mb_device_tw_pixel_format(sd.device), TW_PIXEL_FORMAT_RGBA_8888);
    ASSERT_EQ(mb_device_tw_force_pixel_format(sd.device), TW_FORCE_PIXEL_FORMAT_RGB_565);
    ASSERT_EQ(mb_device_tw_overscan_percent(sd.device), 10);
    ASSERT_EQ(mb_device_tw_default_x_offset(sd.device), 20);
    ASSERT_EQ(mb_device_tw_default_y_offset(sd.device), 30);
    ASSERT_STREQ(mb_device_tw_brightness_path(sd.device), "/sys/class/backlight");
    ASSERT_STREQ(mb_device_tw_secondary_brightness_path(sd.device), "/sys/class/lcd-backlight");
    ASSERT_EQ(mb_device_tw_max_brightness(sd.device), 255);
    ASSERT_EQ(mb_device_tw_default_brightness(sd.device), 100);
    ASSERT_STREQ(mb_device_tw_battery_path(sd.device), "/sys/class/battery");
    ASSERT_STREQ(mb_device_tw_cpu_temp_path(sd.device), "/sys/class/cputemp");
    ASSERT_STREQ(mb_device_tw_input_blacklist(sd.device), "foo");
    ASSERT_STREQ(mb_device_tw_input_whitelist(sd.device), "bar");

    const char *graphics_backends[] = {
        "overlay_msm_old",
        "fbdev",
        nullptr
    };
    ASSERT_TRUE(string_array_eq(mb_device_tw_graphics_backends(sd.device), graphics_backends));

    ASSERT_STREQ(mb_device_tw_theme(sd.device), "portrait_hdpi");
}

TEST(JsonTest, LoadInvalidKey)
{
    ScopedDevice sd(sample_invalid_key);

    ASSERT_EQ(sd.device, nullptr);
    ASSERT_EQ(sd.error.type, MB_DEVICE_JSON_UNKNOWN_KEY);
    ASSERT_STREQ(sd.error.context, ".foo");
}

TEST(JsonTest, LoadInvalidValue)
{
    ScopedDevice sd1(sample_invalid_device_flags);
    ASSERT_EQ(sd1.device, nullptr);
    ASSERT_EQ(sd1.error.type, MB_DEVICE_JSON_UNKNOWN_VALUE);
    ASSERT_STREQ(sd1.error.context, ".flags[0]");

    ScopedDevice sd2(sample_invalid_tw_flags);
    ASSERT_EQ(sd2.device, nullptr);
    ASSERT_EQ(sd2.error.type, MB_DEVICE_JSON_UNKNOWN_VALUE);
    ASSERT_STREQ(sd2.error.context, ".boot_ui.flags[0]");

    ScopedDevice sd3(sample_invalid_tw_pixel_format);
    ASSERT_EQ(sd3.device, nullptr);
    ASSERT_EQ(sd3.error.type, MB_DEVICE_JSON_UNKNOWN_VALUE);
    ASSERT_STREQ(sd3.error.context, ".boot_ui.pixel_format");

    ScopedDevice sd4(sample_invalid_tw_force_pixel_format);
    ASSERT_EQ(sd4.device, nullptr);
    ASSERT_EQ(sd4.error.type, MB_DEVICE_JSON_UNKNOWN_VALUE);
    ASSERT_STREQ(sd4.error.context, ".boot_ui.force_pixel_format");
}

TEST(JsonTest, LoadInvalidType)
{
    ScopedDevice sd1(sample_invalid_root);
    ASSERT_EQ(sd1.device, nullptr);
    ASSERT_EQ(sd1.error.type, MB_DEVICE_JSON_MISMATCHED_TYPE);
    ASSERT_STREQ(sd1.error.context, ".");
    ASSERT_STREQ(sd1.error.actual_type, "array");
    ASSERT_STREQ(sd1.error.expected_type, "object");

    ScopedDevice sd2(sample_invalid_type);
    ASSERT_EQ(sd2.device, nullptr);
    ASSERT_EQ(sd2.error.type, MB_DEVICE_JSON_MISMATCHED_TYPE);
    ASSERT_STREQ(sd2.error.context, ".boot_ui");
    ASSERT_STREQ(sd2.error.actual_type, "string");
    ASSERT_STREQ(sd2.error.expected_type, "object");
}

TEST(JsonTest, LoadMalformed)
{
    ScopedDevice sd1(sample_malformed);
    ASSERT_EQ(sd1.device, nullptr);
    ASSERT_EQ(sd1.error.type, MB_DEVICE_JSON_PARSE_ERROR);
    ASSERT_EQ(sd1.error.line, 1);
    ASSERT_EQ(sd1.error.column, 1);
}

TEST(JsonTest, LoadMultiple)
{
    ScopedDevices sd1(sample_multiple);
    ASSERT_NE(sd1.devices, nullptr);

    ScopedDevices sd2(sample_complete);
    ASSERT_EQ(sd2.devices, nullptr);
    ASSERT_EQ(sd2.error.type, MB_DEVICE_JSON_MISMATCHED_TYPE);
    ASSERT_STREQ(sd2.error.context, ".");
    ASSERT_STREQ(sd2.error.actual_type, "object");
    ASSERT_STREQ(sd2.error.expected_type, "array");
}

TEST(JsonTest, CreateJson)
{
    ScopedDevice sd1(sample_complete);
    ASSERT_NE(sd1.device, nullptr);

    std::unique_ptr<char, void (*)(void *)> json(
            mb_device_to_json(sd1.device), free);
    ASSERT_TRUE(json.operator bool());

    ScopedDevice sd2(json.get());
    ASSERT_TRUE(mb_device_equals(sd1.device, sd2.device));
}
