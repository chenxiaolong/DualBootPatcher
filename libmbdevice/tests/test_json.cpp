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
#include "mbdevice/json.h"
#include "mbdevice/capi/json.h"

#define TO_U(TYPE, VALUE) \
    static_cast<std::underlying_type_t<TYPE>>(TYPE::VALUE)

using namespace mb::device;

static constexpr char sample_complete[] = R"json(
    {
        "name": "Test Device",
        "id": "test",
        "codenames": [
            "test1",
            "test2",
            "test3",
            "test4"
        ],
        "architecture": "arm64-v8a",
        "flags": [
            "HAS_COMBINED_BOOT_AND_RECOVERY"
        ],
        "block_devs": {
            "base_dirs": [
                "/dev/block/bootdevice/by-name"
            ],
            "system": [
                "/dev/block/bootdevice/by-name/system",
                "/dev/block/sda1"
            ],
            "cache": [
                "/dev/block/bootdevice/by-name/cache",
                "/dev/block/sda2"
            ],
            "data": [
                "/dev/block/bootdevice/by-name/userdata",
                "/dev/block/sda3"
            ],
            "boot": [
                "/dev/block/bootdevice/by-name/boot",
                "/dev/block/sda4"
            ],
            "recovery": [
                "/dev/block/bootdevice/by-name/recovery",
                "/dev/block/sda5"
            ],
            "extra": [
                "/dev/block/bootdevice/by-name/modem",
                "/dev/block/sda6"
            ]
        },
        "boot_ui": {
            "supported": true,
            "flags": [
                "TW_TOUCHSCREEN_SWAP_XY",
                "TW_TOUCHSCREEN_FLIP_X",
                "TW_TOUCHSCREEN_FLIP_Y",
                "TW_GRAPHICS_FORCE_USE_LINELENGTH",
                "TW_SCREEN_BLANK_ON_BOOT",
                "TW_BOARD_HAS_FLIPPED_SCREEN",
                "TW_IGNORE_MAJOR_AXIS_0",
                "TW_IGNORE_MT_POSITION_0",
                "TW_IGNORE_ABS_MT_TRACKING_ID",
                "TW_NEW_ION_HEAP",
                "TW_NO_SCREEN_BLANK",
                "TW_NO_SCREEN_TIMEOUT",
                "TW_ROUND_SCREEN",
                "TW_NO_CPU_TEMP",
                "TW_QCOM_RTC_FIX",
                "TW_HAS_DOWNLOAD_MODE",
                "TW_PREFER_LCD_BACKLIGHT"
            ],
            "pixel_format": "RGBA_8888",
            "force_pixel_format": "RGB_565",
            "overscan_percent": 10,
            "default_x_offset": 20,
            "default_y_offset": 30,
            "brightness_path": "/sys/class/backlight",
            "secondary_brightness_path": "/sys/class/lcd-backlight",
            "max_brightness": 255,
            "default_brightness": 100,
            "battery_path": "/sys/class/battery",
            "cpu_temp_path": "/sys/class/cputemp",
            "input_blacklist": "foo",
            "input_whitelist": "bar",
            "graphics_backends": [
                "overlay_msm_old",
                "fbdev"
            ],
            "theme": "portrait_hdpi"
        }
    }
)json";

static constexpr char sample_invalid_root[] = R"json(
    [
        "foo",
        "bar"
    ]
)json";

static constexpr char sample_invalid_key[] = R"json(
    {
        "foo": "bar"
    }
)json";

static constexpr char sample_invalid_device_flags[] = R"json(
    {
        "flags": [
            "FOO_BAR"
        ]
    }
)json";

static constexpr char sample_invalid_tw_flags[] = R"json(
    {
        "boot_ui": {
            "flags": [
                "TW_FOO_BAR"
            ]
        }
    }
)json";

static constexpr char sample_invalid_tw_pixel_format[] = R"json(
    {
        "boot_ui": {
            "pixel_format": "FOO_BAR"
        }
    }
)json";

static constexpr char sample_invalid_tw_force_pixel_format[] = R"json(
    {
        "boot_ui": {
            "force_pixel_format": "FOO_BAR"
        }
    }
)json";

static constexpr char sample_invalid_type[] = R"json(
    {
        "boot_ui": "FOO_BAR"
    }
)json";

static constexpr char sample_malformed[] = R"json(
    {
)json";

static constexpr char sample_multiple[] = R"json(
    [
        {
            "name": "test1",
            "id": "test1",
            "codenames": ["test1"],
            "architecture": "armeabi-v7a",
            "block_devs": {
                "system": ["/dev/blah"],
                "cache": ["/dev/blah"],
                "data": ["/dev/blah"],
                "boot": ["/dev/blah"]
            }
        },
        {
            "name": "test2",
            "id": "test2",
            "codenames": ["test2"],
            "architecture": "arm64-v8a",
            "block_devs": {
                "system": ["/dev/blah"],
                "cache": ["/dev/blah"],
                "data": ["/dev/blah"],
                "boot": ["/dev/blah"]
            }
        }
    ]
)json";


