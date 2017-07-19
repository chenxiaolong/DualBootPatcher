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

#include "mbdevice/json.h"

#include <string.h>

#include <jansson.h>

#include "mbcommon/string.h"

#include "mbdevice/internal/array.h"
#include "mbdevice/internal/structs.h"


#define JSON_BOOLEAN JSON_TRUE

struct flag_mapping
{
    const char *key;
    uint64_t flag;
};

struct flag_mapping device_flag_mappings[] = {
#define FLAG(F) { #F, FLAG_ ## F }
    FLAG(HAS_COMBINED_BOOT_AND_RECOVERY),
    FLAG(FSTAB_SKIP_SDCARD0),
#undef FLAG
    { NULL, 0 }
};

struct flag_mapping tw_flag_mappings[] = {
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

struct tw_pxfmt_mapping
{
    const char *key;
    enum TwPixelFormat value;
};

struct tw_pxfmt_mapping tw_pxfmt_mappings[] = {
#define FLAG(F) { #F, TW_PIXEL_FORMAT_ ## F }
    FLAG(DEFAULT),
    FLAG(ABGR_8888),
    FLAG(RGBX_8888),
    FLAG(BGRA_8888),
    FLAG(RGBA_8888),
#undef FLAG
    { NULL, TW_PIXEL_FORMAT_DEFAULT }
};

struct tw_force_pxfmt_mapping
{
    const char *key;
    enum TwForcePixelFormat value;
};

struct tw_force_pxfmt_mapping tw_force_pxfmt_mappings[] = {
#define FLAG(F) { #F, TW_FORCE_PIXEL_FORMAT_ ## F }
    FLAG(NONE),
    FLAG(RGB_565),
#undef FLAG
    { NULL, TW_FORCE_PIXEL_FORMAT_NONE }
};

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

static void json_error_set_standard_error(struct MbDeviceJsonError *error,
                                          int std_error)
{
    error->type = MB_DEVICE_JSON_STANDARD_ERROR;
    error->std_error = std_error;
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
    int ret;

    if (!json_is_boolean(value)) {
        json_error_set_mismatched_type(
                error, context, value->type, JSON_BOOLEAN);
        return -1;
    }

    ret = fn(device, json_boolean_value(value));
    if (ret < 0) {
        json_error_set_standard_error(error, ret);
        return -1;
    }

    return 0;
}

static inline int device_set_int(setter_int fn, struct Device *device,
                                 json_t *value, const char *context,
                                 struct MbDeviceJsonError *error)
{
    int ret;

    if (!json_is_integer(value)) {
        json_error_set_mismatched_type(
                error, context, value->type, JSON_INTEGER);
        return -1;
    }

    ret = fn(device, json_integer_value(value));
    if (ret < 0) {
        json_error_set_standard_error(error, ret);
        return -1;
    }

    return 0;
}

static inline int device_set_string(setter_string fn, struct Device *device,
                                    json_t *value, const char *context,
                                    struct MbDeviceJsonError *error)
{
    int ret;

    if (!json_is_string(value)) {
        json_error_set_mismatched_type(
                error, context, value->type, JSON_STRING);
        return -1;
    }

    ret = fn(device, json_string_value(value));
    if (ret < 0) {
        json_error_set_standard_error(error, ret);
        return -1;
    }

    return 0;
}

static inline int device_set_string_array(setter_string_array fn,
                                          struct Device *device,
                                          json_t *node, const char *context,
                                          struct MbDeviceJsonError *error)
{
    int ret = 0;
    int fn_ret;
    char subcontext[100];
    const char **array = NULL;
    size_t array_size;
    size_t index;
    json_t *value;

    if (!json_is_array(node)) {
        json_error_set_mismatched_type(
                error, context, node->type, JSON_ARRAY);
        ret = -1;
        goto done;
    }

    array_size = (json_array_size(node) + 1) * sizeof(char *);
    array = (const char **) malloc(array_size);
    if (!array) {
        json_error_set_standard_error(error, MB_DEVICE_ERROR_ERRNO);
        ret = -1;
        goto done;
    }
    memset(array, 0, array_size);

