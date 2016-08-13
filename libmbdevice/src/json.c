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

#include "mbdevice/json.h"

#include <string.h>

#include <jansson.h>

#include "mbdevice/internal/array.h"


#define JSON_BOOLEAN JSON_TRUE

static const char * json_type_to_string(json_type type)
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
        return NULL;
    }
}

static void json_error_set_parse_error(struct MbDeviceJsonError *error,
                                       int line, int column)
{
    error->type = MB_DEVICE_JSON_PARSE_ERROR;
    error->line = line;
    error->column = column;
}

static void json_error_set_mismatched_type(struct MbDeviceJsonError *error,
                                           const char *context,
                                           json_type actual_type,
                                           json_type expected_type)
{
    if (!*context) {
        context = ".";
    }

    error->type = MB_DEVICE_JSON_MISMATCHED_TYPE;
    strncpy(error->context, context, sizeof(error->context));
    error->context[sizeof(error->context) - 1] = '\0';

    const char *str_type = json_type_to_string(actual_type);
    strncpy(error->actual_type, str_type, sizeof(error->actual_type));
    error->actual_type[sizeof(error->actual_type) - 1] = '\0';

    str_type = json_type_to_string(expected_type);
    strncpy(error->expected_type, str_type, sizeof(error->expected_type));
    error->expected_type[sizeof(error->expected_type) - 1] = '\0';
}

static void json_error_set_unknown_key(struct MbDeviceJsonError *error,
                                       const char *context)
{
    if (!*context) {
        context = ".";
    }

    error->type = MB_DEVICE_JSON_UNKNOWN_KEY;
    strncpy(error->context, context, sizeof(error->context));
    error->context[sizeof(error->context) - 1] = '\0';
}

static void json_error_set_unknown_value(struct MbDeviceJsonError *error,
                                         const char *context)
{
    if (!*context) {
        context = ".";
    }

    error->type = MB_DEVICE_JSON_UNKNOWN_VALUE;
    strncpy(error->context, context, sizeof(error->context));
    error->context[sizeof(error->context) - 1] = '\0';
}

typedef int (*setter_boolean)(struct Device *, bool);
typedef int (*setter_int)(struct Device *, int);
typedef int (*setter_string)(struct Device *, const char *);
typedef int (*setter_string_array)(struct Device *, char const * const *);

static inline int device_set_boolean(setter_boolean fn, struct Device *device,
                                     json_t *value, const char *context,
                                     struct MbDeviceJsonError *error)
{
    if (!json_is_boolean(value)) {
        json_error_set_mismatched_type(
                    error, context, value->type, JSON_BOOLEAN);
        return MB_DEVICE_ERROR_JSON;
    }
    return fn(device, json_boolean_value(value));
}

static inline int device_set_int(setter_int fn, struct Device *device,
                                 json_t *value, const char *context,
                                 struct MbDeviceJsonError *error)
{
    if (!json_is_integer(value)) {
        json_error_set_mismatched_type(
                error, context, value->type, JSON_INTEGER);
        return MB_DEVICE_ERROR_JSON;
    }
    return fn(device, json_integer_value(value));
}

static inline int device_set_string(setter_string fn, struct Device *device,
                                    json_t *value, const char *context,
                                    struct MbDeviceJsonError *error)
{
    if (!json_is_string(value)) {
        json_error_set_mismatched_type(
                error, context, value->type, JSON_STRING);
        return MB_DEVICE_ERROR_JSON;
    }
    return fn(device, json_string_value(value));
}

static inline int device_set_string_array(setter_string_array fn,
                                          struct Device *device,
                                          json_t *node, const char *context,
                                          struct MbDeviceJsonError *error)
{
    int ret = MB_DEVICE_OK;
    char subcontext[100];
    const char **array = NULL;
    size_t array_size;
    size_t index;
    json_t *value;

    if (!json_is_array(node)) {
        json_error_set_mismatched_type(
                error, context, node->type, JSON_ARRAY);
        ret = MB_DEVICE_ERROR_JSON;
        goto done;
    }

    array_size = (json_array_size(node) + 1) * sizeof(char *);
    array = (const char **) malloc(array_size);
    if (!array) {
        ret = MB_DEVICE_ERROR_ERRNO;
        goto done;
    }
    memset(array, 0, array_size);

