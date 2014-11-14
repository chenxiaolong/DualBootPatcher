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

#include "cwrapper/cpatchinfo.h"

#include <cstdlib>
#include <cstring>

#include "patchinfo.h"


/*!
 * \file cpatchinfo.h
 * \brief C Wrapper for PatchInfo
 *
 * Please see the documentation for PatchInfo from the C++ API for more details.
 * The C functions directly correspond to the PatchInfo member functions.
 *
 * \sa PatchInfo
 */

extern "C" {

    // Static constants

    /*! \brief Key for getting the default values */
    const char * mbp_patchinfo_default()
    {
        return PatchInfo::Default.c_str();
    }

    /*! \brief Key for getting the `<not-matched>` values */
    const char *mbp_patchinfo_notmatched()
    {
        return PatchInfo::NotMatched.c_str();
    }


    /*!
     * \brief Create a new PatchInfo object.
     *
     * \note The returned object must be freed with mbp_patchinfo_destroy().
     *
     * \return New CPatchInfo
     */
    CPatchInfo * mbp_patchinfo_create()
    {
        return reinterpret_cast<CPatchInfo *>(new PatchInfo());
    }

    /*!
     * \brief Destroys a CPatchInfo object.
     *
     * \param info CPatchInfo to destroy
     */
    void mbp_patchinfo_destroy(CPatchInfo *info)
    {
        delete reinterpret_cast<PatchInfo *>(info);
    }

    /*!
     * \brief PatchInfo identifier
     *
     * \note The output data is dynamically allocated. It should be `free()`'d
     *       when it is no longer needed.
     *
     * \param info CPatchInfo object
     *
     * \return PatchInfo ID
     *
     * \sa PatchInfo::id()
     */
    char * mbp_patchinfo_id(const CPatchInfo *info)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        return strdup(pi->id().c_str());
    }

    /*!
     * \brief Set the PatchInfo ID
     *
     * \param info CPatchInfo object
     * \param id ID
     *
     * \sa PatchInfo::setId()
     */
    void mbp_patchinfo_set_id(CPatchInfo *info, const char *id)
    {
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(info);
        pi->setId(id);
    }

    /*!
     * \brief Name of ROM or kernel
     *
     * \note The output data is dynamically allocated. It should be `free()`'d
     *       when it is no longer needed.
     *
     * \param info CPatchInfo object
     *
     * \return ROM or kernel name
     *
     * \sa PatchInfo::name()
     */
    char * mbp_patchinfo_name(const CPatchInfo *info)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        return strdup(pi->name().c_str());
    }

    /*!
     * \brief Set name of ROM or kernel
     *
     * \param info CPatchInfo object
     * \param name Name of ROM or kernel
     *
     * \sa PatchInfo::setName()
     */
    void mbp_patchinfo_set_name(CPatchInfo *info, const char *name)
    {
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(info);
        pi->setName(name);
    }

    /*!
     * \brief Get the parameter key for a filename
     *
     * \note The output data is dynamically allocated. It should be `free()`'d
     *       when it is no longer needed.
     *
     * \param info CPatchInfo object
     * \param fileName Filename
     *
     * \return Conditional regex OR mbp_patchinfo_notmatched() OR
     *         mbp_patchinfo_default()
     *
     * \sa PatchInfo::keyFromFilename()
     */
    char * mbp_patchinfo_key_from_filename(const CPatchInfo *info,
                                           const char *fileName)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        return strdup(pi->keyFromFilename(fileName).c_str());
    }

    /*!
     * \brief List of filename matching regexes
     *
     * \note The returned array should be freed with `mbp_free_array()` when it
     *       is no longer needed.
     *
     * \param info CPatchInfo object
     *
     * \return A NULL-terminated array containing the regexes
     *
     * \sa PatchInfo::regexes()
     */
    char ** mbp_patchinfo_regexes(const CPatchInfo *info)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        auto const regexes = pi->regexes();

        char **cRegexes = (char **) std::malloc(
                sizeof(char *) * (regexes.size() + 1));
        for (unsigned int i = 0; i < regexes.size(); ++i) {
            cRegexes[i] = strdup(regexes[i].c_str());
        }
        cRegexes[regexes.size()] = nullptr;

        return cRegexes;
    }

    /*!
     * \brief Set the list of filename matching regexes
     *
     * \param info CPatchInfo object
     * \param regexes NULL-terminated list of regexes
     *
     * \sa PatchInfo::setRegexes()
     */
    void mbp_patchinfo_set_regexes(CPatchInfo *info, const char **regexes)
    {
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(info);
        std::vector<std::string> list;

        for (; *regexes != NULL; ++regexes) {
            list.push_back(*regexes);
        }

        pi->setRegexes(std::move(list));
    }

    /*!
     * \brief List of filename excluding regexes
     *
     * \note The returned array should be freed with `mbp_free_array()` when it
     *       is no longer needed.
     *
     * \param info CPatchInfo object
     *
     * \return A NULL-terminated array containing the exclusion regexes
     *
     * \sa PatchInfo::excludeRegexes()
     */
    char ** mbp_patchinfo_exclude_regexes(const CPatchInfo *info)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        auto const regexes = pi->excludeRegexes();

        char **cRegexes = (char **) std::malloc(
                sizeof(char *) * (regexes.size() + 1));
        for (unsigned int i = 0; i < regexes.size(); ++i) {
            cRegexes[i] = strdup(regexes[i].c_str());
        }
        cRegexes[regexes.size()] = nullptr;

        return cRegexes;
    }

    /*!
     * \brief Set the list of filename excluding regexes
     *
     * \param info CPatchInfo object
     * \param regexes NULL-terminated list of exclusion regexes
     *
     * \sa PatchInfo::setExcludeRegexes()
     */
    void mbp_patchinfo_set_exclude_regexes(CPatchInfo *info,
                                           const char **regexes)
    {
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(info);
        std::vector<std::string> list;

        for (; *regexes != NULL; ++regexes) {
            list.push_back(*regexes);
        }

        pi->setExcludeRegexes(std::move(list));
    }

    /*!
     * \brief List of conditional regexes for parameters
     *
     * \note The returned array should be freed with `mbp_free_array()` when it
     *       is no longer needed.
     *
     * \param info CPatchInfo object
     *
     * \return A NULL-terminated array containing the conditional regexes
     *
     * \sa PatchInfo::condRegexes()
     */
    char ** mbp_patchinfo_cond_regexes(const CPatchInfo *info)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        auto const regexes = pi->condRegexes();

        char **cRegexes = (char **) std::malloc(
                sizeof(char *) * (regexes.size() + 1));
        for (unsigned int i = 0; i < regexes.size(); ++i) {
            cRegexes[i] = strdup(regexes[i].c_str());
        }
        cRegexes[regexes.size()] = nullptr;

        return cRegexes;
    }

    /*!
     * \brief Set the list of conditional regexes
     *
     * \param info CPatchInfo object
     * \param regexes NULL-terminated list of conditional regexes
     *
     * \sa PatchInfo::setCondRegexes()
     */
    void mbp_patchinfo_set_cond_regexes(CPatchInfo *info, const char **regexes)
    {
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(info);
        std::vector<std::string> list;

        for (; *regexes != NULL; ++regexes) {
            list.push_back(*regexes);
        }

        pi->setCondRegexes(std::move(list));
    }

    /*!
     * \brief Check if the PatchInfo has a <not-matched> element
     *
     * \param info CPatchInfo object
     *
     * \return 0 if PatchInfo has a <not-matched> element, otherwise -1
     *
     * \sa PatchInfo::hasNotMatched()
     */
    int mbp_patchinfo_has_not_matched(const CPatchInfo *info)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        return pi->hasNotMatched() ? 0 : -1;
    }

    /*!
     * \brief Set whether the PatchInfo has a <not-matched> element
     *
     * \param info CPatchInfo object
     * \param hasElem Has not matched element
     *
     * \sa PatchInfo::setHasNotMatched()
     */
    void mbp_patchinfo_set_has_not_matched(CPatchInfo *info, int hasElem)
    {
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(info);
        pi->setHasNotMatched(hasElem != 0);
    }

    /*!
     * \brief Add AutoPatcher to PatchInfo
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     * \param apName AutoPatcher name
     * \param args AutoPatcher arguments
     *
     * \sa PatchInfo::addAutoPatcher()
     */
    void mbp_patchinfo_add_autopatcher(CPatchInfo *info, const char *key,
                                       const char *apName, CStringMap *args)
    {
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(info);
        PatchInfo::AutoPatcherArgs *apArgs =
                reinterpret_cast<PatchInfo::AutoPatcherArgs *>(args);

        pi->addAutoPatcher(key, apName, *apArgs);
    }

    /*!
     * \brief Remove AutoPatcher from PatchInfo
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     * \param apName AutoPatcher name
     *
     * \sa PatchInfo::removeAutoPatcher()
     */
    void mbp_patchinfo_remove_autopatcher(CPatchInfo *info, const char *key,
                                          const char *apName)
    {
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(info);
        pi->removeAutoPatcher(key, apName);
    }

    /*!
     * \brief Get list of AutoPatcher names
     *
     * \note The returned array should be freed with `mbp_free_array()` when it
     *       is no longer needed.
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     *
     * \return NULL-terminated list of AutoPatcher names
     *
     * \sa PatchInfo::autoPatchers()
     */
    char ** mbp_patchinfo_autopatchers(const CPatchInfo *info, const char *key)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        auto list = pi->autoPatchers(key);

        char **names = (char **) std::malloc(
                sizeof(char *) * (list.size() + 1));
        for (unsigned int i = 0; i < list.size(); ++i) {
            names[i] = strdup(list[i].c_str());
        }
        names[list.size()] = nullptr;

        return names;
    }

    /*!
     * \brief Get AutoPatcher arguments
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     * \param apName AutoPatcher name
     *
     * \return Arguments
     *
     * \sa PatchInfo::autoPatcherArgs()
     */
    CStringMap * mbp_patchinfo_autopatcher_args(const CPatchInfo *info,
                                                const char *key,
                                                const char *apName)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        PatchInfo::AutoPatcherArgs *args =
                new PatchInfo::AutoPatcherArgs(pi->autoPatcherArgs(key, apName));
        return reinterpret_cast<CStringMap *>(args);
    }

    /*!
     * \brief Whether the patched file has a boot image
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     *
     * \return 0 if the patched file has a boot image, otherwise -1
     *
     * \sa PatchInfo::hasBootImage()
     */
    int mbp_patchinfo_has_boot_image(const CPatchInfo *info, const char *key)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        return pi->hasBootImage(key) ? 0 : -1;
    }

    /*!
     * \brief Set whether the patched file has a boot image
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     * \param hasBootImage Has boot image
     *
     * \sa PatchInfo::setHasBootImage()
     */
    void mbp_patchinfo_set_has_boot_image(CPatchInfo *info,
                                          const char *key, int hasBootImage)
    {
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(info);
        pi->setHasBootImage(key, hasBootImage != 0);
    }

    /*!
     * \brief Whether boot images should be autodetected
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     *
     * \return 0 if boot images should be autodetected, otherwise -1
     *
     * \sa PatchInfo::autodetectBootImages()
     */
    int mbp_patchinfo_autodetect_boot_images(const CPatchInfo *info,
                                             const char *key)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        return pi->autodetectBootImages(key) ? 0 : -1;
    }

    /*!
     * \brief Set whether boot images should be autodetected
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     * \param autoDetect Autodetect boot images
     *
     * \sa PatchInfo::setAutoDetectBootImages()
     */
    void mbp_patchinfo_set_autodetect_boot_images(CPatchInfo *info,
                                                  const char *key, int autoDetect)
    {
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(info);
        pi->setAutoDetectBootImages(key, autoDetect != 0);
    }

    /*!
     * \brief List of manually specified boot images
     *
     * \note The returned array should be freed with `mbp_free_array()` when it
     *       is no longer needed.
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     *
     * \return A NULL-terminated array containing the manually specified boot
     *         images
     *
     * \sa PatchInfo::bootImages()
     */
    char ** mbp_patchinfo_boot_images(const CPatchInfo *info, const char *key)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        auto const bootImages = pi->bootImages(key);

        char **cBootImages = (char **) std::malloc(
                sizeof(char *) * (bootImages.size() + 1));
        for (unsigned int i = 0; i < bootImages.size(); ++i) {
            cBootImages[i] = strdup(bootImages[i].c_str());
        }
        cBootImages[bootImages.size()] = nullptr;

        return cBootImages;
    }

    /*!
     * \brief Set list of manually specified boot images
     *
     * \param info CPatchInfo object
     * \param bootImages NULL-terminated list of boot images
     *
     * \sa PatchInfo::setBootImages()
     */
    void mbp_patchinfo_set_boot_images(CPatchInfo *info,
                                       const char *key, const char **bootImages)
    {
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(info);
        std::vector<std::string> list;

        for (; *bootImages != NULL; ++bootImages) {
            list.push_back(*bootImages);
        }

        pi->setBootImages(key, std::move(list));
    }

    /*!
     * \brief Which ramdisk patcher to use
     *
     * \note The output data is dynamically allocated. It should be `free()`'d
     *       when it is no longer needed.
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     *
     * \return Which ramdisk patcher to use
     *
     * \sa PatchInfo::ramdisk()
     */
    char * mbp_patchinfo_ramdisk(const CPatchInfo *info, const char *key)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        return strdup(pi->ramdisk(key).c_str());
    }

    /*!
     * \brief Set which ramdisk patcher to use
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     * \param ramdisk Which ramdisk patcher to use
     *
     * \sa PatchInfo::setRamdisk()
     */
    void mbp_patchinfo_set_ramdisk(CPatchInfo *info,
                                   const char *key, const char *ramdisk)
    {
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(info);
        pi->setRamdisk(key, ramdisk);
    }

    /*!
     * \brief Which patched init binary to use
     *
     * \note The output data is dynamically allocated. It should be `free()`'d
     *       when it is no longer needed.
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     *
     * \return Which patched init binary to use
     *
     * \sa PatchInfo::patchedInit()
     */
    char * mbp_patchinfo_patched_init(const CPatchInfo *info, const char *key)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        return strdup(pi->patchedInit(key).c_str());
    }

    /*!
     * \brief Set which patched init binary to use
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     * \param init Patched init binary
     *
     * \sa PatchInfo::setPatchedInit()
     */
    void mbp_patchinfo_set_patched_init(CPatchInfo *info,
                                        const char *key, const char *init)
    {
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(info);
        pi->setPatchedInit(key, init);
    }

    /*!
     * \brief Whether device model checks should be kept
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     *
     * \return 0 if the device model checks should be kept, otherwise -1
     *
     * \sa PatchInfo::deviceCheck()
     */
    int mbp_patchinfo_device_check(const CPatchInfo *info, const char *key)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        return pi->deviceCheck(key) ? 0 : -1;
    }

    /*!
     * \brief Set whether device model checks should be kept
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     * \param deviceCheck Keep device model checks
     *
     * \sa PatchInfo::setDeviceCheck()
     */
    void mbp_patchinfo_set_device_check(CPatchInfo *info,
                                        const char *key, int deviceCheck)
    {
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(info);
        pi->setDeviceCheck(key, deviceCheck != 0);
    }

    /*!
     * \brief List of supported partition configurations
     *
     * \note The returned array should be freed with `mbp_free_array()` when it
     *       is no longer needed.
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     *
     * \return A NULL-terminated array containing the supported PartitionConfig
     *         IDs
     *
     * \sa PatchInfo::supportedConfigs()
     */
    char ** mbp_patchinfo_supported_configs(const CPatchInfo *info,
                                            const char *key)
    {
        const PatchInfo *pi = reinterpret_cast<const PatchInfo *>(info);
        auto const configs = pi->supportedConfigs(key);

        char **cConfigs = (char **) std::malloc(
                sizeof(char *) * (configs.size() + 1));
        for (unsigned int i = 0; i < configs.size(); ++i) {
            cConfigs[i] = strdup(configs[i].c_str());
        }
        cConfigs[configs.size()] = nullptr;

        return cConfigs;
    }

    /*!
     * \brief Set list of supported partition configurations
     *
     * \param info CPatchInfo object
     * \param key Parameter key
     * \param configs List of supported PartitionConfig IDs
     *
     * \sa PatchInfo::setSupportedConfigs()
     */
    void mbp_patchinfo_set_supported_configs(CPatchInfo *info,
                                             const char *key, char **configs)
    {
        PatchInfo *pi = reinterpret_cast<PatchInfo *>(info);
        std::vector<std::string> list;

        for (; *configs != NULL; ++configs) {
            list.push_back(*configs);
        }

        pi->setSupportedConfigs(key, std::move(list));
    }

}