    json_array_foreach(node, index, value) {
        snprintf(subcontext, sizeof(subcontext),
                 "%s[%" MB_PRIzu "]", context, index);

        if (!json_is_string(value)) {
            json_error_set_mismatched_type(
                    error, subcontext, value->type, JSON_STRING);
            ret = -1;
            goto done;
        }

        array[index] = json_string_value(value);
    }

    fn_ret = fn(device, array);
    if (fn_ret < 0) {
        json_error_set_standard_error(error, fn_ret);
        ret = -1;
        goto done;
    }

done:
    free(array);
    return ret;
}

static int process_device_flags(struct Device *device, json_t *node,
                                const char *context,
                                struct MbDeviceJsonError *error)
{
    char subcontext[100];
    size_t index;
    json_t *value;
    uint64_t flags = 0;

    if (!json_is_array(node)) {
        json_error_set_mismatched_type(error, context, node->type, JSON_ARRAY);
        return -1;
    }

    json_array_foreach(node, index, value) {
        snprintf(subcontext, sizeof(subcontext),
                 "%s[%" MB_PRIzu "]", context, index);

        if (!json_is_string(value)) {
            json_error_set_mismatched_type(
                    error, subcontext, value->type, JSON_STRING);
            return -1;
        }

        const char *str = json_string_value(value);
        uint64_t old_flags = flags;

        for (struct flag_mapping *m = device_flag_mappings; m->key; ++m) {
            if (strcmp(m->key, str) == 0) {
                flags |= m->flag;
                break;
            }
        }

        if (flags == old_flags) {
            json_error_set_unknown_value(error, subcontext);
            return -1;
        }
    }

    int fn_ret = mb_device_set_flags(device, flags);
    if (fn_ret < 0) {
        json_error_set_standard_error(error, fn_ret);
        return -1;
    }

    return 0;
}

static int process_boot_ui_flags(struct Device *device, json_t *node,
                                 const char *context,
                                 struct MbDeviceJsonError *error)
{
    char subcontext[100];
    size_t index;
    json_t *value;
    uint64_t flags = 0;

    if (!json_is_array(node)) {
        json_error_set_mismatched_type(error, context, node->type, JSON_ARRAY);
        return -1;
    }

    json_array_foreach(node, index, value) {
        snprintf(subcontext, sizeof(subcontext),
                 "%s[%" MB_PRIzu "]", context, index);

        if (!json_is_string(value)) {
            json_error_set_mismatched_type(
                    error, subcontext, value->type, JSON_STRING);
            return -1;
        }

        const char *str = json_string_value(value);
        uint64_t old_flags = flags;

        for (struct flag_mapping *m = tw_flag_mappings; m->key; ++m) {
            if (strcmp(m->key, str) == 0) {
                flags |= m->flag;
                break;
            }
        }

        if (flags == old_flags) {
            json_error_set_unknown_value(error, subcontext);
            return -1;
        }
    }

    int fn_ret = mb_device_set_tw_flags(device, flags);
    if (fn_ret < 0) {
        json_error_set_standard_error(error, fn_ret);
        return -1;
    }

    return 0;
}

static int process_boot_ui_pixel_format(struct Device *device, json_t *node,
                                        const char *context,
                                        struct MbDeviceJsonError *error)
{
    const char *str;

    if (!json_is_string(node)) {
        json_error_set_mismatched_type(error, context, node->type, JSON_STRING);
        return -1;
    }

    str = json_string_value(node);

    for (struct tw_pxfmt_mapping *m = tw_pxfmt_mappings; m->key; ++m) {
        if (strcmp(m->key, str) == 0) {
            int fn_ret = mb_device_set_tw_pixel_format(device, m->value);
            if (fn_ret < 0) {
                json_error_set_standard_error(error, fn_ret);
                return -1;
            }
            return 0;
        }
    }

    json_error_set_unknown_value(error, context);
    return -1;
}

static int process_boot_ui_force_pixel_format(struct Device *device, json_t *node,
                                              const char *context,
                                              struct MbDeviceJsonError *error)
{
    const char *str;

