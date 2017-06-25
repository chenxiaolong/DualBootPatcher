/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbdevice/device.h"

#include <stdlib.h>
#include <string.h>

#include "mbdevice/internal/array.h"
#include "mbdevice/internal/structs.h"


#define GETTER(TYPE, NAME) \
    TYPE mb_device_ ## NAME (const struct Device *device)
#define SETTER(TYPE, NAME) \
    int mb_device_set_ ## NAME (struct Device *device, TYPE value)

#define STRING_GETTER(TARGET)   \
    {                           \
        return (TARGET);        \
    }

#define STRING_ARRAY_GETTER(TARGET)             \
    {                                           \
        return (char const * const *) (TARGET); \
    }

#define STRING_SETTER(TARGET, VALUE)                \
    {                                               \
        char *dup = NULL;                           \
        if ((VALUE) && !(dup = strdup(VALUE))) {    \
            return MB_DEVICE_ERROR_ERRNO;           \
        }                                           \
        free(TARGET);                               \
        (TARGET) = dup;                             \
        return MB_DEVICE_OK;                        \
    }

#define STRING_ARRAY_SETTER(TARGET, VALUE)                  \
    {                                                       \
        char **dup = NULL;                                  \
        if ((VALUE) && !(dup = string_array_dup(VALUE))) {  \
            return MB_DEVICE_ERROR_ERRNO;                   \
        }                                                   \
        string_array_free(TARGET);                          \
        (TARGET) = dup;                                     \
        return MB_DEVICE_OK;                                \
    }


/*!
 * \brief Create new device definition object
 *
 * \return New object or NULL if memory allocation is unsuccessful
 */
struct Device * mb_device_new()
{
    struct Device *device = (struct Device *) malloc(sizeof(struct Device));
    if (!device) {
        return NULL;
    }

    if (!initialize_device(device)) {
        free(device);
        return NULL;
    }

    return device;
}

/*!
 * \brief Free device definition object
 *
 * \param device Object to free (can be NULL)
 */
void mb_device_free(struct Device *device)
{
    if (device) {
        cleanup_device(device);
        free(device);
    }
}

/*!
 * \brief Get the device ID
 *
 * \param device Device definition object
 * \return Device ID or NULL if unset
 */
GETTER(const char *, id)
{
    STRING_GETTER(device->id)
}

/*!
 * \brief Set the device ID
 *
 * \note The object is not changed if this function returns an error.
 *
 * \param device Device definition object
 * \param value Device ID or NULL to clear field
 * \return Whether the field was successfully set
 */
SETTER(const char *, id)
{
    STRING_SETTER(device->id, value)
}

/*!
 * \brief Get the device codenames
 *
 * \param device Device definition object
 * \return NULL-terminated list of device names or NULL if unset
 */
GETTER(char const * const *, codenames)
{
    STRING_ARRAY_GETTER(device->codenames)
}

/*!
 * \brief Set the device codenames
 *
 * \note This object is not changed if this function returns an error.
 *
 * \param device Device definition object
 * \param value NULL-terminated device codenames or NULL to clear field
 * \return Whether the field was successfully set
 */
SETTER(char const * const *, codenames)
{
    STRING_ARRAY_SETTER(device->codenames, value)
}

/*!
 * \brief Get the device name
 *
 * \param device Device definition object
 * \return Device name or NULL if unset
 */
GETTER(const char *, name)
{
    STRING_GETTER(device->name)
}

/*!
 * \brief Set the device name
 *
 * \note This object is not changed if this function returns an error;
 *
 * \param device Device definition object
 * \param value Device name or NULL to clear field
 * \return Whether the field was successfully set
 */
SETTER(const char *, name)
{
    STRING_SETTER(device->name, value);
}

/*!
 * \brief Get the device architecture
 *
 * \param device Device definition object
 * \return Device name or NULL if unset
 */
GETTER(const char *, architecture)
{
    STRING_GETTER(device->architecture)
}

/*!
 * \brief Set the device architecture
 *
 * \note This object is not changed if this function returns an error.
 *
 * \param device Device definition object
 * \param value Device architecture or NULL to clear field
 * \return Whether the field was successfully set
 */
SETTER(const char *, architecture)
{
    if (value) {
        if (strcmp(value, ARCH_ARMEABI_V7A) != 0
                && strcmp(value, ARCH_ARM64_V8A) != 0
                && strcmp(value, ARCH_X86) != 0
                && strcmp(value, ARCH_X86_64) != 0) {
            return MB_DEVICE_ERROR_INVALID_VALUE;
        }
    }

    STRING_SETTER(device->architecture, value)
}

