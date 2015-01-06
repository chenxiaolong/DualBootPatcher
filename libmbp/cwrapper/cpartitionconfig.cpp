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

#include "cwrapper/cpartitionconfig.h"

#include <cassert>

#include <cwrapper/private/util.h>

#include "partitionconfig.h"


#define CAST(x) \
    assert(x != nullptr); \
    PartitionConfig *pc = reinterpret_cast<PartitionConfig *>(x);
#define CCAST(x) \
    assert(x != nullptr); \
    const PartitionConfig *pc = reinterpret_cast<const PartitionConfig *>(x);


/*!
 * \file cpartitionconfig.h
 * \brief C Wrapper for PartitionConfig
 *
 * Please see the documentation for PartitionConfig from the C++ API for more
 * details. The C functions directly correspond to the PartitionConfig member
 * functions.
 *
 * \sa PartitionConfig
 */

extern "C" {

/*!
 * \brief Create a new CPartConfig object.
 *
 * \note The returned object must be freed with mbp_partconfig_destroy().
 *
 * \return New CPartConfig
 */
CPartConfig * mbp_partconfig_create(void)
{
    return reinterpret_cast<CPartConfig *>(new PartitionConfig());
}

/*!
 * \brief Destroys a CPartConfig object.
 *
 * \param config CPartConfig to destroy
 */
void mbp_partconfig_destroy(CPartConfig *config)
{
    CAST(config);
    delete pc;
}

/*!
 * \brief Partition configuration name
 *
 * \param config CPartConfig object
 *
 * \note The returned string is dynamically allocated. It should be free()'d
 *       when it is no longer needed.
 *
 * \return Name
 *
 * \sa PartitionConfig::name()
 */
char * mbp_partconfig_name(const CPartConfig *config)
{
    CCAST(config);
    return string_to_cstring(pc->name());
}

/*!
 * \brief Set partition configuration name
 *
 * \param config CPartConfig object
 * \param name Name
 *
 * \sa PartitionConfig::setName()
 */
void mbp_partconfig_set_name(CPartConfig *config, const char *name)
{
    CAST(config);
    pc->setName(name);
}

/*!
 * \brief Partition configuration description
 *
 * \param config CPartConfig object
 *
 * \note The returned string is dynamically allocated. It should be free()'d
 *       when it is no longer needed.
 *
 * \return Description
 *
 * \sa PartitionConfig::description()
 */
char * mbp_partconfig_description(const CPartConfig *config)
{
    CCAST(config);
    return string_to_cstring(pc->description());
}

/*!
 * \brief Set partition configuration description
 *
 * \param config CPartConfig object
 * \param description Description
 *
 * \sa PartitionConfig::setDescription()
 */
void mbp_partconfig_set_description(CPartConfig *config,
                                    const char *description)
{
    CAST(config);
    pc->setDescription(description);
}

/*!
 * \brief Kernel identifier
 *
 * \param config CPartConfig object
 *
 * \note The returned string is dynamically allocated. It should be free()'d
 *       when it is no longer needed.
 *
 * \return Kernel ID
 *
 * \sa PartitionConfig::kernel()
 */
char * mbp_partconfig_kernel(const CPartConfig *config)
{
    CCAST(config);
    return string_to_cstring(pc->kernel());
}

/*!
 * \brief Set kernel identifier
 *
 * \param config CPartConfig object
 * \param kernel Kernel ID
 *
 * \sa PartitionConfig::setKernel()
 */
void mbp_partconfig_set_kernel(CPartConfig *config, const char *kernel)
{
    CAST(config);
    pc->setKernel(kernel);
}

/*!
 * \brief Partition configuration ID
 *
 * \param config CPartConfig object
 *
 * \note The returned string is dynamically allocated. It should be free()'d
 *       when it is no longer needed.
 *
 * \return ID
 *
 * \sa PartitionConfig::id()
 */
char * mbp_partconfig_id(const CPartConfig *config)
{
    CCAST(config);
    return string_to_cstring(pc->id());
}

/*!
 * \brief Set partition configuration ID
 *
 * \param config CPartConfig object
 * \param id ID
 *
 * \sa PartitionConfig::setId()
 */
void mbp_partconfig_set_id(CPartConfig *config, const char *id)
{
    CAST(config);
    pc->setId(id);
}

/*!
 * \brief Source path for /system bind mount
 *
 * \param config CPartConfig object
 *
 * \note The returned string is dynamically allocated. It should be free()'d
 *       when it is no longer needed.
 *
 * \return Source path
 *
 * \sa PartitionConfig::targetSystem()
 */
char * mbp_partconfig_target_system(const CPartConfig *config)
{
    CCAST(config);
    return string_to_cstring(pc->targetSystem());
}

/*!
 * \brief Set source path for /system bind-mount
 *
 * \param config CPartConfig object
 * \param path Source path
 *
 * \sa PartitionConfig::setTargetSystem()
 */
void mbp_partconfig_set_target_system(CPartConfig *config, const char *path)
{
    CAST(config);
    pc->setTargetSystem(path);
}

/*!
 * \brief Source path for /cache bind mount
 *
 * \param config CPartConfig object
 *
 * \note The returned string is dynamically allocated. It should be free()'d
 *       when it is no longer needed.
 *
 * \return Source path
 *
 * \sa PartitionConfig::targetCache()
 */
char * mbp_partconfig_target_cache(const CPartConfig *config)
{
    CCAST(config);
    return string_to_cstring(pc->targetCache());
}

/*!
 * \brief Set source path for /cache bind-mount
 *
 * \param config CPartConfig object
 * \param path Source path
 *
 * \sa PartitionConfig::setTargetCache()
 */
void mbp_partconfig_set_target_cache(CPartConfig *config, const char *path)
{
    CAST(config);
    pc->setTargetCache(path);
}

/*!
 * \brief Source path for /data bind mount
 *
 * \param config CPartConfig object
 *
 * \note The returned string is dynamically allocated. It should be free()'d
 *       when it is no longer needed.
 *
 * \return Source path
 *
 * \sa PartitionConfig::targetData()
 */
char * mbp_partconfig_target_data(const CPartConfig *config)
{
    CCAST(config);
    return string_to_cstring(pc->targetData());
}

/*!
 * \brief Set source path for /data bind-mount
 *
 * \param config CPartConfig object
 * \param path Source path
 *
 * \sa PartitionConfig::setTargetData()
 */
void mbp_partconfig_set_target_data(CPartConfig *config, const char *path)
{
    CAST(config);
    pc->setTargetData(path);
}

}
