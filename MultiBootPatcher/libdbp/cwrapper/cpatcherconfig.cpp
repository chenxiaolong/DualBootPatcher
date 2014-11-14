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

#include "patcherpaths.h"


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
        return reinterpret_cast<CPatcherConfig *>(new PatcherPaths());
    }

    /*!
     * \brief Destroys a CPatcherConfig object.
     *
     * \param config CPatcherConfig to destroy
     */
    void mbp_config_destroy(CPatcherConfig *pc)
    {
        delete reinterpret_cast<PatcherPaths *>(pc);
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
     * \sa PatcherPaths::error()
     */
    CPatcherError * mbp_config_error(const CPatcherConfig *pc)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        PatcherError *pe = new PatcherError(pp->error());
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
     * \sa PatcherPaths::binariesDirectory()
     */
    char * mbp_config_binaries_directory(const CPatcherConfig *pc)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        return strdup(pp->binariesDirectory().c_str());
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
     * \sa PatcherPaths::dataDirectory()
     */
    char * mbp_config_data_directory(const CPatcherConfig *pc)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        return strdup(pp->dataDirectory().c_str());
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
     * \sa PatcherPaths::initsDirectory()
     */
    char * mbp_config_inits_directory(const CPatcherConfig *pc)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        return strdup(pp->initsDirectory().c_str());
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
     * \sa PatcherPaths::patchesDirectory()
     */
    char * mbp_config_patches_directory(const CPatcherConfig *pc)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        return strdup(pp->patchesDirectory().c_str());
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
     * \sa PatcherPaths::patchInfosDirectory()
     */
    char * mbp_config_patchinfos_directory(const CPatcherConfig *pc)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        return strdup(pp->patchInfosDirectory().c_str());
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
     * \sa PatcherPaths::scriptsDirectory()
     */
    char * mbp_config_scripts_directory(const CPatcherConfig *pc)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        return strdup(pp->scriptsDirectory().c_str());
    }

    /*!
     * \brief Set binaries directory
     *
     * \param pc CPatcherConfig object
     * \param path Path to binaries directory
     *
     * \sa PatcherPaths::setBinariesDirectory()
     */
    void mbp_config_set_binaries_directory(CPatcherConfig *pc, char *path)
    {
        PatcherPaths *pp = reinterpret_cast<PatcherPaths *>(pc);
        pp->setBinariesDirectory(path);
    }

    /*!
     * \brief Set top-level data directory
     *
     * \param pc CPatcherConfig object
     * \param path Path to data directory
     *
     * \sa PatcherPaths::setDataDirectory()
     */
    void mbp_config_set_data_directory(CPatcherConfig *pc, char *path)
    {
        PatcherPaths *pp = reinterpret_cast<PatcherPaths *>(pc);
        pp->setDataDirectory(path);
    }

    /*!
     * \brief Set init binaries directory
     *
     * \param pc CPatcherConfig object
     * \param path Path to init binaries directory
     *
     * \sa PatcherPaths::setInitsDirectory()
     */
    void mbp_config_set_inits_directory(CPatcherConfig *pc, char *path)
    {
        PatcherPaths *pp = reinterpret_cast<PatcherPaths *>(pc);
        pp->setInitsDirectory(path);
    }

    /*!
     * \brief Set patch files directory
     *
     * \param pc CPatcherConfig object
     * \param path Path to patch files directory
     *
     * \sa PatcherPaths::setPatchesDirectory()
     */
    void mbp_config_set_patches_directory(CPatcherConfig *pc, char *path)
    {
        PatcherPaths *pp = reinterpret_cast<PatcherPaths *>(pc);
        pp->setPatchesDirectory(path);
    }

    /*!
     * \brief Set PatchInfo files directory
     *
     * \param pc CPatcherConfig object
     * \param path Path to PatchInfo files directory
     *
     * \sa PatcherPaths::setPatchInfosDirectory()
     */
    void mbp_config_set_patchinfos_directory(CPatcherConfig *pc, char *path)
    {
        PatcherPaths *pp = reinterpret_cast<PatcherPaths *>(pc);
        pp->setPatchInfosDirectory(path);
    }

    /*!
     * \brief Set shell scripts directory
     *
     * \param pc CPatcherConfig object
     * \param path Path to shell scripts directory
     *
     * \sa PatcherPaths::setScriptsDirectory()
     */
    void mbp_config_set_scripts_directory(CPatcherConfig *pc, char *path)
    {
        PatcherPaths *pp = reinterpret_cast<PatcherPaths *>(pc);
        pp->setScriptsDirectory(path);
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
     * \sa PatcherPaths::version()
     */
    char * mbp_config_version(const CPatcherConfig *pc)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        return strdup(pp->version().c_str());
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
     * \sa PatcherPaths::devices()
     */
    CDevice ** mbp_config_devices(const CPatcherConfig *pc)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        auto const devices = pp->devices();

        CDevice **cDevices = (CDevice **) std::malloc(
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
     * \sa PatcherPaths::patchInfos()
     */
    CPatchInfo ** mbp_config_patchinfos(const CPatcherConfig *pc)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        auto const infos = pp->patchInfos();

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
     * \sa PatcherPaths::patchInfos(const Device * const) const
     */
    CPatchInfo ** mbp_config_patchinfos_for_device(const CPatcherConfig *pc,
                                                   const CDevice *device)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        const Device *d = reinterpret_cast<const Device *>(device);
        auto const infos = pp->patchInfos(d);

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
     * \sa PatcherPaths::findMatchingPatchInfo()
     */
    CPatchInfo * mbp_config_find_matching_patchinfo(const CPatcherConfig *pc,
                                                    CDevice *device,
                                                    const char *filename)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        Device *d = reinterpret_cast<Device *>(device);
        PatchInfo *pi = pp->findMatchingPatchInfo(d, filename);
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
     * \sa PatcherPaths::patchers()
     */
    char ** mbp_config_patchers(const CPatcherConfig *pc)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        auto const ids = pp->patchers();

        char **cIds = (char **) std::malloc(
                sizeof(char *) * (ids.size() + 1));
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
     * \sa PatcherPaths::autoPatchers()
     */
    char **mbp_config_autopatchers(const CPatcherConfig *pc)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        auto const ids = pp->autoPatchers();

        char **cIds = (char **) std::malloc(
                sizeof(char *) * (ids.size() + 1));
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
     * \sa PatcherPaths::ramdiskPatchers()
     */
    char ** mbp_config_ramdiskpatchers(const CPatcherConfig *pc)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        auto const ids = pp->ramdiskPatchers();

        char **cIds = (char **) std::malloc(
                sizeof(char *) * (ids.size() + 1));
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
     * \sa PatcherPaths::patcherName()
     */
    char * mbp_config_patcher_name(const CPatcherConfig *pc, const char *id)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        return strdup(pp->patcherName(id).c_str());
    }

    /*!
     * \brief Create new Patcher
     *
     * \param pc CPatcherConfig object
     * \param id Patcher ID
     * \return New Patcher
     *
     * \sa PatcherPaths::createPatcher()
     */
    CPatcher * mbp_config_create_patcher(CPatcherConfig *pc,
                                         const char *id)
    {
        PatcherPaths *pp = reinterpret_cast<PatcherPaths *>(pc);
        Patcher *p = pp->createPatcher(id);
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
     * \sa PatcherPaths::createAutoPatcher()
     */
    CAutoPatcher * mbp_config_create_autopatcher(CPatcherConfig *pc,
                                                 const char *id,
                                                 const CFileInfo *info,
                                                 const CStringMap *args)
    {
        PatcherPaths *pp = reinterpret_cast<PatcherPaths *>(pc);
        const FileInfo *fi = reinterpret_cast<const FileInfo *>(info);
        const PatchInfo::AutoPatcherArgs *apArgs =
                reinterpret_cast<const PatchInfo::AutoPatcherArgs *>(args);
        AutoPatcher *ap = pp->createAutoPatcher(id, fi, *apArgs);
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
     * \sa PatcherPaths::createRamdiskPatcher()
     */
    CRamdiskPatcher * mbp_config_create_ramdisk_patcher(CPatcherConfig *pc,
                                                        const char *id,
                                                        const CFileInfo *info,
                                                        CCpioFile *cpio)
    {
        PatcherPaths *pp = reinterpret_cast<PatcherPaths *>(pc);
        const FileInfo *fi = reinterpret_cast<const FileInfo *>(info);
        CpioFile *cf = reinterpret_cast<CpioFile *>(cpio);
        RamdiskPatcher *rp = pp->createRamdiskPatcher(id, fi, cf);
        return reinterpret_cast<CRamdiskPatcher *>(rp);
    }

    /*!
     * \brief Destroys a Patcher and frees its memory
     *
     * \param pc CPatcherConfig object
     * \param patcher CPatcher to destroy
     *
     * \sa PatcherPaths::destroyPatcher()
     */
    void mbp_config_destroy_patcher(CPatcherConfig *pc, CPatcher *patcher)
    {
        PatcherPaths *pp = reinterpret_cast<PatcherPaths *>(pc);
        Patcher *p = reinterpret_cast<Patcher *>(patcher);
        pp->destroyPatcher(p);
    }

    /*!
     * \brief Destroys an AutoPatcher and frees its memory
     *
     * \param pc CPatcherConfig object
     * \param patcher CAutoPatcher to destroy
     *
     * \sa PatcherPaths::destroyAutoPatcher()
     */
    void mbp_config_destroy_autopatcher(CPatcherConfig *pc,
                                        CAutoPatcher *patcher)
    {
        PatcherPaths *pp = reinterpret_cast<PatcherPaths *>(pc);
        AutoPatcher *ap = reinterpret_cast<AutoPatcher *>(patcher);
        pp->destroyAutoPatcher(ap);
    }

    /*!
     * \brief Destroys a RamdiskPatcher and frees its memory
     *
     * \param pc CPatcherConfig object
     * \param patcher CRamdiskPatcher to destroy
     *
     * \sa PatcherPaths::destroyRamdiskPatcher()
     */
    void mbp_config_destroy_ramdisk_patcher(CPatcherConfig *pc,
                                            CRamdiskPatcher *patcher)
    {
        PatcherPaths *pp = reinterpret_cast<PatcherPaths *>(pc);
        RamdiskPatcher *rp = reinterpret_cast<RamdiskPatcher *>(patcher);
        pp->destroyRamdiskPatcher(rp);
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
     * \sa PatcherPaths::partitionConfigs()
     */
    CPartConfig ** mbp_config_partitionconfigs(const CPatcherConfig *pc)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        auto const configs = pp->partitionConfigs();

        CPartConfig **cConfigs = (CPartConfig **) std::malloc(
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
     * \sa PatcherPaths::initBinaries()
     */
    char ** mbp_config_init_binaries(const CPatcherConfig *pc)
    {
        const PatcherPaths *pp = reinterpret_cast<const PatcherPaths *>(pc);
        auto const inits = pp->initBinaries();

        char **cInits = (char **) std::malloc(
                sizeof(char *) * (inits.size() + 1));
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
     * \sa PatcherPaths::loadPatchInfos()
     */
    int mbp_config_load_patchinfos(CPatcherConfig *pc)
    {
        PatcherPaths *pp = reinterpret_cast<PatcherPaths *>(pc);
        bool ret = pp->loadPatchInfos();
        return ret ? 0 : -1;
    }

}