GETTER(uint64_t, flags)
{
    return device->flags;
}

SETTER(uint64_t, flags)
{
    // TODO: Check flags
    device->flags = value;
    return MB_DEVICE_OK;
}

/*!
 * \brief Get the block device base directories
 *
 * \param device Device definition object
 * \return NULL-terminated list of block device base directories or NULL if unset
 */
GETTER(char const * const *, block_dev_base_dirs)
{
    STRING_ARRAY_GETTER(device->base_dirs)
}

/*!
 * \brief Set the block device base directories
 *
 * \note This object is not changed if this function returns an error.
 *
 * \param device Device definition object
 * \param value NULL-terminated list of block device base directories or NULL
 *              to clear field
 * \return Whether the field was successfully set
 */
SETTER(char const * const *, block_dev_base_dirs)
{
    STRING_ARRAY_SETTER(device->base_dirs, value)
}

/*!
 * \brief Get the system block device paths
 *
 * \param device Device definition object
 * \return NULL-terminated list of system block device paths or NULL if unset
 */
GETTER(char const * const *, system_block_devs)
{
    STRING_ARRAY_GETTER(device->system_devs)
}

/*!
 * \brief Set the system block device paths
 *
 * \note This object is not changed if this function returns an error.
 *
 * \param device Device definition object
 * \param value NULL-terminated list of system block device paths or NULL to
 *              clear field
 * \return Whether the field was successfully set
 */
SETTER(char const * const *, system_block_devs)
{
    STRING_ARRAY_SETTER(device->system_devs, value)
}

/*!
 * \brief Get the cache block device paths
 *
 * \param device Device definition object
 * \return NULL-terminated list of cache block device paths or NULL if unset
 */
GETTER(char const * const *, cache_block_devs)
{
    STRING_ARRAY_GETTER(device->cache_devs)
}

/*!
 * \brief Set the cache block device paths
 *
 * \note This object is not changed if this function returns an error.
 *
 * \param device Device definition object
 * \param value NULL-terminated list of cache block device paths or NULL to
 *              clear field
 * \return Whether the field was successfully set
 */
SETTER(char const * const *, cache_block_devs)
{
    STRING_ARRAY_SETTER(device->cache_devs, value)
}

/*!
 * \brief Get the data block device paths
 *
 * \param device Device definition object
 * \return NULL-terminated list of data block device paths or NULL if unset
 */
GETTER(char const * const *, data_block_devs)
{
    STRING_ARRAY_GETTER(device->data_devs)
}

/*!
 * \brief Set the data block device paths
 *
 * \note This object is not changed if this function returns an error.
 *
 * \param device Device definition object
 * \param value NULL-terminated list of data block device paths or NULL to
 *              clear field
 * \return Whether the field was successfully set
 */
SETTER(char const * const *, data_block_devs)
{
    STRING_ARRAY_SETTER(device->data_devs, value)
}

/*!
 * \brief Get the boot block device paths
 *
 * \param device Device definition object
 * \return NULL-terminated list of boot block device paths or NULL if unset
 */
GETTER(char const * const *, boot_block_devs)
{
    STRING_ARRAY_GETTER(device->boot_devs)
}

/*!
 * \brief Set the boot block device paths
 *
 * \note This object is not changed if this function returns an error.
 *
 * \param device Device definition object
 * \param value NULL-terminated list of boot block device paths or NULL to
 *              clear field
 * \return Whether the field was successfully set
 */
SETTER(char const * const *, boot_block_devs)
{
    STRING_ARRAY_SETTER(device->boot_devs, value)
}

/*!
 * \brief Get the recovery block device paths
 *
 * \param device Device definition object
 * \return NULL-terminated list of recovery block devices or NULL if unset
 */
GETTER(char const * const *, recovery_block_devs)
{
    STRING_ARRAY_GETTER(device->recovery_devs)
}

/*!
 * \brief Set the recovery block device paths
 *
 * \note This object is not changed if this function returns an error.
 *
 * \param device Device definition object
 * \param value NULL-terminated list of recovery block device paths or NULL to
 *              clear field
 * \return Whether the field was successfully set
 */
SETTER(char const * const *, recovery_block_devs)
{
    STRING_ARRAY_SETTER(device->recovery_devs, value)
}

