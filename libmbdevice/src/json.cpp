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

#include "mbdevice/json.h"

#include <cstring>

#include <jansson.h>

#include "mbcommon/string.h"


#define JSON_BOOLEAN JSON_TRUE

namespace mb
{
namespace device
{

using ScopedJsonT = std::unique_ptr<json_t, decltype(json_decref) *>;

using DeviceFlagMapping = std::pair<const char *, DeviceFlag>;
using TwFlagMapping = std::pair<const char *, TwFlag>;
using TwPixelFormatMapping = std::pair<const char *, TwPixelFormat>;
using TwForcePixelFormatMapping = std::pair<const char *, TwForcePixelFormat>;

static constexpr std::array<DeviceFlagMapping, 2> g_device_flag_mappings{{
    { "HAS_COMBINED_BOOT_AND_RECOVERY", DeviceFlag::HasCombinedBootAndRecovery },
    { "FSTAB_SKIP_SDCARD0",             DeviceFlag::FstabSkipSdcard0 },
}};

static constexpr std::array<TwFlagMapping, 17> g_tw_flag_mappings{{
    { "TW_TOUCHSCREEN_SWAP_XY",           TwFlag::TouchscreenSwapXY },
    { "TW_TOUCHSCREEN_FLIP_X",            TwFlag::TouchscreenFlipX },
    { "TW_TOUCHSCREEN_FLIP_Y",            TwFlag::TouchscreenFlipY },
    { "TW_GRAPHICS_FORCE_USE_LINELENGTH", TwFlag::GraphicsForceUseLineLength },
    { "TW_SCREEN_BLANK_ON_BOOT",          TwFlag::ScreenBlankOnBoot },
    { "TW_BOARD_HAS_FLIPPED_SCREEN",      TwFlag::BoardHasFlippedScreen },
    { "TW_IGNORE_MAJOR_AXIS_0",           TwFlag::IgnoreMajorAxis0 },
    { "TW_IGNORE_MT_POSITION_0",          TwFlag::IgnoreMtPosition0 },
    { "TW_IGNORE_ABS_MT_TRACKING_ID",     TwFlag::IgnoreAbsMtTrackingId },
    { "TW_NEW_ION_HEAP",                  TwFlag::NewIonHeap },
    { "TW_NO_SCREEN_BLANK",               TwFlag::NoScreenBlank },
    { "TW_NO_SCREEN_TIMEOUT",             TwFlag::NoScreenTimeout },
    { "TW_ROUND_SCREEN",                  TwFlag::RoundScreen },
    { "TW_NO_CPU_TEMP",                   TwFlag::NoCpuTemp },
    { "TW_QCOM_RTC_FIX",                  TwFlag::QcomRtcFix },
    { "TW_HAS_DOWNLOAD_MODE",             TwFlag::HasDownloadMode },
    { "TW_PREFER_LCD_BACKLIGHT",          TwFlag::PreferLcdBacklight },
}};

static constexpr std::array<TwPixelFormatMapping, 5> g_tw_pxfmt_mappings{{
    { "DEFAULT",   TwPixelFormat::Default },
    { "ABGR_8888", TwPixelFormat::Abgr8888 },
    { "RGBX_8888", TwPixelFormat::Rgbx8888 },
    { "BGRA_8888", TwPixelFormat::Bgra8888 },
    { "RGBA_8888", TwPixelFormat::Rgba8888 },
}};

static constexpr std::array<TwForcePixelFormatMapping, 2> g_tw_force_pxfmt_mappings{{
    { "NONE",    TwForcePixelFormat::None },
    { "RGB_565", TwForcePixelFormat::Rgb565 },
}};

static std::string json_type_to_string(json_type type)
{
    switch (type) {
    case JSON_OBJECT:
        return "object";
    case JSON_ARRAY:
        return "array";
    case JSON_STRING:
        return "string";
    case JSON_INTEGER:
        return "integer";
    case JSON_REAL:
        return "real";
    case JSON_TRUE:
    case JSON_FALSE:
        return "boolean";
    case JSON_NULL:
        return "null";
    default:
        return {};
    }
}

static void json_error_set_parse_error(JsonError &error, int line, int column)
{
    error.type = JsonErrorType::ParseError;
    error.line = line;
    error.column = column;
}

static void json_error_set_mismatched_type(JsonError &error,
                                           std::string context,
                                           json_type actual_type,
                                           json_type expected_type)
{
    error.type = JsonErrorType::MismatchedType;

    if (context.empty()) {
        error.context = ".";
    } else {
        error.context = std::move(context);
    }

    error.actual_type = json_type_to_string(actual_type);
    error.expected_type = json_type_to_string(expected_type);
}

static void json_error_set_unknown_key(JsonError &error, std::string context)
{
    error.type = JsonErrorType::UnknownKey;

    if (context.empty()) {
        error.context = ".";
    } else {
        error.context = std::move(context);
    }
}

static void json_error_set_unknown_value(JsonError &error, std::string context)
{
    error.type = JsonErrorType::UnknownValue;

    if (context.empty()) {
        error.context = ".";
    } else {
        error.context = std::move(context);
    }
}

static inline bool device_set_boolean(void (Device::*setter)(bool),
                                      Device &device, json_t *value,
                                      const std::string &context,
                                      JsonError &error)
{
    if (!json_is_boolean(value)) {
        json_error_set_mismatched_type(
                error, context, value->type, JSON_BOOLEAN);
        return false;
    }

    (device.*setter)(json_boolean_value(value));
    return true;
}

static inline bool device_set_int(void (Device::*setter)(int),
                                  Device &device, json_t *value,
                                  const std::string &context,
                                  JsonError &error)
{
    if (!json_is_integer(value)) {
        json_error_set_mismatched_type(
                error, context, value->type, JSON_INTEGER);
        return false;
    }

    (device.*setter)(json_integer_value(value));
    return true;
}

static inline bool device_set_string(void (Device::*setter)(std::string),
                                     Device &device, json_t *value,
                                     const std::string &context,
                                     JsonError &error)
{
    if (!json_is_string(value)) {
        json_error_set_mismatched_type(
                error, context, value->type, JSON_STRING);
        return false;
    }

    (device.*setter)(json_string_value(value));
    return true;
}

static inline bool device_set_string_array(void (Device::*setter)(std::vector<std::string>),
                                           Device &device, json_t *node,
                                           const std::string &context,
                                           JsonError &error)
{
    std::string subcontext;
    size_t index;
    json_t *value;
    std::vector<std::string> array;

    if (!json_is_array(node)) {
        json_error_set_mismatched_type(
                error, context, node->type, JSON_ARRAY);
        return false;
    }

    json_array_foreach(node, index, value) {
#ifdef __ANDROID__
        subcontext = format("%s[%" MB_PRIzu "]", context.c_str(), index);
#else
        subcontext = context;
        subcontext += "[";
        subcontext += std::to_string(index);
        subcontext += "]";
#endif

        if (!json_is_string(value)) {
            json_error_set_mismatched_type(
                    error, subcontext, value->type, JSON_STRING);
            return false;
        }

        array.push_back(json_string_value(value));
    }

    (device.*setter)(std::move(array));
    return true;
}

static bool process_device_flags(Device &device, json_t *node,
                                 const std::string &context, JsonError &error)
{
    std::string subcontext;
    size_t index;
    json_t *value;
    DeviceFlags flags = 0;

    if (!json_is_array(node)) {
        json_error_set_mismatched_type(error, context, node->type, JSON_ARRAY);
        return false;
    }

    json_array_foreach(node, index, value) {
#ifdef __ANDROID__
        subcontext = mb::format("%s[%" MB_PRIzu "]", context.c_str(), index);
#else
        subcontext = context;
        subcontext += "[";
        subcontext += std::to_string(index);
        subcontext += "]";
#endif

        if (!json_is_string(value)) {
            json_error_set_mismatched_type(
                    error, subcontext, value->type, JSON_STRING);
            return false;
        }

        const std::string &str = json_string_value(value);
        DeviceFlags old_flags = flags;

        for (auto const &item : g_device_flag_mappings) {
            if (str == item.first) {
                flags |= item.second;
                break;
            }
        }

        if (flags == old_flags) {
            json_error_set_unknown_value(error, subcontext);
            return false;
        }
    }

    device.set_flags(flags);
    return true;
}

static bool process_boot_ui_flags(Device &device, json_t *node,
                                  const std::string &context, JsonError &error)
{
    std::string subcontext;
    size_t index;
    json_t *value;
    TwFlags flags = 0;

    if (!json_is_array(node)) {
        json_error_set_mismatched_type(error, context, node->type, JSON_ARRAY);
        return false;
    }

    json_array_foreach(node, index, value) {
#ifdef __ANDROID__
        subcontext = mb::format("%s[%" MB_PRIzu "]", context.c_str(), index);
#else
        subcontext = context;
        subcontext += "[";
        subcontext += std::to_string(index);
        subcontext += "]";
#endif

        if (!json_is_string(value)) {
            json_error_set_mismatched_type(
                    error, subcontext, value->type, JSON_STRING);
            return false;
        }

        const std::string &str = json_string_value(value);
        uint64_t old_flags = flags;

        for (auto const &item : g_tw_flag_mappings) {
            if (str == item.first) {
                flags |= item.second;
                break;
            }
        }

        if (flags == old_flags) {
            json_error_set_unknown_value(error, subcontext);
            return false;
        }
    }

    device.set_tw_flags(flags);
    return true;
}

static bool process_boot_ui_pixel_format(Device &device, json_t *node,
                                         const std::string &context,
                                         JsonError &error)
{
    if (!json_is_string(node)) {
        json_error_set_mismatched_type(error, context, node->type, JSON_STRING);
        return false;
    }

    std::string str = json_string_value(node);

    for (auto const &item : g_tw_pxfmt_mappings) {
        if (str == item.first) {
            device.set_tw_pixel_format(item.second);
            return true;
        }
    }

    json_error_set_unknown_value(error, context);
    return false;
}

static bool process_boot_ui_force_pixel_format(Device &device, json_t *node,
                                               const std::string &context,
                                               JsonError &error)
{
    if (!json_is_string(node)) {
        json_error_set_mismatched_type(error, context, node->type, JSON_STRING);
        return false;
    }

    std::string str = json_string_value(node);

    for (auto const &item : g_tw_force_pxfmt_mappings) {
        if (str == item.first) {
            device.set_tw_force_pixel_format(item.second);
            return true;
        }
    }

    json_error_set_unknown_value(error, context);
    return false;
}

static bool process_boot_ui(Device &device, json_t *node,
                            const std::string &context,
                            JsonError &error)
{
    bool ret;
    std::string subcontext;
    const char *key;
    json_t *value;

    if (!json_is_object(node)) {
        json_error_set_mismatched_type(
                error, context, node->type, JSON_OBJECT);
        return false;
    }

    json_object_foreach(node, key, value) {
        subcontext = context;
        subcontext += ".";
        subcontext += key;

        if (strcmp(key, "supported") == 0) {
            ret = device_set_boolean(&Device::set_tw_supported,
                                     device, value, subcontext, error);
        } else if (strcmp(key, "flags") == 0) {
            ret = process_boot_ui_flags(device, value, subcontext, error);
        } else if (strcmp(key, "pixel_format") == 0) {
            ret = process_boot_ui_pixel_format(device, value, subcontext,
                                               error);
        } else if (strcmp(key, "force_pixel_format") == 0) {
            ret = process_boot_ui_force_pixel_format(device, value, subcontext,
                                                     error);
        } else if (strcmp(key, "overscan_percent") == 0) {
            ret = device_set_int(&Device::set_tw_overscan_percent,
                                 device, value, subcontext, error);
        } else if (strcmp(key, "default_x_offset") == 0) {
            ret = device_set_int(&Device::set_tw_default_x_offset,
                                 device, value, subcontext, error);
        } else if (strcmp(key, "default_y_offset") == 0) {
            ret = device_set_int(&Device::set_tw_default_y_offset,
                                 device, value, subcontext, error);
        } else if (strcmp(key, "brightness_path") == 0) {
            ret = device_set_string(&Device::set_tw_brightness_path,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "secondary_brightness_path") == 0) {
            ret = device_set_string(&Device::set_tw_secondary_brightness_path,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "max_brightness") == 0) {
            ret = device_set_int(&Device::set_tw_max_brightness,
                                 device, value, subcontext, error);
        } else if (strcmp(key, "default_brightness") == 0) {
            ret = device_set_int(&Device::set_tw_default_brightness,
                                 device, value, subcontext, error);
        } else if (strcmp(key, "battery_path") == 0) {
            ret = device_set_string(&Device::set_tw_battery_path,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "cpu_temp_path") == 0) {
            ret = device_set_string(&Device::set_tw_cpu_temp_path,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "input_blacklist") == 0) {
            ret = device_set_string(&Device::set_tw_input_blacklist,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "input_whitelist") == 0) {
            ret = device_set_string(&Device::set_tw_input_whitelist,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "graphics_backends") == 0) {
            ret = device_set_string_array(&Device::set_tw_graphics_backends,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "theme") == 0) {
            ret = device_set_string(&Device::set_tw_theme,
                                    device, value, subcontext, error);
        } else {
            json_error_set_unknown_key(error, subcontext);
            ret = false;
        }

        if (!ret) {
            break;
        }
    }

    return ret;
}

static bool process_block_devs(Device &device, json_t *node,
                               const std::string &context,
                               JsonError &error)
{
    bool ret;
    std::string subcontext;
    const char *key;
    json_t *value;

    if (!json_is_object(node)) {
        json_error_set_mismatched_type(
                error, context, node->type, JSON_OBJECT);
        return false;
    }

    json_object_foreach(node, key, value) {
        subcontext = context;
        subcontext += ".";
        subcontext += key;

        if (strcmp(key, "base_dirs") == 0) {
            ret = device_set_string_array(&Device::set_block_dev_base_dirs,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "system") == 0) {
            ret = device_set_string_array(&Device::set_system_block_devs,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "cache") == 0) {
            ret = device_set_string_array(&Device::set_cache_block_devs,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "data") == 0) {
            ret = device_set_string_array(&Device::set_data_block_devs,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "boot") == 0) {
            ret = device_set_string_array(&Device::set_boot_block_devs,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "recovery") == 0) {
            ret = device_set_string_array(&Device::set_recovery_block_devs,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "extra") == 0) {
            ret = device_set_string_array(&Device::set_extra_block_devs,
                                          device, value, subcontext, error);
        } else {
            json_error_set_unknown_key(error, subcontext);
            ret = false;
        }

        if (!ret) {
            break;
        }
    }

    return ret;
}

static bool process_device(Device &device, json_t *node,
                           const std::string &context,
                           JsonError &error)
{
    bool ret;
    std::string subcontext;
    const char *key;
    json_t *value;

    if (!json_is_object(node)) {
        json_error_set_mismatched_type(
                error, context, node->type, JSON_OBJECT);
        return false;
    }

    json_object_foreach(node, key, value) {
        subcontext = context;
        subcontext += ".";
        subcontext += key;

        if (strcmp(key, "name") == 0) {
            ret = device_set_string(&Device::set_name,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "id") == 0) {
            ret = device_set_string(&Device::set_id,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "codenames") == 0) {
            ret = device_set_string_array(&Device::set_codenames,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "architecture") == 0) {
            ret = device_set_string(&Device::set_architecture,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "flags") == 0) {
            ret = process_device_flags(device, value, subcontext, error);
        } else if (strcmp(key, "block_devs") == 0) {
            ret = process_block_devs(device, value, subcontext, error);
        } else if (strcmp(key, "boot_ui") == 0) {
            ret = process_boot_ui(device, value, subcontext, error);
        } else {
            json_error_set_unknown_key(error, subcontext);
            ret = false;
        }

        if (!ret) {
            break;
        }
    }

    return ret;
}

bool device_from_json(const std::string &json, Device &device, JsonError &error)
{
    json_error_t json_error;

    ScopedJsonT root(json_loads(json.c_str(), 0, &json_error), &json_decref);
    if (!root) {
        json_error_set_parse_error(error, json_error.line, json_error.column);
        return false;
    }

    device = Device();

    return process_device(device, root.get(), "", error);
}

bool device_list_from_json(const std::string &json,
                           std::vector<Device> &devices,
                           JsonError &error)
{
    std::vector<Device> result;
    json_t *elem;
    size_t index;
    json_error_t json_error;
    std::string context;

    ScopedJsonT root(json_loads(json.c_str(), 0, &json_error), &json_decref);
    if (!root) {
        json_error_set_parse_error(error, json_error.line, json_error.column);
        return false;
    }

    if (!json_is_array(root)) {
        json_error_set_mismatched_type(error, "", root->type, JSON_ARRAY);
        return false;
    }

    json_array_foreach(root.get(), index, elem) {
#ifdef __ANDROID__
        context = mb::format("[%" MB_PRIzu "]", index);
#else
        context = "[";
        context += std::to_string(index);
        context += "]";
#endif

        Device device;

        if (!process_device(device, elem, context, error)) {
            return false;
        }

        result.push_back(std::move(device));
    }

    devices.swap(result);
    return true;
}

static json_t * json_string_array(const std::vector<std::string> &array)
{
    ScopedJsonT j_array(json_array(), &json_decref);
    if (!j_array) {
        return nullptr;
    }

    for (auto const &item : array) {
        if (json_array_append_new(j_array.get(),
                                  json_string(item.c_str())) < 0) {
            return nullptr;
        }
    }

    return j_array.release();
}

bool device_to_json(const Device &device, std::string &json)
{
    ScopedJsonT root(json_object(), &json_decref);
    if (!root) {
        return false;
    }

    auto const &id = device.id();
    if (!id.empty() && json_object_set_new(
            root.get(), "id", json_string(id.c_str())) < 0) {
        return false;
    }

    auto const &codenames = device.codenames();
    if (!codenames.empty() && json_object_set_new(
            root.get(), "codenames", json_string_array(codenames)) < 0) {
        return false;
    }

    auto const &name = device.name();
    if (!name.empty() && json_object_set_new(
            root.get(), "name", json_string(name.c_str())) < 0) {
        return false;
    }

    auto const &architecture = device.architecture();
    if (!architecture.empty() && json_object_set_new(
            root.get(), "architecture", json_string(architecture.c_str())) < 0) {
        return false;
    }

    auto const flags = device.flags();
    if (flags) {
        json_t *array = json_array();

        if (json_object_set_new(root.get(), "flags", array) < 0) {
            return false;
        }

        for (auto const &item : g_device_flag_mappings) {
            if ((flags & item.second) && json_array_append_new(
                    array, json_string(item.first)) < 0) {
                return false;
            }
        }
    }

    /* Block devs */
    json_t *block_devs = json_object();

    if (json_object_set_new(root.get(), "block_devs", block_devs) < 0) {
        return false;
    }

    auto const &base_dirs = device.block_dev_base_dirs();
    if (!base_dirs.empty() && json_object_set_new(
            block_devs, "base_dirs", json_string_array(base_dirs)) < 0) {
        return false;
    }

    auto const &system_devs = device.system_block_devs();
    if (!system_devs.empty() && json_object_set_new(
            block_devs, "system", json_string_array(system_devs)) < 0) {
        return false;
    }

    auto const &cache_devs = device.cache_block_devs();
    if (!cache_devs.empty() && json_object_set_new(
            block_devs, "cache", json_string_array(cache_devs)) < 0) {
        return false;
    }

    auto const &data_devs = device.data_block_devs();
    if (!data_devs.empty() && json_object_set_new(
            block_devs, "data", json_string_array(data_devs)) < 0) {
        return false;
    }

    auto const &boot_devs = device.boot_block_devs();
    if (!boot_devs.empty() && json_object_set_new(
            block_devs, "boot", json_string_array(boot_devs)) < 0) {
        return false;
    }

    auto const &recovery_devs = device.recovery_block_devs();
    if (!recovery_devs.empty() && json_object_set_new(
            block_devs, "recovery", json_string_array(recovery_devs)) < 0) {
        return false;
    }

    auto const &extra_devs = device.extra_block_devs();
    if (!extra_devs.empty() && json_object_set_new(
            block_devs, "extra", json_string_array(extra_devs)) < 0) {
        return false;
    }

    /* Boot UI */

    json_t *boot_ui = json_object();

    if (json_object_set_new(root.get(), "boot_ui", boot_ui) < 0) {
        return false;
    }

    if (device.tw_supported() && json_object_set_new(
            boot_ui, "supported", json_true()) < 0) {
        return false;
    }

    auto const tw_flags = device.tw_flags();
    if (tw_flags) {
        json_t *array = json_array();

        if (json_object_set_new(boot_ui, "flags", array) < 0) {
            return false;
        }

        for (auto const &item : g_tw_flag_mappings) {
            if ((tw_flags & item.second) && json_array_append_new(
                    array, json_string(item.first)) < 0) {
                return false;
            }
        }
    }

    auto const pixel_format = device.tw_pixel_format();
    if (pixel_format != TwPixelFormat::Default) {
        for (auto const &item : g_tw_pxfmt_mappings) {
            if (pixel_format == item.second) {
                if (json_object_set_new(boot_ui, "pixel_format",
                                        json_string(item.first)) < 0) {
                    return false;
                }
                break;
            }
        }
    }

    auto const force_pixel_format = device.tw_force_pixel_format();
    if (force_pixel_format != TwForcePixelFormat::None) {
        for (auto const &item : g_tw_force_pxfmt_mappings) {
            if (force_pixel_format == item.second) {
                if (json_object_set_new(boot_ui, "force_pixel_format",
                                        json_string(item.first)) < 0) {
                    return false;
                }
                break;
            }
        }
    }

    auto const overscan_percent = device.tw_overscan_percent();
    if (overscan_percent != 0 && json_object_set_new(
            boot_ui, "overscan_percent", json_integer(overscan_percent)) < 0) {
        return false;
    }

    auto const default_x_offset = device.tw_default_x_offset();
    if (default_x_offset != 0 && json_object_set_new(
            boot_ui, "default_x_offset", json_integer(default_x_offset)) < 0) {
        return false;
    }

    auto const default_y_offset = device.tw_default_y_offset();
    if (default_y_offset != 0 && json_object_set_new(
            boot_ui, "default_y_offset", json_integer(default_y_offset)) < 0) {
        return false;
    }

    auto const &brightness_path = device.tw_brightness_path();
    if (!brightness_path.empty() && json_object_set_new(
            boot_ui, "brightness_path",
            json_string(brightness_path.c_str())) < 0) {
        return false;
    }

    auto const &secondary_brightness_path =
            device.tw_secondary_brightness_path();
    if (!secondary_brightness_path.empty() && json_object_set_new(
            boot_ui, "secondary_brightness_path",
            json_string(secondary_brightness_path.c_str())) < 0) {
        return false;
    }

    auto const max_brightness = device.tw_max_brightness();
    if (max_brightness != -1 && json_object_set_new(
            boot_ui, "max_brightness", json_integer(max_brightness)) < 0) {
        return false;
    }

    auto const default_brightness = device.tw_default_brightness();
    if (default_brightness != -1 && json_object_set_new(
            boot_ui, "default_brightness",
            json_integer(default_brightness)) < 0) {
        return false;
    }

    auto const &battery_path = device.tw_battery_path();
    if (!battery_path.empty() && json_object_set_new(
            boot_ui, "battery_path", json_string(battery_path.c_str())) < 0) {
        return false;
    }

    auto const &cpu_temp_path = device.tw_cpu_temp_path();
    if (!cpu_temp_path.empty() && json_object_set_new(
            boot_ui, "cpu_temp_path", json_string(cpu_temp_path.c_str())) < 0) {
        return false;
    }

    auto const &input_blacklist = device.tw_input_blacklist();
    if (!input_blacklist.empty() && json_object_set_new(
            boot_ui, "input_blacklist",
            json_string(input_blacklist.c_str())) < 0) {
        return false;
    }

    auto const &input_whitelist = device.tw_input_whitelist();
    if (!input_whitelist.empty() && json_object_set_new(
            boot_ui, "input_whitelist",
            json_string(input_whitelist.c_str())) < 0) {
        return false;
    }

    auto const &graphics_backends = device.tw_graphics_backends();
    if (!graphics_backends.empty() && json_object_set_new(
            boot_ui, "graphics_backends",
            json_string_array(graphics_backends)) < 0) {
        return false;
    }

    auto const &theme = device.tw_theme();
    if (!theme.empty() && json_object_set_new(
            boot_ui, "theme", json_string(theme.c_str())) < 0) {
        return false;
    }

    if (json_object_size(boot_ui) == 0) {
        json_object_del(root.get(), "boot_ui");
    }

    std::unique_ptr<char, decltype(free) *> result(
            json_dumps(root.get(), 0), &free);
    if (!result) {
        return false;
    }

    json = result.get();
    return true;
}


}
}