    json_array_foreach(node, index, value) {
        snprintf(subcontext, sizeof(subcontext), "%s[%zu]", context, index);

        if (!json_is_string(value)) {
            json_error_set_mismatched_type(
                    error, subcontext, value->type, JSON_STRING);
            ret = MB_DEVICE_ERROR_JSON;
            goto done;
        }

        array[index] = json_string_value(value);
    }

    ret = fn(device, array);

done:
    free(array);
    return ret;
}

static int process_boot_ui_flags(struct Device *device, json_t *node,
                                 const char *context,
                                 struct MbDeviceJsonError *error)
{
    char subcontext[100];
    size_t index;
    json_t *value;
    uint64_t flags = 0;

    struct mapping {
        const char *key;
        uint64_t flag;
    };

    struct mapping mappings[] = {
#define FLAG(F) { #F, FLAG_ ## F }
        FLAG(TW_TOUCHSCREEN_SWAP_XY),
        FLAG(TW_TOUCHSCREEN_FLIP_X),
        FLAG(TW_TOUCHSCREEN_FLIP_Y),
        FLAG(TW_GRAPHICS_FORCE_USE_LINELENGTH),
        FLAG(TW_SCREEN_BLANK_ON_BOOT),
        FLAG(TW_BOARD_HAS_FLIPPED_SCREEN),
        FLAG(TW_IGNORE_MAJOR_AXIS_0),
        FLAG(TW_IGNORE_MT_POSITION_0),
        FLAG(TW_IGNORE_ABS_MT_TRACKING_ID),
        FLAG(TW_NEW_ION_HEAP),
        FLAG(TW_NO_SCREEN_BLANK),
        FLAG(TW_NO_SCREEN_TIMEOUT),
        FLAG(TW_ROUND_SCREEN),
        FLAG(TW_NO_CPU_TEMP),
        FLAG(TW_QCOM_RTC_FIX),
        FLAG(TW_HAS_DOWNLOAD_MODE),
        FLAG(TW_PREFER_LCD_BACKLIGHT),
#undef FLAG
        { NULL, 0 }
    };

    if (!json_is_array(node)) {
        json_error_set_mismatched_type(error, context, node->type, JSON_ARRAY);
        return MB_DEVICE_ERROR_JSON;
    }

    json_array_foreach(node, index, value) {
        snprintf(subcontext, sizeof(subcontext), "%s[%zu]", context, index);

        if (!json_is_string(value)) {
            json_error_set_mismatched_type(
                    error, subcontext, value->type, JSON_STRING);
            return MB_DEVICE_ERROR_JSON;
        }

        const char *str = json_string_value(value);
        uint64_t old_flags = flags;

        for (struct mapping *m = mappings; m->key; ++m) {
            if (strcmp(m->key, str) == 0) {
                flags |= m->flag;
                break;
            }
        }

        if (flags == old_flags) {
            json_error_set_unknown_value(error, subcontext);
            return MB_DEVICE_ERROR_JSON;
        }
    }

    return mb_device_set_tw_flags(device, flags);
}

static int process_boot_ui_pixel_format(struct Device *device, json_t *node,
                                        const char *context,
                                        struct MbDeviceJsonError *error)
{
    const char *str;

    struct mapping {
        const char *key;
        enum TwPixelFormat value;
    };

    struct mapping mappings[] = {
#define FLAG(F) { #F, TW_PIXEL_FORMAT_ ## F }
        FLAG(DEFAULT),
        FLAG(ABGR_8888),
        FLAG(RGBX_8888),
        FLAG(BGRA_8888),
        FLAG(RGBA_8888),
#undef FLAG
        { NULL, TW_PIXEL_FORMAT_DEFAULT }
    };

    if (!json_is_string(node)) {
        json_error_set_mismatched_type(error, context, node->type, JSON_STRING);
        return MB_DEVICE_ERROR_JSON;
    }

    str = json_string_value(node);

    for (struct mapping *m = mappings; m->key; ++m) {
        if (strcmp(m->key, str) == 0) {
            return mb_device_set_tw_pixel_format(device, m->value);
        }
    }

    json_error_set_unknown_value(error, context);
    return MB_DEVICE_ERROR_JSON;
}

static int process_boot_ui_force_pixel_format(struct Device *device, json_t *node,
                                              const char *context,
                                              struct MbDeviceJsonError *error)
{
    const char *str;

    struct mapping {
        const char *key;
        enum TwForcePixelFormat value;
    };

