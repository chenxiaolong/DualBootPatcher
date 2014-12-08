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

#include "cwrapper/cdevice.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "device.h"


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

    // Static constants

    /*! \brief SELinux permissive mode constant */
    const char * mbp_device_selinux_permissive(void)
    {
        return Device::SelinuxPermissive.c_str();
    }

    /*! \brief Leave SELinux mode unchanged constant */
    const char * mbp_device_selinux_unchanged(void)
    {
        return Device::SelinuxUnchanged.c_str();
    }

    /*! \brief System partition constant */
    const char * mbp_device_system_partition(void)
    {
        return Device::SystemPartition.c_str();
    }

    /*! \brief Cache partition constant */
    const char * mbp_device_cache_partition(void)
    {
        return Device::CachePartition.c_str();
    }

    /*! \brief Data partition */
    const char * mbp_device_data_partition(void)
    {
        return Device::DataPartition.c_str();
    }


    /*!
     * \brief Create a new Device object.
     *
     * \note The returned object must be freed with mbp_device_destroy().
     *
     * \return New CDevice
     */
    CDevice * mbp_device_create(void)
    {
        return reinterpret_cast<CDevice *>(new Device());
    }

    /*!
     * \brief Destroys a CDevice object.
     *
     * \param device CDevice to destroy
     */
    void mbp_device_destroy(CDevice *device)
    {
        assert(device != nullptr);
        delete reinterpret_cast<Device *>(device);
    }

    /*!
     * \brief Device's codename
     *
     * \note The output data is dynamically allocated. It should be `free()`'d
     *       when it is no longer needed.
     *
     * \param device CDevice object
     *
     * \return Device codename
     *
     * \sa Device::codename()
     */
    char * mbp_device_codename(const CDevice *device)
    {
        assert(device != nullptr);
        const Device *d = reinterpret_cast<const Device *>(device);
        return strdup(d->codename().c_str());
    }

    /*!
     * \brief Set the device's codename
     *
     * \param device CDevice object
     * \param name Codename
     *
     * \sa Device::setCodename()
     */
    void mbp_device_set_codename(CDevice *device, const char *name)
    {
        assert(device != nullptr);
        Device *d = reinterpret_cast<Device *>(device);
        d->setCodename(name);
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
        assert(device != nullptr);
        const Device *d = reinterpret_cast<const Device *>(device);
        return strdup(d->name().c_str());
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
        assert(device != nullptr);
        Device *d = reinterpret_cast<Device *>(device);
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
        assert(device != nullptr);
        const Device *d = reinterpret_cast<const Device *>(device);
        return strdup(d->name().c_str());
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
        assert(device != nullptr);
        Device *d = reinterpret_cast<Device *>(device);
        d->setArchitecture(arch);
    }

    /*!
     * \brief Device's SELinux mode
     *
     * \note The output data is dynamically allocated. It should be `free()`'d
     *       when it is no longer needed.
     *
     * \param device CDevice object
     *
     * \return SELinux mode
     *
     * \sa Device::selinux()
     */
    char *mbp_device_selinux(const CDevice *device)
    {
        assert(device != nullptr);
        const Device *d = reinterpret_cast<const Device *>(device);
        return strdup(d->selinux().c_str());
    }

    /*!
     * \brief Set the device's SELinux mode
     *
     * \param device CDevice object
     * \param selinux SELinux mode
     *
     * \sa Device::setSelinux()
     */
    void mbp_device_set_selinux(CDevice *device, const char *selinux)
    {
        assert(device != nullptr);
        Device *d = reinterpret_cast<Device *>(device);
        d->setSelinux(selinux);
    }

    /*!
     * \brief Get partition number for a specific partition
     *
     * \note The output data is dynamically allocated. It should be `free()`'d
     *       when it is no longer needed.
     *
     * \param device CDevice object
     * \param which Partition
     *
     * \return Partition number
     *
     * \sa Device::partition()
     */
    char *mbp_device_partition(const CDevice *device, const char *which)
    {
        assert(device != nullptr);
        const Device *d = reinterpret_cast<const Device *>(device);
        return strdup(d->partition(which).c_str());
    }

    /*!
     * \brief Set the partition number for a specific partition
     *
     * \param device CDevice object
     * \param which Partition
     * \param partition Partition number
     *
     * \sa Device::setPartition()
     */
    void mbp_device_set_partition(CDevice *device,
                                  const char *which, const char *partition)
    {
        assert(device != nullptr);
        Device *d = reinterpret_cast<Device *>(device);
        d->setPartition(which, partition);
    }

    /*!
     * \brief List of partition types with partition numbers assigned
     *
     * \note The returned array should be freed with `mbp_free_array()` when it
     *       is no longer needed.
     *
     * \return A NULL-terminated array containing the partition types
     *
     * \sa Device::partitionTypes()
     */
    char **mbp_device_partition_types(const CDevice *device)
    {
        assert(device != nullptr);
        const Device *d = reinterpret_cast<const Device *>(device);
        auto const types = d->partitionTypes();

        char **cTypes = (char **) std::malloc(
                sizeof(char *) * (types.size() + 1));
        for (unsigned int i = 0; i < types.size(); ++i) {
            cTypes[i] = strdup(types[i].c_str());
        }
        cTypes[types.size()] = nullptr;

        return cTypes;
    }

}