/*!
 * \brief Get the extra block device paths
 *
 * \param device Device definition object
 * \return NULL-terminated list of extra block device paths or NULL if unset
 */
GETTER(char const * const *, extra_block_devs)
{
    STRING_ARRAY_GETTER(device->extra_devs)
}

/*!
 * \brief Set the extra block device paths
 *
 * \note This object is not changed if this function returns an error.
 *
 * \param device Device definition object
 * \param value NULL-terminated list of extra block device paths or NULL to
 *              clear field
 * \return Whether the field was successfully set
 */
SETTER(char const * const *, extra_block_devs)
{
    STRING_ARRAY_SETTER(device->extra_devs, value)
}

/* Boot UI */

GETTER(bool, tw_supported)
{
    return device->tw_options.supported;
}

SETTER(bool, tw_supported)
{
    device->tw_options.supported = value;
    return MB_DEVICE_OK;
}

GETTER(uint64_t, tw_flags)
{
    return device->tw_options.flags;
}

SETTER(uint64_t, tw_flags)
{
    // TODO: Check flags
    device->tw_options.flags = value;
    return MB_DEVICE_OK;
}

GETTER(enum TwPixelFormat, tw_pixel_format)
{
    return device->tw_options.pixel_format;
}

SETTER(enum TwPixelFormat, tw_pixel_format)
{
    // TODO: Check arguments
    device->tw_options.pixel_format = value;
    return MB_DEVICE_OK;
}

GETTER(enum TwForcePixelFormat, tw_force_pixel_format)
{
    return device->tw_options.force_pixel_format;
}

SETTER(enum TwForcePixelFormat, tw_force_pixel_format)
{
    // TODO: Check value
    device->tw_options.force_pixel_format = value;
    return MB_DEVICE_OK;
}

GETTER(int, tw_overscan_percent)
{
    return device->tw_options.overscan_percent;
}

SETTER(int, tw_overscan_percent)
{
    device->tw_options.overscan_percent = value;
    return MB_DEVICE_OK;
}

GETTER(int, tw_default_x_offset)
{
    return device->tw_options.default_x_offset;
}

SETTER(int, tw_default_x_offset)
{
    device->tw_options.default_x_offset = value;
    return MB_DEVICE_OK;
}

GETTER(int, tw_default_y_offset)
{
    return device->tw_options.default_y_offset;
}

SETTER(int, tw_default_y_offset)
{
    device->tw_options.default_y_offset = value;
    return MB_DEVICE_OK;
}

GETTER(const char *, tw_brightness_path)
{
    STRING_GETTER(device->tw_options.brightness_path)
}

SETTER(const char *, tw_brightness_path)
{
    STRING_SETTER(device->tw_options.brightness_path, value)
}

GETTER(const char *, tw_secondary_brightness_path)
{
    STRING_GETTER(device->tw_options.secondary_brightness_path);
}

SETTER(const char *, tw_secondary_brightness_path)
{
    STRING_SETTER(device->tw_options.secondary_brightness_path, value)
}

GETTER(int, tw_max_brightness)
{
    return device->tw_options.max_brightness;
}

SETTER(int, tw_max_brightness)
{
    device->tw_options.max_brightness = value;
    return MB_DEVICE_OK;
}

GETTER(int, tw_default_brightness)
{
    return device->tw_options.default_brightness;
}

SETTER(int, tw_default_brightness)
{
    device->tw_options.default_brightness = value;
    return MB_DEVICE_OK;
}

GETTER(const char *, tw_battery_path)
{
    STRING_GETTER(device->tw_options.battery_path)
}

SETTER(const char *, tw_battery_path)
{
    STRING_SETTER(device->tw_options.battery_path, value)
}

GETTER(const char *, tw_cpu_temp_path)
{
    STRING_GETTER(device->tw_options.cpu_temp_path)
}

SETTER(const char *, tw_cpu_temp_path)
{
    STRING_SETTER(device->tw_options.cpu_temp_path, value)
}

GETTER(const char *, tw_input_blacklist)
{
    STRING_GETTER(device->tw_options.input_blacklist)
}

SETTER(const char *, tw_input_blacklist)
{
    STRING_SETTER(device->tw_options.input_blacklist, value)
}

GETTER(const char *, tw_input_whitelist)
{
    STRING_GETTER(device->tw_options.input_whitelist)
}