TEST(JsonTest, LoadCompleteDefinition)
{
    Device device;
    JsonError error;
    ASSERT_TRUE(device_from_json(sample_complete, device, error));

    ASSERT_EQ(device.id(), "test");

    std::vector<std::string> codenames{"test1", "test2", "test3", "test4"};
    ASSERT_EQ(device.codenames(), codenames);

    ASSERT_EQ(device.name(), "Test Device");
    ASSERT_EQ(device.architecture(), "arm64-v8a");

    DeviceFlags device_flags = DeviceFlag::HasCombinedBootAndRecovery;
    ASSERT_EQ(device.flags(), device_flags);

    std::vector<std::string> base_dirs{"/dev/block/bootdevice/by-name"};
    ASSERT_EQ(device.block_dev_base_dirs(), base_dirs);

    std::vector<std::string> system_devs{
        "/dev/block/bootdevice/by-name/system",
        "/dev/block/sda1",
    };
    ASSERT_EQ(device.system_block_devs(), system_devs);

    std::vector<std::string> cache_devs{
        "/dev/block/bootdevice/by-name/cache",
        "/dev/block/sda2",
    };
    ASSERT_EQ(device.cache_block_devs(), cache_devs);

    std::vector<std::string> data_devs{
        "/dev/block/bootdevice/by-name/userdata",
        "/dev/block/sda3",
    };
    ASSERT_EQ(device.data_block_devs(), data_devs);

    std::vector<std::string> boot_devs{
        "/dev/block/bootdevice/by-name/boot",
        "/dev/block/sda4",
    };
    ASSERT_EQ(device.boot_block_devs(), boot_devs);

    std::vector<std::string> recovery_devs{
        "/dev/block/bootdevice/by-name/recovery",
        "/dev/block/sda5",
    };
    ASSERT_EQ(device.recovery_block_devs(), recovery_devs);

    std::vector<std::string> extra_devs{
        "/dev/block/bootdevice/by-name/modem",
        "/dev/block/sda6",
    };
    ASSERT_EQ(device.extra_block_devs(), extra_devs);

    /* Boot UI */

    ASSERT_TRUE(device.tw_supported());

    TwFlags flags =
            TwFlag::TouchscreenSwapXY
            | TwFlag::TouchscreenFlipX
            | TwFlag::TouchscreenFlipY
            | TwFlag::GraphicsForceUseLineLength
            | TwFlag::ScreenBlankOnBoot
            | TwFlag::BoardHasFlippedScreen
            | TwFlag::IgnoreMajorAxis0
            | TwFlag::IgnoreMtPosition0
            | TwFlag::IgnoreAbsMtTrackingId
            | TwFlag::NewIonHeap
            | TwFlag::NoScreenBlank
            | TwFlag::NoScreenTimeout
            | TwFlag::RoundScreen
            | TwFlag::NoCpuTemp
            | TwFlag::QcomRtcFix
            | TwFlag::HasDownloadMode
            | TwFlag::PreferLcdBacklight;
    ASSERT_EQ(device.tw_flags(), flags);

    ASSERT_EQ(device.tw_pixel_format(), TwPixelFormat::Rgba8888);
    ASSERT_EQ(device.tw_force_pixel_format(), TwForcePixelFormat::Rgb565);
    ASSERT_EQ(device.tw_overscan_percent(), 10);
    ASSERT_EQ(device.tw_default_x_offset(), 20);
    ASSERT_EQ(device.tw_default_y_offset(), 30);
    ASSERT_EQ(device.tw_brightness_path(), "/sys/class/backlight");
    ASSERT_EQ(device.tw_secondary_brightness_path(),
              "/sys/class/lcd-backlight");
    ASSERT_EQ(device.tw_max_brightness(), 255);
    ASSERT_EQ(device.tw_default_brightness(), 100);
    ASSERT_EQ(device.tw_battery_path(), "/sys/class/battery");
    ASSERT_EQ(device.tw_cpu_temp_path(), "/sys/class/cputemp");
    ASSERT_EQ(device.tw_input_blacklist(), "foo");
    ASSERT_EQ(device.tw_input_whitelist(), "bar");

    std::vector<std::string> graphics_backends{"overlay_msm_old", "fbdev"};
    ASSERT_EQ(device.tw_graphics_backends(), graphics_backends);

    ASSERT_EQ(device.tw_theme(), "portrait_hdpi");
}

