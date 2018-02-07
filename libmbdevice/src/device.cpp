/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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


namespace mb::device
{

Device::Device()
    : m_base()
    , m_tw()
{
    m_tw.pixel_format = TwPixelFormat::Default;
    m_tw.force_pixel_format = TwForcePixelFormat::None;
    m_tw.max_brightness = -1;
    m_tw.default_brightness = -1;
}

Device::~Device() = default;

/*!
 * \brief Get the device ID
 *
 * \return Device ID
 */
std::string Device::id() const
{
    return m_base.id;
}

/*!
 * \brief Set the device ID
 *
 * \param id Device ID
 */
void Device::set_id(std::string id)
{
    m_base.id = std::move(id);
}

/*!
 * \brief Get the device codenames
 *
 * \return List of device names
 */
std::vector<std::string> Device::codenames() const
{
    return m_base.codenames;
}

/*!
 * \brief Set the device codenames
 *
 * \param codenames List of device codenames
 */
void Device::set_codenames(std::vector<std::string> codenames)
{
    m_base.codenames = std::move(codenames);
}

/*!
 * \brief Get the device name
 *
 * \return Device name
 */
std::string Device::name() const
{
    return m_base.name;
}

/*!
 * \brief Set the device name
 *
 * \param name Device name
 */
void Device::set_name(std::string name)
{
    m_base.name = std::move(name);
}

/*!
 * \brief Get the device architecture
 *
 * \return Device architecture
 */
std::string Device::architecture() const
{
    return m_base.architecture;
}

/*!
 * \brief Set the device architecture
 *
 * \param architecture Device architecture
 */
void Device::set_architecture(std::string architecture)
{
    m_base.architecture = std::move(architecture);
}

DeviceFlags Device::flags() const
{
    return m_base.flags;
}

void Device::set_flags(DeviceFlags flags)
{
    m_base.flags = flags;
}

/*!
 * \brief Get the block device base directories
 *
 * \return List of block device base directories
 */
std::vector<std::string> Device::block_dev_base_dirs() const
{
    return m_base.base_dirs;
}

/*!
 * \brief Set the block device base directories
 *
 * \param base_dirs List of block device base directories
 */
void Device::set_block_dev_base_dirs(std::vector<std::string> base_dirs)
{
    m_base.base_dirs = std::move(base_dirs);
}

/*!
 * \brief Get the system block device paths
 *
 * \return List of system block device paths
 */
std::vector<std::string> Device::system_block_devs() const
{
    return m_base.system_devs;
}

/*!
 * \brief Set the system block device paths
 *
 * \param block_devs List of system block device paths
 */
void Device::set_system_block_devs(std::vector<std::string> block_devs)
{
    m_base.system_devs = std::move(block_devs);
}

/*!
 * \brief Get the cache block device paths
 *
 * \return List of cache block device paths
 */
std::vector<std::string> Device::cache_block_devs() const
{
    return m_base.cache_devs;
}

/*!
 * \brief Set the cache block device paths
 *
 * \param block_devs List of cache block device paths
 */
void Device::set_cache_block_devs(std::vector<std::string> block_devs)
{
    m_base.cache_devs = std::move(block_devs);
}

/*!
 * \brief Get the data block device paths
 *
 * \return List of data block device paths
 */
std::vector<std::string> Device::data_block_devs() const
{
    return m_base.data_devs;
}

/*!
 * \brief Set the data block device paths
 *
 * \param block_devs List of data block device paths
 */
void Device::set_data_block_devs(std::vector<std::string> block_devs)
{
    m_base.data_devs = std::move(block_devs);
}

/*!
 * \brief Get the boot block device paths
 *
 * \return List of boot block device paths
 */
std::vector<std::string> Device::boot_block_devs() const
{
    return m_base.boot_devs;
}

/*!
 * \brief Set the boot block device paths
 *
 * \param block_devs List of boot block device paths
 */
void Device::set_boot_block_devs(std::vector<std::string> block_devs)
{
    m_base.boot_devs = std::move(block_devs);
}

/*!
 * \brief Get the recovery block device paths
 *
 * \return List of recovery block devices
 */
std::vector<std::string> Device::recovery_block_devs() const
{
    return m_base.recovery_devs;
}

/*!
 * \brief Set the recovery block device paths
 *
 * \param block_devs List of recovery block device paths
 */
void Device::set_recovery_block_devs(std::vector<std::string> block_devs)
{
    m_base.recovery_devs = std::move(block_devs);
}

/*!
 * \brief Get the extra block device paths
 *
 * \return List of extra block device paths
 */
std::vector<std::string> Device::extra_block_devs() const
{
    return m_base.extra_devs;
}

/*!
 * \brief Set the extra block device paths
 *
 * \param block_devs List of extra block device paths
 */
void Device::set_extra_block_devs(std::vector<std::string> block_devs)
{
    m_base.extra_devs = std::move(block_devs);
}

bool Device::tw_supported() const
{
    return m_tw.supported;
}

void Device::set_tw_supported(bool supported)
{
    m_tw.supported = supported;
}

TwFlags Device::tw_flags() const
{
    return m_tw.flags;
}

void Device::set_tw_flags(TwFlags flags)
{
    m_tw.flags = flags;
}

TwPixelFormat Device::tw_pixel_format() const
{
    return m_tw.pixel_format;
}

void Device::set_tw_pixel_format(TwPixelFormat format)
{
    m_tw.pixel_format = format;
}

TwForcePixelFormat Device::tw_force_pixel_format() const
{
    return m_tw.force_pixel_format;
}

void Device::set_tw_force_pixel_format(TwForcePixelFormat format)
{
    m_tw.force_pixel_format = format;
}

int Device::tw_overscan_percent() const
{
    return m_tw.overscan_percent;
}

void Device::set_tw_overscan_percent(int percent)
{
    m_tw.overscan_percent = percent;
}

int Device::tw_default_x_offset() const
{
    return m_tw.default_x_offset;
}

void Device::set_tw_default_x_offset(int offset)
{
    m_tw.default_x_offset = offset;
}

int Device::tw_default_y_offset() const
{
    return m_tw.default_y_offset;
}

void Device::set_tw_default_y_offset(int offset)
{
    m_tw.default_y_offset = offset;
}

std::string Device::tw_brightness_path() const
{
    return m_tw.brightness_path;
}

void Device::set_tw_brightness_path(std::string path)
{
    m_tw.brightness_path = std::move(path);
}

std::string Device::tw_secondary_brightness_path() const
{
    return m_tw.secondary_brightness_path;
}

void Device::set_tw_secondary_brightness_path(std::string path)
{
    m_tw.secondary_brightness_path = std::move(path);
}

int Device::tw_max_brightness() const
{
    return m_tw.max_brightness;
}

void Device::set_tw_max_brightness(int value)
{
    m_tw.max_brightness = value;
}

int Device::tw_default_brightness() const
{
    return m_tw.default_brightness;
}

void Device::set_tw_default_brightness(int value)
{
    m_tw.default_brightness = value;
}

std::string Device::tw_battery_path() const
{
    return m_tw.battery_path;
}

void Device::set_tw_battery_path(std::string path)
{
    m_tw.battery_path = std::move(path);
}

std::string Device::tw_cpu_temp_path() const
{
    return m_tw.cpu_temp_path;
}

void Device::set_tw_cpu_temp_path(std::string path)
{
    m_tw.cpu_temp_path = std::move(path);
}

std::string Device::tw_input_blacklist() const
{
    return m_tw.input_blacklist;
}

void Device::set_tw_input_blacklist(std::string blacklist)
{
    m_tw.input_blacklist = std::move(blacklist);
}

std::string Device::tw_input_whitelist() const
{
    return m_tw.input_whitelist;
}

void Device::set_tw_input_whitelist(std::string whitelist)
{
    m_tw.input_whitelist = std::move(whitelist);
}

std::vector<std::string> Device::tw_graphics_backends() const
{
    return m_tw.graphics_backends;
}

void Device::set_tw_graphics_backends(std::vector<std::string> backends)
{
    m_tw.graphics_backends = std::move(backends);
}

std::string Device::tw_theme() const
{
    return m_tw.theme;
}

void Device::set_tw_theme(std::string theme)
{
    m_tw.theme = std::move(theme);
}


ValidateFlags Device::validate() const
{
    ValidateFlags flags = 0;

    if (m_base.id.empty()) {
        flags |= ValidateFlag::MissingId;
    }

    if (m_base.codenames.empty()) {
        flags |= ValidateFlag::MissingCodenames;
    }

    if (m_base.name.empty()) {
        flags |= ValidateFlag::MissingName;
    }

    if (m_base.architecture.empty()) {
        flags |= ValidateFlag::MissingArchitecture;
    } else if (m_base.architecture != ARCH_ARMEABI_V7A
            && m_base.architecture != ARCH_ARM64_V8A
            && m_base.architecture != ARCH_X86
            && m_base.architecture != ARCH_X86_64) {
        flags |= ValidateFlag::InvalidArchitecture;
    }

    if ((m_base.flags & DEVICE_FLAG_MASK) != m_base.flags) {
        flags |= ValidateFlag::InvalidFlags;
    }

    if (m_base.system_devs.empty()) {
        flags |= ValidateFlag::MissingSystemBlockDevs;
    }

    if (m_base.cache_devs.empty()) {
        flags |= ValidateFlag::MissingCacheBlockDevs;
    }

    if (m_base.data_devs.empty()) {
        flags |= ValidateFlag::MissingDataBlockDevs;
    }

    if (m_base.boot_devs.empty()) {
        flags |= ValidateFlag::MissingBootBlockDevs;
    }

    //if (m_base.recovery_devs.empty()) {
    //    flags |= ValidateFlag::MissingRecoveryBlockDevs;
    //}

    if (m_tw.supported) {
        if ((m_tw.flags & TW_FLAG_MASK) != m_tw.flags) {
            flags |= ValidateFlag::InvalidBootUiFlags;
        }

        if (m_tw.theme.empty()) {
            flags |= ValidateFlag::MissingBootUiTheme;
        }

        if (m_tw.graphics_backends.empty()) {
            flags |= ValidateFlag::MissingBootUiGraphicsBackends;
        }
    }

    return flags;
}

namespace detail
{

bool BaseOptions::operator==(const BaseOptions &other) const
{
    return id == other.id
            && codenames == other.codenames
            && name == other.name
            && architecture == other.architecture
            && flags == other.flags
            && base_dirs == other.base_dirs
            && system_devs == other.system_devs
            && cache_devs == other.cache_devs
            && data_devs == other.data_devs
            && boot_devs == other.boot_devs
            && recovery_devs == other.recovery_devs
            && extra_devs == other.extra_devs;
}

bool TwOptions::operator==(const TwOptions &other) const
{
    return supported == other.supported
            && flags == other.flags
            && pixel_format == other.pixel_format
            && force_pixel_format == other.force_pixel_format
            && overscan_percent == other.overscan_percent
            && default_x_offset == other.default_x_offset
            && default_y_offset == other.default_y_offset
            && brightness_path == other.brightness_path
            && secondary_brightness_path == other.secondary_brightness_path
            && max_brightness == other.max_brightness
            && default_brightness == other.default_brightness
            && battery_path == other.battery_path
            && cpu_temp_path == other.cpu_temp_path
            && input_blacklist == other.input_blacklist
            && input_whitelist == other.input_whitelist
            && graphics_backends == other.graphics_backends
            && theme == other.theme;
}

}

bool Device::operator==(const Device &other) const
{
    return m_base == other.m_base && m_tw == other.m_tw;
}

}