    if (!json_is_string(node)) {
        json_error_set_mismatched_type(error, context, node->type, JSON_STRING);
        return -1;
    }

    str = json_string_value(node);

    for (struct tw_force_pxfmt_mapping *m = tw_force_pxfmt_mappings;
            m->key; ++m) {
        if (strcmp(m->key, str) == 0) {
            int fn_ret = mb_device_set_tw_force_pixel_format(device, m->value);
            if (fn_ret < 0) {
                json_error_set_standard_error(error, fn_ret);
                return -1;
            }
            return 0;
        }
    }

    json_error_set_unknown_value(error, context);
    return -1;
}

static int process_boot_ui(struct Device *device, json_t *node,
                           const char *context,
                           struct MbDeviceJsonError *error)
{
    int ret = 0;
    char subcontext[100];
    const char *key;
    json_t *value;

    if (!json_is_object(node)) {
        json_error_set_mismatched_type(
                error, context, node->type, JSON_OBJECT);
        return -1;
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
            ret = -1;
        }

        if (ret != 0) {
            break;
        }
    }

    return ret;
}

static int process_block_devs(struct Device *device, json_t *node,
                              const char *context,
                              struct MbDeviceJsonError *error)
{
    int ret = 0;
    char subcontext[100];
    const char *key;
    json_t *value;

    if (!json_is_object(node)) {
        json_error_set_mismatched_type(
                error, context, node->type, JSON_OBJECT);
        return -1;
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
            ret = -1;
        }

        if (ret != 0) {
            break;
        }
    }

    return ret;
}

static int process_device(struct Device *device, json_t *node,
                          const char *context,
                          struct MbDeviceJsonError *error)
{
    int ret = 0;
    char subcontext[100];
    const char *key;
    json_t *value;

    if (!json_is_object(node)) {
        json_error_set_mismatched_type(
                error, context, node->type, JSON_OBJECT);
        return -1;
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
        } else if (strcmp(key, "flags") == 0) {
            ret = process_device_flags(device, value, subcontext, error);
        } else if (strcmp(key, "block_devs") == 0) {
            ret = process_block_devs(device, value, subcontext, error);
        } else if (strcmp(key, "boot_ui") == 0) {
            ret = process_boot_ui(device, value, subcontext, error);
        } else {
            json_error_set_unknown_key(error, subcontext);
            ret = -1;
        }

        if (ret != 0) {
            break;
        }
    }

    return ret;
}

struct Device * mb_device_new_from_json(const char *json,
                                        struct MbDeviceJsonError *error)
{
    struct Device *device = NULL;
    json_t *root = NULL;
    json_error_t json_error;
    bool ok = true;

    root = json_loads(json, 0, &json_error);
    if (!root) {
        json_error_set_parse_error(error, json_error.line, json_error.column);
        ok = false;
        goto done;
    }

    device = mb_device_new();
    if (!device) {
        json_error_set_standard_error(error, MB_DEVICE_ERROR_ERRNO);
        ok = false;
        goto done;
    }

    if (process_device(device, root, "", error) < 0) {
        ok = false;
        goto done;
    }

done:
    if (root) {
        json_decref(root);
    }
    if (!ok) {
        mb_device_free(device);
    }
    return ok ? device : NULL;
}

struct Device ** mb_device_new_list_from_json(const char *json,
                                              struct MbDeviceJsonError *error)
{
    struct Device **devices = NULL;
    size_t devices_size;
    json_t *root = NULL;
    json_t *elem;
    size_t index;
    json_error_t json_error;
    bool ok = true;
    char context[100];

    root = json_loads(json, 0, &json_error);
    if (!root) {
        json_error_set_parse_error(error, json_error.line, json_error.column);
        ok = false;
        goto done;
    }

    if (!json_is_array(root)) {
        json_error_set_mismatched_type(error, "", root->type, JSON_ARRAY);
        ok = false;
        goto done;
    }