SETTER(const char *, tw_input_whitelist)
{
    STRING_SETTER(device->tw_options.input_whitelist, value)
}

GETTER(char const * const *, tw_graphics_backends)
{
    STRING_ARRAY_GETTER(device->tw_options.graphics_backends)
}

SETTER(char const * const *, tw_graphics_backends)
{
    STRING_ARRAY_SETTER(device->tw_options.graphics_backends, value)
}

GETTER(const char *, tw_theme)
{
    STRING_GETTER(device->tw_options.theme)
}

SETTER(const char *, tw_theme)
{
    STRING_SETTER(device->tw_options.theme, value)
}

static inline bool array_equals(char * const *a, char * const *b)
{
    for (; *a && *b; ++a, ++b) {
        if (strcmp(*a, *b) != 0) {
            return false;
        }
    }

    return !*a && !*b;
}

#define BOTH_NULL_OR_NONNULL(A, B) \
    (((A) && (B)) || (!(A) && !(B)))
#define BOTH_NULL_OR_STRINGS_EQUAL(A, B) \
    (((A) && (B) && strcmp(A, B) == 0) || (!(A) && !(B)))
#define BOTH_NULL_OR_ARRAYS_EQUAL(A, B) \
    (((A) && (B) && array_equals(A, B)) || (!(A) && !(B)))

bool mb_device_equals(struct Device *a, struct Device *b)
{
    return BOTH_NULL_OR_NONNULL(a, b)
            && BOTH_NULL_OR_STRINGS_EQUAL(a->id, b->id)
            && BOTH_NULL_OR_ARRAYS_EQUAL(a->codenames, b->codenames)
            && BOTH_NULL_OR_STRINGS_EQUAL(a->name, b->name)
            && BOTH_NULL_OR_STRINGS_EQUAL(a->architecture, b->architecture)
            && a->flags == b->flags
            && BOTH_NULL_OR_ARRAYS_EQUAL(a->base_dirs, b->base_dirs)
            && BOTH_NULL_OR_ARRAYS_EQUAL(a->system_devs, b->system_devs)
            && BOTH_NULL_OR_ARRAYS_EQUAL(a->cache_devs, b->cache_devs)
            && BOTH_NULL_OR_ARRAYS_EQUAL(a->data_devs, b->data_devs)
            && BOTH_NULL_OR_ARRAYS_EQUAL(a->boot_devs, b->boot_devs)
            && BOTH_NULL_OR_ARRAYS_EQUAL(a->recovery_devs, b->recovery_devs)
            && BOTH_NULL_OR_ARRAYS_EQUAL(a->extra_devs, b->extra_devs)
            /* Boot UI */
            && a->tw_options.supported == b->tw_options.supported
            && a->tw_options.flags == b->tw_options.flags
            && a->tw_options.pixel_format == b->tw_options.pixel_format
            && a->tw_options.force_pixel_format == b->tw_options.force_pixel_format
            && a->tw_options.overscan_percent == b->tw_options.overscan_percent
            && a->tw_options.default_x_offset == b->tw_options.default_x_offset
            && a->tw_options.default_y_offset == b->tw_options.default_y_offset
            && BOTH_NULL_OR_STRINGS_EQUAL(a->tw_options.brightness_path,
                                          b->tw_options.brightness_path)
            && BOTH_NULL_OR_STRINGS_EQUAL(a->tw_options.secondary_brightness_path,
                                          b->tw_options.secondary_brightness_path)
            && a->tw_options.max_brightness == b->tw_options.max_brightness
            && a->tw_options.default_brightness == b->tw_options.default_brightness
            && BOTH_NULL_OR_STRINGS_EQUAL(a->tw_options.battery_path,
                                          b->tw_options.battery_path)
            && BOTH_NULL_OR_STRINGS_EQUAL(a->tw_options.cpu_temp_path,
                                          b->tw_options.cpu_temp_path)
            && BOTH_NULL_OR_STRINGS_EQUAL(a->tw_options.input_blacklist,
                                          b->tw_options.input_blacklist)
            && BOTH_NULL_OR_STRINGS_EQUAL(a->tw_options.input_whitelist,
                                          b->tw_options.input_whitelist)
            && BOTH_NULL_OR_ARRAYS_EQUAL(a->tw_options.graphics_backends,
                                         b->tw_options.graphics_backends)
            && BOTH_NULL_OR_STRINGS_EQUAL(a->tw_options.theme,
                                          b->tw_options.theme)
            ;
}
