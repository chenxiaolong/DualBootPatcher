/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbdevice/capi/device.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "mbcommon/capi/util.h"
#include "mbdevice/device.h"

#define GETTER(TYPE, NAME) \
    TYPE mb_device_ ## NAME (const CDevice *device)
#define SETTER(TYPE, NAME) \
    void mb_device_set_ ## NAME (CDevice *device, TYPE value)

#define CAST(x) \
    assert(x != nullptr); \
    auto *d = reinterpret_cast<Device *>(x);
#define CCAST(x) \
    assert(x != nullptr); \
    auto const *d = reinterpret_cast<const Device *>(x);

using namespace mb;
using namespace mb::device;

MB_BEGIN_C_DECLS

CDevice * mb_device_new()
{
    return reinterpret_cast<CDevice *>(new Device());
}

void mb_device_free(CDevice *device)
{
    delete reinterpret_cast<Device *>(device);
}

GETTER(char *, id)
{
    CCAST(device);
    return capi_str_to_cstr(d->id());
}

SETTER(const char *, id)
{
    CAST(device);
    d->set_id(capi_cstr_to_str(value));
}

GETTER(char * const *, codenames)
{
    CCAST(device);
    return capi_vector_to_cstr_array(d->codenames());
}

SETTER(char const * const *, codenames)
{
    CAST(device);
    d->set_codenames(capi_cstr_array_to_vector(value));
}

GETTER(char *, name)
{
    CCAST(device);
    return capi_str_to_cstr(d->name());
}

SETTER(const char *, name)
{
    CAST(device);
    d->set_name(capi_cstr_to_str(value));
}

GETTER(char *, architecture)
{
    CCAST(device);
    return capi_str_to_cstr(d->architecture());
}

SETTER(const char *, architecture)
{
    CAST(device);
    d->set_architecture(capi_cstr_to_str(value));
}

GETTER(uint32_t, flags)
{
    CCAST(device);
    return d->flags();
}

SETTER(uint32_t, flags)
{
    CAST(device);
    d->set_flags(static_cast<DeviceFlag>(value));
}

GETTER(char * const *, block_dev_base_dirs)
{
    CCAST(device);
    return capi_vector_to_cstr_array(d->block_dev_base_dirs());
}

SETTER(char const * const *, block_dev_base_dirs)
{
    CAST(device);
    d->set_block_dev_base_dirs(capi_cstr_array_to_vector(value));
}

GETTER(char * const *, system_block_devs)
{
    CCAST(device);
    return capi_vector_to_cstr_array(d->system_block_devs());
}

SETTER(char const * const *, system_block_devs)
{
    CAST(device);
    d->set_system_block_devs(capi_cstr_array_to_vector(value));
}

GETTER(char * const *, cache_block_devs)
{
    CCAST(device);
    return capi_vector_to_cstr_array(d->cache_block_devs());
}

SETTER(char const * const *, cache_block_devs)
{
    CAST(device);
    d->set_cache_block_devs(capi_cstr_array_to_vector(value));
}

GETTER(char * const *, data_block_devs)
{
    CCAST(device);
    return capi_vector_to_cstr_array(d->data_block_devs());
}

SETTER(char const * const *, data_block_devs)
{
    CAST(device);
    d->set_data_block_devs(capi_cstr_array_to_vector(value));
}

GETTER(char * const *, boot_block_devs)
{
    CCAST(device);
    return capi_vector_to_cstr_array(d->boot_block_devs());
}

SETTER(char const * const *, boot_block_devs)
{
    CAST(device);
    d->set_boot_block_devs(capi_cstr_array_to_vector(value));
}

GETTER(char * const *, recovery_block_devs)
{
    CCAST(device);
    return capi_vector_to_cstr_array(d->recovery_block_devs());
}

SETTER(char const * const *, recovery_block_devs)
{
    CAST(device);
    d->set_recovery_block_devs(capi_cstr_array_to_vector(value));
}

GETTER(char * const *, extra_block_devs)
{
    CCAST(device);
    return capi_vector_to_cstr_array(d->extra_block_devs());
}

SETTER(char const * const *, extra_block_devs)
{
    CAST(device);
    d->set_extra_block_devs(capi_cstr_array_to_vector(value));
}

/* Boot UI */

GETTER(bool, tw_supported)
{
    CCAST(device);
    return d->tw_supported();
}

SETTER(bool, tw_supported)
{
    CAST(device);
    d->set_tw_supported(value);
}

GETTER(uint32_t, tw_flags)
{
    CCAST(device);
    return d->tw_flags();
}

SETTER(uint32_t, tw_flags)
{
    CAST(device);
    d->set_tw_flags(static_cast<TwFlag>(value));
}