    struct mapping mappings[] = {
#define FLAG(F) { #F, TW_FORCE_PIXEL_FORMAT_ ## F }
        FLAG(NONE),
        FLAG(RGB_565),
#undef FLAG
        { NULL, TW_FORCE_PIXEL_FORMAT_NONE }
    };

    if (!json_is_string(node)) {
        json_error_set_mismatched_type(error, context, node->type, JSON_STRING);
        return MB_DEVICE_ERROR_JSON;
    }

    str = json_string_value(node);

    for (struct mapping *m = mappings; m->key; ++m) {
        if (strcmp(m->key, str) == 0) {
            return mb_device_set_tw_force_pixel_format(device, m->value);
        }
    }

    json_error_set_unknown_value(error, context);
    return MB_DEVICE_ERROR_JSON;
}

static int process_boot_ui(struct Device *device, json_t *node,
                           const char *context,
                           struct MbDeviceJsonError *error)
{
    int ret = MB_DEVICE_OK;
    char subcontext[100];
    const char *key;
    json_t *value;

    if (!json_is_object(node)) {
        json_error_set_mismatched_type(
                error, context, node->type, JSON_OBJECT);
        return MB_DEVICE_ERROR_JSON;
    }

    json_object_foreach(node, key, value) {
        snprintf(subcontext, sizeof(subcontext), "%s.%s", context, key);

        if (strcmp(key, "supported") == 0) {
            ret = device_set_boolean(&mb_device_set_tw_supported,
                                     device, value, subcontext, error);
        } else if (strcmp(key, "flags") == 0) {
            ret = process_boot_ui_flags(
                    device, value, subcontext, error);
        } else if (strcmp(key, "pixel_format") == 0) {
            ret = process_boot_ui_pixel_format(
                    device, value, subcontext, error);
        } else if (strcmp(key, "force_pixel_format") == 0) {
            ret = process_boot_ui_force_pixel_format(
                    device, value, subcontext, error);
        } else if (strcmp(key, "overscan_percent") == 0) {
            ret = device_set_int(&mb_device_set_tw_overscan_percent,
                                 device, value, subcontext, error);
        } else if (strcmp(key, "default_x_offset") == 0) {
            ret = device_set_int(&mb_device_set_tw_default_x_offset,
                                 device, value, subcontext, error);
        } else if (strcmp(key, "default_y_offset") == 0) {
            ret = device_set_int(&mb_device_set_tw_default_y_offset,
                                 device, value, subcontext, error);
        } else if (strcmp(key, "brightness_path") == 0) {
            ret = device_set_string(&mb_device_set_tw_brightness_path,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "secondary_brightness_path") == 0) {
            ret = device_set_string(&mb_device_set_tw_secondary_brightness_path,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "max_brightness") == 0) {
            ret = device_set_int(&mb_device_set_tw_max_brightness,
                                 device, value, subcontext, error);
        } else if (strcmp(key, "default_brightness") == 0) {
            ret = device_set_int(&mb_device_set_tw_default_brightness,
                                 device, value, subcontext, error);
        } else if (strcmp(key, "battery_path") == 0) {
            ret = device_set_string(&mb_device_set_tw_battery_path,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "cpu_temp_path") == 0) {
            ret = device_set_string(&mb_device_set_tw_cpu_temp_path,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "input_blacklist") == 0) {
            ret = device_set_string(&mb_device_set_tw_input_blacklist,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "input_whitelist") == 0) {
            ret = device_set_string(&mb_device_set_tw_input_whitelist,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "graphics_backends") == 0) {
            ret = device_set_string_array(&mb_device_set_tw_graphics_backends,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "theme") == 0) {
            ret = device_set_string(&mb_device_set_tw_theme,
                                    device, value, subcontext, error);
        } else {
            json_error_set_unknown_key(error, subcontext);
            ret = MB_DEVICE_ERROR_JSON;
        }

        if (ret != MB_DEVICE_OK) {
            break;
        }
    }

    return ret;
}

static int process_crypto(struct Device *device, json_t *node,
                          const char *context,
                          struct MbDeviceJsonError *error)
{
    int ret = MB_DEVICE_OK;
    char subcontext[100];
    const char *key;
    json_t *value;

    if (!json_is_object(node)) {
        json_error_set_mismatched_type(
                error, context, node->type, JSON_OBJECT);
        return MB_DEVICE_ERROR_JSON;
    }