    devices_size = (json_array_size(root) + 1) * sizeof(struct Device *);
    devices = (struct Device **) malloc(devices_size);
    if (!devices) {
        json_error_set_standard_error(error, MB_DEVICE_ERROR_ERRNO);
        ok = false;
        goto done;
    }
    memset(devices, 0, devices_size);

    json_array_foreach(root, index, elem) {
        snprintf(context, sizeof(context), "[%" MB_PRIzu "]", index);

        devices[index] = mb_device_new();
        if (!devices[index]) {
            json_error_set_standard_error(error, MB_DEVICE_ERROR_ERRNO);
            ok = false;
            goto done;
        }

        if (process_device(devices[index], elem, context, error) < 0) {
            ok = false;
            goto done;
        }
    }

done:
    if (root) {
        json_decref(root);
    }
    if (!ok && devices) {
        for (struct Device **iter = devices; *iter; ++iter) {
            mb_device_free(*iter);
        }
        free(devices);
    }
    return ok ? devices : NULL;
}

static json_t * json_string_array(char * const *array)
{
    json_t *j_array = NULL;

    j_array = json_array();
    if (!j_array) {
        goto error;
    }

    for (char * const *it = array; *it; ++it) {
        if (json_array_append_new(j_array, json_string(*it)) < 0) {
            goto error;
        }
    }

    return j_array;

error:
    if (j_array) {
        json_decref(j_array);
    }
    return NULL;
}