TEST(JsonTest, LoadInvalidKey)
{
    Device device;
    JsonError error;
    ASSERT_FALSE(device_from_json(sample_invalid_key, device, error));
    ASSERT_EQ(error.type, JsonErrorType::SchemaValidationFailure);
    ASSERT_EQ(error.schema_uri, "#");
    ASSERT_EQ(error.schema_keyword, "additionalProperties");
    ASSERT_EQ(error.document_uri, "#/foo");
}

TEST(JsonTest, LoadInvalidValue)
{
    Device d1;
    JsonError e1;
    ASSERT_FALSE(device_from_json(sample_invalid_device_flags, d1, e1));
    ASSERT_EQ(e1.type, JsonErrorType::SchemaValidationFailure);
    ASSERT_EQ(e1.schema_uri, "#/properties/flags/items");
    ASSERT_EQ(e1.schema_keyword, "enum");
    ASSERT_EQ(e1.document_uri, "#/flags/0");

    Device d2;
    JsonError e2;
    ASSERT_FALSE(device_from_json(sample_invalid_tw_flags, d2, e2));
    ASSERT_EQ(e2.type, JsonErrorType::SchemaValidationFailure);
    ASSERT_EQ(e2.schema_uri, "#/properties/boot_ui/properties/flags/items");
    ASSERT_EQ(e2.schema_keyword, "enum");
    ASSERT_EQ(e2.document_uri, "#/boot_ui/flags/0");

    Device d3;
    JsonError e3;
    ASSERT_FALSE(device_from_json(sample_invalid_tw_pixel_format, d3, e3));
    ASSERT_EQ(e3.type, JsonErrorType::SchemaValidationFailure);
    ASSERT_EQ(e3.schema_uri, "#/properties/boot_ui/properties/pixel_format");
    ASSERT_EQ(e3.schema_keyword, "enum");
    ASSERT_EQ(e3.document_uri, "#/boot_ui/pixel_format");

    Device d4;
    JsonError e4;
    ASSERT_FALSE(device_from_json(sample_invalid_tw_force_pixel_format, d4, e4));
    ASSERT_EQ(e4.type, JsonErrorType::SchemaValidationFailure);
    ASSERT_EQ(e4.schema_uri, "#/properties/boot_ui/properties/force_pixel_format");
    ASSERT_EQ(e4.schema_keyword, "enum");
    ASSERT_EQ(e4.document_uri, "#/boot_ui/force_pixel_format");
}

TEST(JsonTest, LoadInvalidType)
{
    Device d1;
    JsonError e1;
    ASSERT_FALSE(device_from_json(sample_invalid_root, d1, e1));
    ASSERT_EQ(e1.type, JsonErrorType::SchemaValidationFailure);
    ASSERT_EQ(e1.schema_uri, "#");
    ASSERT_EQ(e1.schema_keyword, "type");
    ASSERT_EQ(e1.document_uri, "#");

    Device d2;
    JsonError e2;
    ASSERT_FALSE(device_from_json(sample_invalid_type, d2, e2));
    ASSERT_EQ(e2.type, JsonErrorType::SchemaValidationFailure);
    ASSERT_EQ(e2.schema_uri, "#/properties/boot_ui");
    ASSERT_EQ(e2.schema_keyword, "type");
    ASSERT_EQ(e2.document_uri, "#/boot_ui");
}

TEST(JsonTest, LoadMalformed)
{
    Device d1;
    JsonError e1;
    ASSERT_FALSE(device_from_json(sample_malformed, d1, e1));
    ASSERT_EQ(e1.type, JsonErrorType::ParseError);
}

TEST(JsonTest, LoadMultiple)
{
    std::vector<Device> d1;
    JsonError e1;
    ASSERT_TRUE(device_list_from_json(sample_multiple, d1, e1));

    std::vector<Device> d2;
    JsonError e2;
    ASSERT_FALSE(device_list_from_json(sample_complete, d2, e2));
    ASSERT_EQ(e2.type, JsonErrorType::SchemaValidationFailure);
    ASSERT_EQ(e2.schema_uri, "#");
    ASSERT_EQ(e2.schema_keyword, "type");
    ASSERT_EQ(e2.document_uri, "#");
}

TEST(JsonTest, CreateJson)
{
    Device d1;
    JsonError e1;
    ASSERT_TRUE(device_from_json(sample_complete, d1, e1));

    std::string json;
    ASSERT_TRUE(device_to_json(d1, json));

    Device d2;
    JsonError e2;
    ASSERT_TRUE(device_from_json(json, d2, e2));

    ASSERT_EQ(d1, d2);
}

TEST(JsonTest, CheckCapiFlagsEqual)
{
    ASSERT_EQ(TO_U(JsonErrorType, ParseError),
              MB_DEVICE_JSON_PARSE_ERROR);
    ASSERT_EQ(TO_U(JsonErrorType, SchemaValidationFailure),
              MB_DEVICE_JSON_SCHEMA_VALIDATION_FAILURE);
}
