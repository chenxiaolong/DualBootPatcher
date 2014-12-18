/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#include "cwrapper/ctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

const char * mbp_device_system_partition(void);
const char * mbp_device_cache_partition(void);
const char * mbp_device_data_partition(void);

CDevice * mbp_device_create(void);
void mbp_device_destroy(CDevice *device);

char * mbp_device_codename(const CDevice *device);
void mbp_device_set_codename(CDevice *device, const char *name);

char * mbp_device_name(const CDevice *device);
void mbp_device_set_name(CDevice *device, const char *name);

char * mbp_device_architecture(const CDevice *device);
void mbp_device_set_architecture(CDevice *device, const char *arch);

char * mbp_device_partition(const CDevice *device, const char *which);
void mbp_device_set_partition(CDevice *device,
                              const char *which, const char *partition);
char ** mbp_device_partition_types(const CDevice *device);

#ifdef __cplusplus
}
#endif
