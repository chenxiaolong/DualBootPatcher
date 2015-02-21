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

#include "cwrapper/cpatcherconfig.h"

#include <cassert>
#include <cstdlib>

#include "cwrapper/private/util.h"

#include "patcherconfig.h"


#define CAST(x) \
    assert(x != nullptr); \
    PatcherConfig *config = reinterpret_cast<PatcherConfig *>(x);
#define CCAST(x) \
    assert(x != nullptr); \
    const PatcherConfig *config = reinterpret_cast<const PatcherConfig *>(x);


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
    return reinterpret_cast<CPatcherConfig *>(new PatcherConfig());
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
 * \note The returned CPatcherError is filled with valid data only if a
 *       CPatcherConfig operation has failed.
 *
 * \note The returned CPatcherError should be freed with mbp_error_destroy()
 *       when it is no longer needed.
 *
 * \param config CPatcherConfig object
 *
 * \return CPatcherError
 *
 * \sa PatcherConfig::error()
 */
CPatcherError * mbp_config_error(const CPatcherConfig *pc)
{
    CCAST(pc);
    PatcherError *pe = new PatcherError(config->error());
    return reinterpret_cast<CPatcherError *>(pe);
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
 * \brief Get version number of the patcher
 *
 * \note The returned string is dynamically allocated. It should be free()'d
 *       when it is no longer needed.
 *
 * \param pc CPatcherConfig object
 * \return Version number
 *
 * \sa PatcherConfig::version()
 */
char * mbp_config_version(const CPatcherConfig *pc)
{
    CCAST(pc);
    return string_to_cstring(config->version());
}

/*!
 * \brief Get list of supported devices
 *
 * \note The returned array should be freed with `free()` when it
 *       is no longer needed.
 *
 * \param pc CPatcherConfig object
 * \return NULL-terminated array of supported devices
 *
 * \sa PatcherConfig::devices()
 */
CDevice ** mbp_config_devices(const CPatcherConfig *pc)
{
    CCAST(pc);
    auto const devices = config->devices();

    CDevice **cDevices = (CDevice **) std::malloc(
            sizeof(CDevice *) * (devices.size() + 1));
    for (unsigned int i = 0; i < devices.size(); ++i) {
        cDevices[i] = reinterpret_cast<CDevice *>(devices[i]);
    }
    cDevices[devices.size()] = nullptr;

    return cDevices;
}

#ifndef LIBMBP_MINI

/*!
 * \brief Get list of PatchInfos
 *
 * \note The returned array should be freed with `free()` when it
 *       is no longer needed.
 *
 * \param pc CPatcherConfig object
 * \return NULL-terminated array of PatchInfos
 *
 * \sa PatcherConfig::patchInfos()
 */
CPatchInfo ** mbp_config_patchinfos(const CPatcherConfig *pc)
{
    CCAST(pc);
    auto const infos = config->patchInfos();

    CPatchInfo **cInfos = (CPatchInfo **) std::malloc(
            sizeof(CPatchInfo *) * (infos.size() + 1));
    for (unsigned int i = 0; i < infos.size(); ++i) {
        cInfos[i] = reinterpret_cast<CPatchInfo *>(infos[i]);
    }
    cInfos[infos.size()] = nullptr;

    return cInfos;
}

/*!
 * \brief Get list of PatchInfos for a device
 *
 * \note The returned array should be freed with `free()` when it
 *       is no longer needed.
 *
 * \param pc CPatcherConfig object
 * \param device Supported device
 * \return NULL-terminated array of PatchInfos
 *
 * \sa PatcherConfig::patchInfos(const Device * const) const
 */
CPatchInfo ** mbp_config_patchinfos_for_device(const CPatcherConfig *pc,
                                               const CDevice *device)
{
    CCAST(pc);
    const Device *d = reinterpret_cast<const Device *>(device);
    auto const infos = config->patchInfos(d);

    CPatchInfo **cInfos = (CPatchInfo **) std::malloc(
            sizeof(CPatchInfo *) * (infos.size() + 1));
    for (unsigned int i = 0; i < infos.size(); ++i) {
        cInfos[i] = reinterpret_cast<CPatchInfo *>(infos[i]);
    }
    cInfos[infos.size()] = nullptr;

    return cInfos;
}

/*!
 * \brief Find matching PatchInfo for a file
 *
 * \param pc CPatcherConfig object
 * \param device Supported device
 * \param filename Supported file
 * \return CPatchInfo if found, otherwise NULL
 *
 * \sa PatcherConfig::findMatchingPatchInfo()
 */
CPatchInfo * mbp_config_find_matching_patchinfo(const CPatcherConfig *pc,
                                                CDevice *device,
                                                const char *filename)
{
    CCAST(pc);
    Device *d = reinterpret_cast<Device *>(device);
    PatchInfo *pi = config->findMatchingPatchInfo(d, filename);
    return reinterpret_cast<CPatchInfo *>(pi);
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
 * \brief Get list of RamdiskPatcher IDs
 *
 * \note The returned array should be freed with `mbp_free_array()` when it
 *       is no longer needed.
 *
 * \param pc CPatcherConfig object
 * \return NULL-terminated array of RamdiskPatcher IDs
 *
 * \sa PatcherConfig::ramdiskPatchers()
 */
char ** mbp_config_ramdiskpatchers(const CPatcherConfig *pc)
{
    CCAST(pc);
    return vector_to_cstring_array(config->ramdiskPatchers());
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
    Patcher *p = config->createPatcher(id);
    return reinterpret_cast<CPatcher *>(p);
}

/*!
 * \brief Create new AutoPatcher
 *
 * \param pc CPatcherConfig object
 * \param id AutoPatcher ID
 * \param info FileInfo describing file to be patched
 * \param args AutoPatcher arguments
 * \return New AutoPatcher
 *
 * \sa PatcherConfig::createAutoPatcher()
 */
CAutoPatcher * mbp_config_create_autopatcher(CPatcherConfig *pc,
                                             const char *id,
                                             const CFileInfo *info,
                                             const CStringMap *args)
{
    CAST(pc);
    const FileInfo *fi = reinterpret_cast<const FileInfo *>(info);
    const PatchInfo::AutoPatcherArgs *apArgs =
            reinterpret_cast<const PatchInfo::AutoPatcherArgs *>(args);
    AutoPatcher *ap = config->createAutoPatcher(id, fi, *apArgs);
    return reinterpret_cast<CAutoPatcher *>(ap);
}

/*!
 * \brief Create new RamdiskPatcher
 *
 * \param pc CPatcherConfig object
 * \param id RamdiskPatcher ID
 * \param info FileInfo describing file to be patched
 * \param cpio CpioFile for the ramdisk archive
 * \return New RamdiskPatcher
 *
 * \sa PatcherConfig::createRamdiskPatcher()
 */
CRamdiskPatcher * mbp_config_create_ramdisk_patcher(CPatcherConfig *pc,
                                                    const char *id,
                                                    const CFileInfo *info,
                                                    CCpioFile *cpio)
{
    CAST(pc);
    const FileInfo *fi = reinterpret_cast<const FileInfo *>(info);
    CpioFile *cf = reinterpret_cast<CpioFile *>(cpio);
    RamdiskPatcher *rp = config->createRamdiskPatcher(id, fi, cf);
    return reinterpret_cast<CRamdiskPatcher *>(rp);
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
    Patcher *p = reinterpret_cast<Patcher *>(patcher);
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
    AutoPatcher *ap = reinterpret_cast<AutoPatcher *>(patcher);
    config->destroyAutoPatcher(ap);
}

/*!
 * \brief Destroys a RamdiskPatcher and frees its memory
 *
 * \param pc CPatcherConfig object
 * \param patcher CRamdiskPatcher to destroy
 *
 * \sa PatcherConfig::destroyRamdiskPatcher()
 */
void mbp_config_destroy_ramdisk_patcher(CPatcherConfig *pc,
                                        CRamdiskPatcher *patcher)
{
    CAST(pc);
    RamdiskPatcher *rp = reinterpret_cast<RamdiskPatcher *>(patcher);
    config->destroyRamdiskPatcher(rp);
}

/*!
 * \brief Load all PatchInfo XML files
 *
 * \param pc CPatcherConfig object
 * \return Whther the XML files were successfully read (with the error set
 *         appropriately)
 *
 * \sa PatcherConfig::loadPatchInfos()
 */
bool mbp_config_load_patchinfos(CPatcherConfig *pc)
{
    CAST(pc);
    return config->loadPatchInfos();
}

#endif

}