GETTER(uint16_t, tw_pixel_format)
{
    CCAST(device);
    return static_cast<std::underlying_type_t<TwPixelFormat>>(
            d->tw_pixel_format());
}

SETTER(uint16_t, tw_pixel_format)
{
    CAST(device);
    d->set_tw_pixel_format(static_cast<TwPixelFormat>(value));
}

GETTER(uint16_t, tw_force_pixel_format)
{
    CCAST(device);
    return static_cast<std::underlying_type_t<TwForcePixelFormat>>(
            d->tw_force_pixel_format());
}

SETTER(uint16_t, tw_force_pixel_format)
{
    CAST(device);
    d->set_tw_force_pixel_format(static_cast<TwForcePixelFormat>(value));
}

GETTER(int, tw_overscan_percent)
{
    CCAST(device);
    return d->tw_overscan_percent();
}

SETTER(int, tw_overscan_percent)
{
    CAST(device);
    d->set_tw_overscan_percent(value);
}

GETTER(int, tw_default_x_offset)
{
    CCAST(device);
    return d->tw_default_x_offset();
}

SETTER(int, tw_default_x_offset)
{
    CAST(device);
    d->set_tw_default_x_offset(value);
}

GETTER(int, tw_default_y_offset)
{
    CCAST(device);
    return d->tw_default_y_offset();
}

SETTER(int, tw_default_y_offset)
{
    CAST(device);
    d->set_tw_default_y_offset(value);
}

GETTER(char *, tw_brightness_path)
{
    CCAST(device);
    return capi_str_to_cstr(d->tw_brightness_path());
}

SETTER(const char *, tw_brightness_path)
{
    CAST(device);
    d->set_tw_brightness_path(capi_cstr_to_str(value));
}

GETTER(char *, tw_secondary_brightness_path)
{
    CCAST(device);
    return capi_str_to_cstr(d->tw_secondary_brightness_path());
}

SETTER(const char *, tw_secondary_brightness_path)
{
    CAST(device);
    d->set_tw_secondary_brightness_path(capi_cstr_to_str(value));
}

GETTER(int, tw_max_brightness)
{
    CCAST(device);
    return d->tw_max_brightness();
}

SETTER(int, tw_max_brightness)
{
    CAST(device);
    d->set_tw_max_brightness(value);
}

GETTER(int, tw_default_brightness)
{
    CCAST(device);
    return d->tw_default_brightness();
}

SETTER(int, tw_default_brightness)
{
    CAST(device);
    d->set_tw_default_brightness(value);
}

GETTER(char *, tw_battery_path)
{
    CCAST(device);
    return capi_str_to_cstr(d->tw_battery_path());
}

SETTER(const char *, tw_battery_path)
{
    CAST(device);
    d->set_tw_battery_path(capi_cstr_to_str(value));
}

GETTER(char *, tw_cpu_temp_path)
{
    CCAST(device);
    return capi_str_to_cstr(d->tw_cpu_temp_path());
}

SETTER(const char *, tw_cpu_temp_path)
{
    CAST(device);
    d->set_tw_cpu_temp_path(capi_cstr_to_str(value));
}

GETTER(char *, tw_input_blacklist)
{
    CCAST(device);
    return capi_str_to_cstr(d->tw_input_blacklist());
}

SETTER(const char *, tw_input_blacklist)
{
    CAST(device);
    d->set_tw_input_blacklist(capi_cstr_to_str(value));
}

GETTER(char *, tw_input_whitelist)
{
    CCAST(device);
    return capi_str_to_cstr(d->tw_input_whitelist());
}

SETTER(const char *, tw_input_whitelist)
{
    CAST(device);
    d->set_tw_input_whitelist(capi_cstr_to_str(value));
}

GETTER(char * const *, tw_graphics_backends)
{
    CCAST(device);
    return capi_vector_to_cstr_array(d->tw_graphics_backends());
}

SETTER(char const * const *, tw_graphics_backends)
{
    CAST(device);
    d->set_tw_graphics_backends(capi_cstr_array_to_vector(value));
}

GETTER(char *, tw_theme)
{
    CCAST(device);
    return capi_str_to_cstr(d->tw_theme());
}

SETTER(const char *, tw_theme)
{
    CAST(device);
    d->set_tw_theme(capi_cstr_to_str(value));
}

uint32_t mb_device_validate(const CDevice *device)
{
    CCAST(device);
    return d->validate();
}

bool mb_device_equals(const CDevice *a, const CDevice *b)
{
    auto const *device_a = reinterpret_cast<const Device *>(a);
    auto const *device_b = reinterpret_cast<const Device *>(b);

    return *device_a == *device_b;
}

MB_END_C_DECLS
