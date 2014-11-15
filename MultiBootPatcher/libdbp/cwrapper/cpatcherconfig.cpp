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

#include "cwrapper/cpatcherconfig.h"

#include <cstring>

#include "patcherconfig.h"


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
    CPatcherConfig * mbp_config_create()
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
        delete reinterpret_cast<PatcherConfig *>(pc);
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
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        PatcherError *pe = new PatcherError(config->error());
        return reinterpret_cast<CPatcherError *>(pe);
    }

    /*!
     * \brief Get binaries directory
     *
     * \note The returned string is dynamically allocated. It should be free()'d
     *       when it is no longer needed.
     *
     * \param pc CPatcherConfig object
     * \return Binaries directory
     *
     * \sa PatcherConfig::binariesDirectory()
     */
    char * mbp_config_binaries_directory(const CPatcherConfig *pc)
    {
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        return strdup(config->binariesDirectory().c_str());
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
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        return strdup(config->dataDirectory().c_str());
    }

    /*!
     * \brief Get init binaries directory
     *
     * \note The returned string is dynamically allocated. It should be free()'d
     *       when it is no longer needed.
     *
     * \param pc CPatcherConfig object
     * \return Init binaries directory
     *
     * \sa PatcherConfig::initsDirectory()
     */
    char * mbp_config_inits_directory(const CPatcherConfig *pc)
    {
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        return strdup(config->initsDirectory().c_str());
    }

    /*!
     * \brief Get patch files directory
     *
     * \note The returned string is dynamically allocated. It should be free()'d
     *       when it is no longer needed.
     *
     * \param pc CPatcherConfig object
     * \return Patch files directory
     *
     * \sa PatcherConfig::patchesDirectory()
     */
    char * mbp_config_patches_directory(const CPatcherConfig *pc)
    {
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        return strdup(config->patchesDirectory().c_str());
    }

    /*!
     * \brief Get PatchInfo files directory
     *
     * \note The returned string is dynamically allocated. It should be free()'d
     *       when it is no longer needed.
     *
     * \param pc CPatcherConfig object
     * \return PatchInfo files directory
     *
     * \sa PatcherConfig::patchInfosDirectory()
     */
    char * mbp_config_patchinfos_directory(const CPatcherConfig *pc)
    {
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        return strdup(config->patchInfosDirectory().c_str());
    }

    /*!
     * \brief Get shell scripts directory
     *
     * \note The returned string is dynamically allocated. It should be free()'d
     *       when it is no longer needed.
     *
     * \param pc CPatcherConfig object
     * \return Shell scripts directory
     *
     * \sa PatcherConfig::scriptsDirectory()
     */
    char * mbp_config_scripts_directory(const CPatcherConfig *pc)
    {
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        return strdup(config->scriptsDirectory().c_str());
    }

    /*!
     * \brief Set binaries directory
     *
     * \param pc CPatcherConfig object
     * \param path Path to binaries directory
     *
     * \sa PatcherConfig::setBinariesDirectory()
     */
    void mbp_config_set_binaries_directory(CPatcherConfig *pc, char *path)
    {
        PatcherConfig *config = reinterpret_cast<PatcherConfig *>(pc);
        config->setBinariesDirectory(path);
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
        PatcherConfig *config = reinterpret_cast<PatcherConfig *>(pc);
        config->setDataDirectory(path);
    }

    /*!
     * \brief Set init binaries directory
     *
     * \param pc CPatcherConfig object
     * \param path Path to init binaries directory
     *
     * \sa PatcherConfig::setInitsDirectory()
     */
    void mbp_config_set_inits_directory(CPatcherConfig *pc, char *path)
    {
        PatcherConfig *config = reinterpret_cast<PatcherConfig *>(pc);
        config->setInitsDirectory(path);
    }

    /*!
     * \brief Set patch files directory
     *
     * \param pc CPatcherConfig object
     * \param path Path to patch files directory
     *
     * \sa PatcherConfig::setPatchesDirectory()
     */
    void mbp_config_set_patches_directory(CPatcherConfig *pc, char *path)
    {
        PatcherConfig *config = reinterpret_cast<PatcherConfig *>(pc);
        config->setPatchesDirectory(path);
    }

    /*!
     * \brief Set PatchInfo files directory
     *
     * \param pc CPatcherConfig object
     * \param path Path to PatchInfo files directory
     *
     * \sa PatcherConfig::setPatchInfosDirectory()
     */
    void mbp_config_set_patchinfos_directory(CPatcherConfig *pc, char *path)
    {
        PatcherConfig *config = reinterpret_cast<PatcherConfig *>(pc);
        config->setPatchInfosDirectory(path);
    }

    /*!
     * \brief Set shell scripts directory
     *
     * \param pc CPatcherConfig object
     * \param path Path to shell scripts directory
     *
     * \sa PatcherConfig::setScriptsDirectory()
     */
    void mbp_config_set_scripts_directory(CPatcherConfig *pc, char *path)
    {
        PatcherConfig *config = reinterpret_cast<PatcherConfig *>(pc);
        config->setScriptsDirectory(path);
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
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        return strdup(config->version().c_str());
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
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        auto const devices = config->devices();

        CDevice **cDevices = (CDevice **) malloc(
                sizeof(CDevice *) * (devices.size() + 1));
        for (unsigned int i = 0; i < devices.size(); ++i) {
            cDevices[i] = reinterpret_cast<CDevice *>(devices[i]);
        }
        cDevices[devices.size()] = nullptr;

        return cDevices;
    }

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
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        auto const infos = config->patchInfos();

        CPatchInfo **cInfos = (CPatchInfo **) malloc(
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
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        const Device *d = reinterpret_cast<const Device *>(device);
        auto const infos = config->patchInfos(d);

        CPatchInfo **cInfos = (CPatchInfo **) malloc(
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
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
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
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        auto const ids = config->patchers();

        char **cIds = (char **) malloc(sizeof(char *) * (ids.size() + 1));
        for (unsigned int i = 0; i < ids.size(); ++i) {
            cIds[i] = strdup(ids[i].c_str());
        }
        cIds[ids.size()] = nullptr;

        return cIds;
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
    char **mbp_config_autopatchers(const CPatcherConfig *pc)
    {
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        auto const ids = config->autoPatchers();

        char **cIds = (char **) malloc(sizeof(char *) * (ids.size() + 1));
        for (unsigned int i = 0; i < ids.size(); ++i) {
            cIds[i] = strdup(ids[i].c_str());
        }
        cIds[ids.size()] = nullptr;

        return cIds;
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
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        auto const ids = config->ramdiskPatchers();

        char **cIds = (char **) malloc(sizeof(char *) * (ids.size() + 1));
        for (unsigned int i = 0; i < ids.size(); ++i) {
            cIds[i] = strdup(ids[i].c_str());
        }
        cIds[ids.size()] = nullptr;

        return cIds;
    }

    /*!
     * \brief Get Patcher's friendly name
     *
     * \note The returned string is dynamically allocated. It should be free()'d
     *       when it is no longer needed.
     *
     * \param pc CPatcherConfig object
     * \param id Patcher ID
     * \return Patcher's name
     *
     * \sa PatcherConfig::patcherName()
     */
    char * mbp_config_patcher_name(const CPatcherConfig *pc, const char *id)
    {
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        return strdup(config->patcherName(id).c_str());
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
        PatcherConfig *config = reinterpret_cast<PatcherConfig *>(pc);
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
        PatcherConfig *config = reinterpret_cast<PatcherConfig *>(pc);
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
        PatcherConfig *config = reinterpret_cast<PatcherConfig *>(pc);
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
        PatcherConfig *config = reinterpret_cast<PatcherConfig *>(pc);
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
        PatcherConfig *config = reinterpret_cast<PatcherConfig *>(pc);
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
        PatcherConfig *config = reinterpret_cast<PatcherConfig *>(pc);
        RamdiskPatcher *rp = reinterpret_cast<RamdiskPatcher *>(patcher);
        config->destroyRamdiskPatcher(rp);
    }

    /*!
     * \brief Get list of partition configurations
     *
     * \note The returned array should be freed with `free()` when it
     *       is no longer needed.
     *
     * \param pc CPatcherConfig object
     * \return NULL-terminated array of PartitionConfigs
     *
     * \sa PatcherConfig::partitionConfigs()
     */
    CPartConfig ** mbp_config_partitionconfigs(const CPatcherConfig *pc)
    {
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        auto const configs = config->partitionConfigs();

        CPartConfig **cConfigs = (CPartConfig **) malloc(
                sizeof(CPartConfig *) * (configs.size() + 1));
        for (unsigned int i = 0; i < configs.size(); ++i) {
            cConfigs[i] = reinterpret_cast<CPartConfig *>(configs[i]);
        }
        cConfigs[configs.size()] = nullptr;

        return cConfigs;
    }

    /*!
     * \brief Get list of init binaries
     *
     * \note The returned array should be freed with `mbp_free_array()` when it
     *       is no longer needed.
     *
     * \param pc CPatcherConfig object
     * \return NULL-terminated array of init binaries
     *
     * \sa PatcherConfig::initBinaries()
     */
    char ** mbp_config_init_binaries(const CPatcherConfig *pc)
    {
        const PatcherConfig *config =
                reinterpret_cast<const PatcherConfig *>(pc);
        auto const inits = config->initBinaries();

        char **cInits = (char **) malloc(sizeof(char *) * (inits.size() + 1));
        for (unsigned int i = 0; i < inits.size(); ++i) {
            cInits[i] = strdup(inits[i].c_str());
        }
        cInits[inits.size()] = nullptr;

        return cInits;
    }

    /*!
     * \brief Load all PatchInfo XML files
     *
     * \param pc CPatcherConfig object
     * \return 0 if the XML files were successfully read, otherwise -1 (and the
     *         error set appropriately)
     *
     * \sa PatcherConfig::loadPatchInfos()
     */
    int mbp_config_load_patchinfos(CPatcherConfig *pc)
    {
        PatcherConfig *config = reinterpret_cast<PatcherConfig *>(pc);
        bool ret = config->loadPatchInfos();
        return ret ? 0 : -1;
    }

}
