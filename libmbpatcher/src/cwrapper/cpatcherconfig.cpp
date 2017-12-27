/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbpatcher/cwrapper/cpatcherconfig.h"

#include <cassert>
#include <cstdlib>

#include "mbcommon/capi/util.h"

#include "mbpatcher/patcherconfig.h"


#define CAST(x) \
    assert(x != nullptr); \
    auto *config = reinterpret_cast<mb::patcher::PatcherConfig *>(x);
#define CCAST(x) \
    assert(x != nullptr); \
    auto const *config = reinterpret_cast<const mb::patcher::PatcherConfig *>(x);


/*!
 * \file cpatcherconfig.h
 * \brief C Wrapper for PatcherConfig
 *
 * Please see the documentation for PatcherConfig from the C++ API for more
 * details. The C functions directly correspond to the PatcherConfig member
 * functions.
 *
 * \sa PatcherConfig
 */

extern "C"
{

/*!
 * \brief Create a new CPatcherConfig object.
 *
 * \note The returned object must be freed with mbpatcher_config_destroy().
 *
 * \return New CPatcherConfig
 */
CPatcherConfig * mbpatcher_config_create(void)
{
    return reinterpret_cast<CPatcherConfig *>(new mb::patcher::PatcherConfig());
}

/*!
 * \brief Destroys a CPatcherConfig object.
 *
 * \param pc CPatcherConfig to destroy
 */
void mbpatcher_config_destroy(CPatcherConfig *pc)
{
    CAST(pc);
    delete config;
}

/*!
 * \brief Get the error information
 *
 * \note The returned ErrorCode is filled with valid data only if a
 *       CPatcherConfig operation has failed.
 *
 * \note The returned ErrorCode should be freed with mbpatcher_error_destroy()
 *       when it is no longer needed.
 *
 * \param pc CPatcherConfig object
 *
 * \return ErrorCode
 *
 * \sa PatcherConfig::error()
 */
/* enum ErrorCode */ int mbpatcher_config_error(const CPatcherConfig *pc)
{
    CCAST(pc);
    return static_cast<int>(config->error());
}

/*!
 * \brief Get top-level data directory
 *
 * \note The returned string is dynamically allocated. It should be free()'d
 *       when it is no longer needed.
 *
 * \param pc CPatcherConfig object
 * \return Data directory
 *
 * \sa PatcherConfig::dataDirectory()
 */
char * mbpatcher_config_data_directory(const CPatcherConfig *pc)
{
    CCAST(pc);
    return mb::capi_str_to_cstr(config->data_directory());
}

/*!
 * \brief Get the temporary directory
 *
 * \note The returned string is dynamically allocated. It should be free()'d
 *       when it is no longer needed.
 *
 * \param pc CPatcherConfig object
 * \return Temporary directory
 *
 * \sa PatcherConfig::tempDirectory()
 */
char * mbpatcher_config_temp_directory(const CPatcherConfig *pc)
{
    CCAST(pc);
    return mb::capi_str_to_cstr(config->temp_directory());
}

/*!
 * \brief Set top-level data directory
 *
 * \param pc CPatcherConfig object
 * \param path Path to data directory
 *
 * \sa PatcherConfig::setDataDirectory()
 */
void mbpatcher_config_set_data_directory(CPatcherConfig *pc, char *path)
{
    CAST(pc);
    config->set_data_directory(path);
}

/*!
 * \brief Set the temporary directory
 *
 * \param pc CPatcherConfig object
 * \param path Path to temporary directory
 *
 * \sa PatcherConfig::setTempDirectory()
 */
void mbpatcher_config_set_temp_directory(CPatcherConfig *pc, char *path)
{
    CAST(pc);
    config->set_temp_directory(path);
}

/*!
 * \brief Get list of Patcher IDs
 *
 * \note The returned array and its contents should be freed with `free()` when
 *       they are no longer needed.
 *
 * \param pc CPatcherConfig object
 * \return NULL-terminated array of Patcher IDs
 *
 * \sa PatcherConfig::patchers()
 */
char ** mbpatcher_config_patchers(const CPatcherConfig *pc)
{
    CCAST(pc);
    return mb::capi_vector_to_cstr_array(config->patchers());
}

/*!
 * \brief Get list of AutoPatcher IDs
 *
 * \note The returned array and its contents should be freed with `free()` when
 *       they are no longer needed.
 *
 * \param pc CPatcherConfig object
 * \return NULL-terminated array of AutoPatcher IDs
 *
 * \sa PatcherConfig::autoPatchers()
 */
char ** mbpatcher_config_autopatchers(const CPatcherConfig *pc)
{
    CCAST(pc);
    return mb::capi_vector_to_cstr_array(config->auto_patchers());
}

/*!
 * \brief Create new Patcher
 *
 * \param pc CPatcherConfig object
 * \param id Patcher ID
 * \return New Patcher
 *
 * \sa PatcherConfig::createPatcher()
 */
CPatcher * mbpatcher_config_create_patcher(CPatcherConfig *pc,
                                           const char *id)
{
    CAST(pc);
    auto *p = config->create_patcher(id);
    return reinterpret_cast<CPatcher *>(p);
}

/*!
 * \brief Create new AutoPatcher
 *
 * \param pc CPatcherConfig object
 * \param id AutoPatcher ID
 * \param info FileInfo describing file to be patched
 * \return New AutoPatcher
 *
 * \sa PatcherConfig::createAutoPatcher()
 */
CAutoPatcher * mbpatcher_config_create_autopatcher(CPatcherConfig *pc,
                                                   const char *id,
                                                   const CFileInfo *info)
{
    CAST(pc);
    auto const *fi = reinterpret_cast<const mb::patcher::FileInfo *>(info);
    auto *ap = config->create_auto_patcher(id, *fi);
    return reinterpret_cast<CAutoPatcher *>(ap);
}

/*!
 * \brief Destroys a Patcher and frees its memory
 *
 * \param pc CPatcherConfig object
 * \param patcher CPatcher to destroy
 *
 * \sa PatcherConfig::destroyPatcher()
 */
void mbpatcher_config_destroy_patcher(CPatcherConfig *pc, CPatcher *patcher)
{
    CAST(pc);
    auto *p = reinterpret_cast<mb::patcher::Patcher *>(patcher);
    config->destroy_patcher(p);
}

/*!
 * \brief Destroys an AutoPatcher and frees its memory
 *
 * \param pc CPatcherConfig object
 * \param patcher CAutoPatcher to destroy
 *
 * \sa PatcherConfig::destroyAutoPatcher()
 */
void mbpatcher_config_destroy_autopatcher(CPatcherConfig *pc,
                                          CAutoPatcher *patcher)
{
    CAST(pc);
    auto *ap = reinterpret_cast<mb::patcher::AutoPatcher *>(patcher);
    config->destroy_auto_patcher(ap);
}

}
