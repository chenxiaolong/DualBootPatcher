/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#include <stdint.h>

#include "mbcommon/common.h"
#include "mbp/cwrapper/ctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

MB_EXPORT CDevice * mbp_device_create(void);
MB_EXPORT void mbp_device_destroy(CDevice *device);

MB_EXPORT char * mbp_device_id(const CDevice *device);
MB_EXPORT void mbp_device_set_id(CDevice *device, const char *id);

MB_EXPORT char ** mbp_device_codenames(const CDevice *device);
MB_EXPORT void mbp_device_set_codenames(CDevice *device, const char **names);

MB_EXPORT char * mbp_device_name(const CDevice *device);
MB_EXPORT void mbp_device_set_name(CDevice *device, const char *name);

MB_EXPORT char * mbp_device_architecture(const CDevice *device);
MB_EXPORT void mbp_device_set_architecture(CDevice *device, const char *arch);

MB_EXPORT uint64_t mbp_device_flags(const CDevice *device);
MB_EXPORT void mbp_device_set_flags(CDevice *device, uint64_t flags);

MB_EXPORT char * mbp_device_ramdisk_patcher(const CDevice *device);
MB_EXPORT void mbp_device_set_ramdisk_patcher(CDevice *device, const char *id);

MB_EXPORT char ** mbp_device_block_dev_base_dirs(const CDevice *device);
MB_EXPORT void mbp_device_set_block_dev_base_dirs(CDevice *device, const char **dirs);
MB_EXPORT char ** mbp_device_system_block_devs(const CDevice *device);
MB_EXPORT void mbp_device_set_system_block_devs(CDevice *device, const char **block_devs);
MB_EXPORT char ** mbp_device_cache_block_devs(const CDevice *device);
MB_EXPORT void mbp_device_set_cache_block_devs(CDevice *device, const char **block_devs);
MB_EXPORT char ** mbp_device_data_block_devs(const CDevice *device);
MB_EXPORT void mbp_device_set_data_block_devs(CDevice *device, const char **block_devs);
MB_EXPORT char ** mbp_device_boot_block_devs(const CDevice *device);
MB_EXPORT void mbp_device_set_boot_block_devs(CDevice *device, const char **block_devs);
MB_EXPORT char ** mbp_device_recovery_block_devs(const CDevice *device);
MB_EXPORT void mbp_device_set_recovery_block_devs(CDevice *device, const char **block_devs);
MB_EXPORT char ** mbp_device_extra_block_devs(const CDevice *device);
MB_EXPORT void mbp_device_set_extra_block_devs(CDevice *device, const char **block_devs);

#ifdef __cplusplus
}
#endif