    json_object_foreach(node, key, value) {
        snprintf(subcontext, sizeof(subcontext), "%s.%s", context, key);

        if (strcmp(key, "supported") == 0) {
            ret = device_set_boolean(&mb_device_set_crypto_supported,
                                     device, value, subcontext, error);
        } else if (strcmp(key, "header_path") == 0) {
            ret = device_set_string(&mb_device_set_crypto_header_path,
                                    device, value, subcontext, error);
        } else {
            json_error_set_unknown_key(error, subcontext);
            ret = MB_DEVICE_ERROR_JSON;
        }

        if (ret != MB_DEVICE_OK) {
            break;
        }
    }

    return ret;
}

static int process_block_devs(struct Device *device, json_t *node,
                              const char *context,
                              struct MbDeviceJsonError *error)
{
    int ret = MB_DEVICE_OK;
    char subcontext[100];
    const char *key;
    json_t *value;

    if (!json_is_object(node)) {
        json_error_set_mismatched_type(
                error, context, node->type, JSON_OBJECT);
        return MB_DEVICE_ERROR_JSON;
    }

    json_object_foreach(node, key, value) {
        snprintf(subcontext, sizeof(subcontext), "%s.%s", context, key);

        if (strcmp(key, "base_dirs") == 0) {
            ret = device_set_string_array(&mb_device_set_block_dev_base_dirs,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "system") == 0) {
            ret = device_set_string_array(&mb_device_set_system_block_devs,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "cache") == 0) {
            ret = device_set_string_array(&mb_device_set_cache_block_devs,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "data") == 0) {
            ret = device_set_string_array(&mb_device_set_data_block_devs,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "boot") == 0) {
            ret = device_set_string_array(&mb_device_set_boot_block_devs,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "recovery") == 0) {
            ret = device_set_string_array(&mb_device_set_recovery_block_devs,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "extra") == 0) {
            ret = device_set_string_array(&mb_device_set_extra_block_devs,
                                          device, value, subcontext, error);
        } else {
            json_error_set_unknown_key(error, subcontext);
            ret = MB_DEVICE_ERROR_JSON;
        }

        if (ret != MB_DEVICE_OK) {
            break;
        }
    }

    return ret;
}

static int process_device(struct Device *device, json_t *node,
                          const char *context,
                          struct MbDeviceJsonError *error)
{
    int ret = MB_DEVICE_OK;
    char subcontext[100];
    const char *key;
    json_t *value;

    if (!json_is_object(node)) {
        json_error_set_mismatched_type(
                error, context, node->type, JSON_OBJECT);
        return MB_DEVICE_ERROR_JSON;
    }

    json_object_foreach(node, key, value) {
        snprintf(subcontext, sizeof(subcontext), "%s.%s", context, key);

        if (strcmp(key, "name") == 0) {
            ret = device_set_string(&mb_device_set_name,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "id") == 0) {
            ret = device_set_string(&mb_device_set_id,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "codenames") == 0) {
            ret = device_set_string_array(&mb_device_set_codenames,
                                          device, value, subcontext, error);
        } else if (strcmp(key, "architecture") == 0) {
            ret = device_set_string(&mb_device_set_architecture,
                                    device, value, subcontext, error);
        } else if (strcmp(key, "block_devs") == 0) {
            ret = process_block_devs(device, value, subcontext, error);
        } else if (strcmp(key, "boot_ui") == 0) {
            ret = process_boot_ui(device, value, subcontext, error);
        } else if (strcmp(key, "crypto") == 0) {
            ret = process_crypto(device, value, subcontext, error);
        } else {
            json_error_set_unknown_key(error, subcontext);
            ret = MB_DEVICE_ERROR_JSON;
        }

        if (ret != MB_DEVICE_OK) {
            break;
        }
    }

    return ret;
}

/*!
 * \brief Create new device definition object from its JSON representation
 */
int mb_device_load_json(struct Device *device, const char *json,
                        struct MbDeviceJsonError *error)
{
    int ret = MB_DEVICE_OK;
    json_t *root = NULL;
    json_error_t json_error;

    root = json_loads(json, 0, &json_error);
    if (!root) {
        json_error_set_parse_error(error, error->line, error->column);
        ret = MB_DEVICE_ERROR_JSON;
        goto done;
    }

    ret = process_device(device, root, "", error);

done:
    if (root) {
        json_decref(root);
    }
    return ret;
}
