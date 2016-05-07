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

#include "mbp/cwrapper/cdevice.h"

#include <cassert>

#include "mbp/cwrapper/private/util.h"

#include "mbp/device.h"


#define CAST(x) \
    assert(x != nullptr); \
    mbp::Device *d = reinterpret_cast<mbp::Device *>(x);
#define CCAST(x) \
    assert(x != nullptr); \
    const mbp::Device *d = reinterpret_cast<const mbp::Device *>(x);


/*!
 * \file cdevice.h
 * \brief C Wrapper for Device
 *
 * Please see the documentation for Device from the C++ API for more details.
 * The C functions directly correspond to the Device member functions.
 *
 * \sa Device
 */

extern "C" {

/*!
 * \brief Create a new Device object.
 *
 * \note The returned object must be freed with mbp_device_destroy().
 *
 * \return New CDevice
 */
CDevice * mbp_device_create(void)
{
    return reinterpret_cast<CDevice *>(new mbp::Device());
}

/*!
 * \brief Destroys a CDevice object.
 *
 * \param device CDevice to destroy
 */
void mbp_device_destroy(CDevice *device)
{
    CAST(device);
    delete d;
}

char * mbp_device_id(const CDevice *device)
{
    CCAST(device);
    return string_to_cstring(d->id());
}

void mbp_device_set_id(CDevice *device, const char *id)
{
    CAST(device);
    d->setId(id);
}

/*!
 * \brief Device's codenames
 *
 * \note The output data is dynamically allocated. It should be `free()`'d
 *       when it is no longer needed.
 *
 * \param device CDevice object
 *
 * \return Device codenames
 *
 * \sa Device::codenames()
 */
char ** mbp_device_codenames(const CDevice *device)
{
    CCAST(device);
    return vector_to_cstring_array(d->codenames());
}

/*!
 * \brief Set the device's codenames
 *
 * \param device CDevice object
 * \param names Codenames
 *
 * \sa Device::setCodenames()
 */
void mbp_device_set_codenames(CDevice *device, const char **names)
{
    CAST(device);
    d->setCodenames(cstring_array_to_vector(names));
}

/*!
 * \brief Device's full name
 *
 * \note The output data is dynamically allocated. It should be `free()`'d
 *       when it is no longer needed.
 *
 * \param device CDevice object
 *
 * \return Device name
 *
 * \sa Device::name()
 */
char *mbp_device_name(const CDevice *device)
{
    CCAST(device);
    return string_to_cstring(d->name());
}

/*!
 * \brief Set the device's name
 *
 * \param device CDevice object
 * \param name Name
 *
 * \sa Device::setName()
 */
void mbp_device_set_name(CDevice *device, const char *name)
{
    CAST(device);
    d->setName(name);
}

/*!
 * \brief Device's CPU architecture
 *
 * \note The output data is dynamically allocated. It should be `free()`'d
 *       when it is no longer needed.
 *
 * \param device CDevice object
 *
 * \return Architecture
 *
 * \sa Device::architecture()
 */
char *mbp_device_architecture(const CDevice *device)
{
    CCAST(device);
    return string_to_cstring(d->architecture());
}

/*!
 * \brief Set the device's CPU architecture
 *
 * \param device CDevice object
 * \param arch Architecture
 *
 * \sa Device::setArchitecture()
 */
void mbp_device_set_architecture(CDevice *device, const char *arch)
{
    CAST(device);
    d->setArchitecture(arch);
}

uint64_t mbp_device_flags(const CDevice *device)
{
    CCAST(device);
    return d->flags();
}

void mbp_device_set_flags(CDevice *device, uint64_t flags)
{
    CAST(device);
    d->setFlags(flags);
}

char * mbp_device_ramdisk_patcher(const CDevice *device)
{
    CCAST(device);
    return string_to_cstring(d->ramdiskPatcher());
}

void mbp_device_set_ramdisk_patcher(CDevice *device, const char *id)
{
    CAST(device);
    d->setRamdiskPatcher(id);
}

char ** mbp_device_block_dev_base_dirs(const CDevice *device)
{
    CCAST(device);
    return vector_to_cstring_array(d->blockDevBaseDirs());
}

void mbp_device_set_block_dev_base_dirs(CDevice *device, const char **dirs)
{
    CAST(device);
    d->setBlockDevBaseDirs(cstring_array_to_vector(dirs));
}

char ** mbp_device_system_block_devs(const CDevice *device)
{
    CCAST(device);
    return vector_to_cstring_array(d->systemBlockDevs());
}

void mbp_device_set_system_block_devs(CDevice *device, const char **block_devs)
{
    CAST(device);
    d->setSystemBlockDevs(cstring_array_to_vector(block_devs));
}

char ** mbp_device_cache_block_devs(const CDevice *device)
{
    CCAST(device);
    return vector_to_cstring_array(d->cacheBlockDevs());
}

void mbp_device_set_cache_block_devs(CDevice *device, const char **block_devs)
{
    CAST(device);
    d->setCacheBlockDevs(cstring_array_to_vector(block_devs));
}

char ** mbp_device_data_block_devs(const CDevice *device)
{
    CCAST(device);
    return vector_to_cstring_array(d->dataBlockDevs());
}

void mbp_device_set_data_block_devs(CDevice *device, const char **block_devs)
{
    CAST(device);
    d->setDataBlockDevs(cstring_array_to_vector(block_devs));
}

char ** mbp_device_boot_block_devs(const CDevice *device)
{
    CCAST(device);
    return vector_to_cstring_array(d->bootBlockDevs());
}

void mbp_device_set_boot_block_devs(CDevice *device, const char **block_devs)
{
    CAST(device);
    d->setBootBlockDevs(cstring_array_to_vector(block_devs));
}

char ** mbp_device_recovery_block_devs(const CDevice *device)
{
    CCAST(device);
    return vector_to_cstring_array(d->recoveryBlockDevs());
}

void mbp_device_set_recovery_block_devs(CDevice *device, const char **block_devs)
{
    CAST(device);
    d->setRecoveryBlockDevs(cstring_array_to_vector(block_devs));
}

char ** mbp_device_extra_block_devs(const CDevice *device)
{
    CCAST(device);
    return vector_to_cstring_array(d->extraBlockDevs());
}

void mbp_device_set_extra_block_devs(CDevice *device, const char **block_devs)
{
    CAST(device);
    d->setExtraBlockDevs(cstring_array_to_vector(block_devs));
}

}