char * mb_device_to_json(struct Device *device)
{
    json_t *root = NULL;
    json_t *block_devs = NULL;
    json_t *boot_ui = NULL;
    char *result = NULL;

    root = json_object();
    if (!root) {
        goto done;
    }

    if (device->id && json_object_set_new(
            root, "id", json_string(device->id)) < 0) {
        goto done;
    }

    if (device->codenames && json_object_set_new(
            root, "codenames", json_string_array(device->codenames)) < 0) {
        goto done;
    }

    if (device->name && json_object_set_new(
            root, "name", json_string(device->name)) < 0) {
        goto done;
    }

    if (device->architecture && json_object_set_new(
            root, "architecture", json_string(device->architecture)) < 0) {
        goto done;
    }

    if (device->flags != 0) {
        json_t *array = json_array();

        if (json_object_set_new(root, "flags", array) < 0) {
            goto done;
        }

        for (struct flag_mapping *it = device_flag_mappings; it->key; ++it) {
            if ((device->flags & it->flag)
                    && json_array_append_new(array, json_string(it->key)) < 0) {
                goto done;
            }
        }
    }

    /* Block devs */
    block_devs = json_object();

    if (json_object_set_new(root, "block_devs", block_devs) < 0) {
        goto done;
    }

    if (device->base_dirs && json_object_set_new(
            block_devs, "base_dirs",
            json_string_array(device->base_dirs)) < 0) {
        goto done;
    }

    if (device->system_devs && json_object_set_new(
            block_devs, "system",
            json_string_array(device->system_devs)) < 0) {
        goto done;
    }

    if (device->cache_devs && json_object_set_new(
            block_devs, "cache",
            json_string_array(device->cache_devs)) < 0) {
        goto done;
    }

    if (device->data_devs && json_object_set_new(
            block_devs, "data",
            json_string_array(device->data_devs)) < 0) {
        goto done;
    }

    if (device->boot_devs && json_object_set_new(
            block_devs, "boot",
            json_string_array(device->boot_devs)) < 0) {
        goto done;
    }

    if (device->recovery_devs && json_object_set_new(
            block_devs, "recovery",
            json_string_array(device->recovery_devs)) < 0) {
        goto done;
    }

    if (device->extra_devs && json_object_set_new(
            block_devs, "extra",
            json_string_array(device->extra_devs)) < 0) {
        goto done;
    }

    /* Boot UI */

    boot_ui = json_object();

    if (json_object_set_new(root, "boot_ui", boot_ui) < 0) {
        goto done;
    }

    if (device->tw_options.supported && json_object_set_new(
            boot_ui, "supported", json_true()) < 0) {
        goto done;
    }

    if (device->tw_options.flags != 0) {
        json_t *array = json_array();

        if (json_object_set_new(boot_ui, "flags", array) < 0) {
            goto done;
        }

        for (struct flag_mapping *it = tw_flag_mappings; it->key; ++it) {
            if ((device->tw_options.flags & it->flag)
                    && json_array_append_new(array, json_string(it->key)) < 0) {
                goto done;
            }
        }
    }

    if (device->tw_options.pixel_format != TW_PIXEL_FORMAT_DEFAULT) {
        for (struct tw_pxfmt_mapping *it = tw_pxfmt_mappings; it->key; ++it) {
            if (device->tw_options.pixel_format == it->value) {
                if (json_object_set_new(boot_ui, "pixel_format",
                                        json_string(it->key)) < 0) {
                    goto done;
                }
                break;
            }
        }
    }

    if (device->tw_options.force_pixel_format != TW_FORCE_PIXEL_FORMAT_NONE) {
        for (struct tw_force_pxfmt_mapping *it = tw_force_pxfmt_mappings;
                it->key; ++it) {
            if (device->tw_options.force_pixel_format == it->value) {
                if (json_object_set_new(boot_ui, "force_pixel_format",
                                        json_string(it->key)) < 0) {
                    goto done;
                }
                break;
            }
        }
    }

    if (device->tw_options.overscan_percent != 0 && json_object_set_new(
            boot_ui, "overscan_percent",
            json_integer(device->tw_options.overscan_percent)) < 0) {
        goto done;
    }

    if (device->tw_options.default_x_offset != 0 && json_object_set_new(
            boot_ui, "default_x_offset",
            json_integer(device->tw_options.default_x_offset)) < 0) {
        goto done;
    }

    if (device->tw_options.default_y_offset != 0 && json_object_set_new(
            boot_ui, "default_y_offset",
            json_integer(device->tw_options.default_y_offset)) < 0) {
        goto done;
    }

    if (device->tw_options.brightness_path && json_object_set_new(
            boot_ui, "brightness_path",
            json_string(device->tw_options.brightness_path)) < 0) {
        goto done;
    }

    if (device->tw_options.secondary_brightness_path && json_object_set_new(
            boot_ui, "secondary_brightness_path",
            json_string(device->tw_options.secondary_brightness_path)) < 0) {
        goto done;
    }

    if (device->tw_options.max_brightness != -1 && json_object_set_new(
            boot_ui, "max_brightness",
            json_integer(device->tw_options.max_brightness)) < 0) {
        goto done;
    }

    if (device->tw_options.default_brightness != -1 && json_object_set_new(
            boot_ui, "default_brightness",
            json_integer(device->tw_options.default_brightness)) < 0) {
        goto done;
    }

    if (device->tw_options.battery_path && json_object_set_new(
            boot_ui, "battery_path",
            json_string(device->tw_options.battery_path)) < 0) {
        goto done;
    }

    if (device->tw_options.cpu_temp_path && json_object_set_new(
            boot_ui, "cpu_temp_path",
            json_string(device->tw_options.cpu_temp_path)) < 0) {
        goto done;
    }

    if (device->tw_options.input_blacklist && json_object_set_new(
            boot_ui, "input_blacklist",
            json_string(device->tw_options.input_blacklist)) < 0) {
        goto done;
    }

    if (device->tw_options.input_whitelist && json_object_set_new(
            boot_ui, "input_whitelist",
            json_string(device->tw_options.input_whitelist)) < 0) {
        goto done;
    }

    if (device->tw_options.graphics_backends && json_object_set_new(
            boot_ui, "graphics_backends",
            json_string_array(device->tw_options.graphics_backends)) < 0) {
        goto done;
    }

    if (device->tw_options.theme && json_object_set_new(
            boot_ui, "theme", json_string(device->tw_options.theme)) < 0) {
        goto done;
    }

    if (json_object_size(boot_ui) == 0) {
        json_object_del(root, "boot_ui");
    }

    result = json_dumps(root, 0);

done:
    if (root) {
        json_decref(root);
    }
    return result;
}
