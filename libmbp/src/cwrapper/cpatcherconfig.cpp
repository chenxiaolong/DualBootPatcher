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

#include "mbp/cwrapper/cpatcherconfig.h"

#include <cassert>
#include <cstdlib>

#include "mbp/cwrapper/private/util.h"

#include "mbp/patcherconfig.h"


#define CAST(x) \
    assert(x != nullptr); \
    mbp::PatcherConfig *config = reinterpret_cast<mbp::PatcherConfig *>(x);
#define CCAST(x) \
    assert(x != nullptr); \
    const mbp::PatcherConfig *config = reinterpret_cast<const mbp::PatcherConfig *>(x);


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

extern "C" {

/*!
 * \brief Create a new CPatcherConfig object.
 *
 * \note The returned object must be freed with mbp_config_destroy().
 *
 * \return New CPatcherConfig
 */
CPatcherConfig * mbp_config_create(void)
{
    return reinterpret_cast<CPatcherConfig *>(new mbp::PatcherConfig());
}

/*!
 * \brief Destroys a CPatcherConfig object.
 *
 * \param config CPatcherConfig to destroy
 */
void mbp_config_destroy(CPatcherConfig *pc)
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
 * \note The returned ErrorCode should be freed with mbp_error_destroy()
 *       when it is no longer needed.
 *
 * \param config CPatcherConfig object
 *
 * \return ErrorCode
 *
 * \sa PatcherConfig::error()
 */
/* enum ErrorCode */ int mbp_config_error(const CPatcherConfig *pc)
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
char * mbp_config_data_directory(const CPatcherConfig *pc)
{
    CCAST(pc);
    return string_to_cstring(config->dataDirectory());
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
char * mbp_config_temp_directory(const CPatcherConfig *pc)
{
    CCAST(pc);
    return string_to_cstring(config->tempDirectory());
}

/*!
 * \brief Set top-level data directory
 *
 * \param pc CPatcherConfig object
 * \param path Path to data directory
 *
 * \sa PatcherConfig::setDataDirectory()
 */
void mbp_config_set_data_directory(CPatcherConfig *pc, char *path)
{
    CAST(pc);
    config->setDataDirectory(path);
}

/*!
 * \brief Set the temporary directory
 *
 * \param pc CPatcherConfig object
 * \param path Path to temporary directory
 *
 * \sa PatcherConfig::setTempDirectory()
 */
void mbp_config_set_temp_directory(CPatcherConfig *pc, char *path)
{
    CAST(pc);
    config->setTempDirectory(path);
}

/*!
 * \brief Get list of Patcher IDs
 *
 * \note The returned array should be freed with `mbp_free_array()` when it
 *       is no longer needed.
 *
 * \param pc CPatcherConfig object
 * \return NULL-terminated array of Patcher IDs
 *
 * \sa PatcherConfig::patchers()
 */
char ** mbp_config_patchers(const CPatcherConfig *pc)
{
    CCAST(pc);
    return vector_to_cstring_array(config->patchers());
}

/*!
 * \brief Get list of AutoPatcher IDs
 *
 * \note The returned array should be freed with `mbp_free_array()` when it
 *       is no longer needed.
 *
 * \param pc CPatcherConfig object
 * \return NULL-terminated array of AutoPatcher IDs
 *
 * \sa PatcherConfig::autoPatchers()
 */
char ** mbp_config_autopatchers(const CPatcherConfig *pc)
{
    CCAST(pc);
    return vector_to_cstring_array(config->autoPatchers());
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
CPatcher * mbp_config_create_patcher(CPatcherConfig *pc,
                                     const char *id)
{
    CAST(pc);
    mbp::Patcher *p = config->createPatcher(id);
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
CAutoPatcher * mbp_config_create_autopatcher(CPatcherConfig *pc,
                                             const char *id,
                                             const CFileInfo *info)
{
    CAST(pc);
    const mbp::FileInfo *fi = reinterpret_cast<const mbp::FileInfo *>(info);
    mbp::AutoPatcher *ap = config->createAutoPatcher(id, fi);
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
void mbp_config_destroy_patcher(CPatcherConfig *pc, CPatcher *patcher)
{
    CAST(pc);
    mbp::Patcher *p = reinterpret_cast<mbp::Patcher *>(patcher);
    config->destroyPatcher(p);
}

/*!
 * \brief Destroys an AutoPatcher and frees its memory
 *
 * \param pc CPatcherConfig object
 * \param patcher CAutoPatcher to destroy
 *
 * \sa PatcherConfig::destroyAutoPatcher()
 */
void mbp_config_destroy_autopatcher(CPatcherConfig *pc,
                                    CAutoPatcher *patcher)
{
    CAST(pc);
    mbp::AutoPatcher *ap = reinterpret_cast<mbp::AutoPatcher *>(patcher);
    config->destroyAutoPatcher(ap);
}

}
