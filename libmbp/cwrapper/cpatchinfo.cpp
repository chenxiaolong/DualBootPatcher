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

#include "cwrapper/cpatchinfo.h"

#include <cassert>

#include "cwrapper/private/util.h"

#include "patchinfo.h"


#define CAST(x) \
    assert(x != nullptr); \
    mbp::PatchInfo *pi = reinterpret_cast<mbp::PatchInfo *>(x);
#define CCAST(x) \
    assert(x != nullptr); \
    const mbp::PatchInfo *pi = reinterpret_cast<const mbp::PatchInfo *>(x);


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

/*!
 * \brief Create a new PatchInfo object.
 *
 * \note The returned object must be freed with mbp_patchinfo_destroy().
 *
 * \return New CPatchInfo
 */
CPatchInfo * mbp_patchinfo_create(void)
{
    return reinterpret_cast<CPatchInfo *>(new mbp::PatchInfo());
}

/*!
 * \brief Destroys a CPatchInfo object.
 *
 * \param info CPatchInfo to destroy
 */
void mbp_patchinfo_destroy(CPatchInfo *info)
{
    CAST(info);
    delete pi;
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
    CCAST(info);
    return string_to_cstring(pi->id());
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
    CAST(info);
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
    CCAST(info);
    return string_to_cstring(pi->name());
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
    CAST(info);
    pi->setName(name);
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
    CCAST(info);
    return vector_to_cstring_array(pi->regexes());
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
    CAST(info);
    pi->setRegexes(cstring_array_to_vector(regexes));
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
    CCAST(info);
    return vector_to_cstring_array(pi->excludeRegexes());
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
    CAST(info);
    pi->setExcludeRegexes(cstring_array_to_vector(regexes));
}

/*!
 * \brief Add AutoPatcher to PatchInfo
 *
 * \param info CPatchInfo object
 * \param apName AutoPatcher name
 * \param args AutoPatcher arguments
 *
 * \sa PatchInfo::addAutoPatcher()
 */
void mbp_patchinfo_add_autopatcher(CPatchInfo *info, const char *apName,
                                   CStringMap *args)
{
    CAST(info);
    mbp::PatchInfo::AutoPatcherArgs *apArgs =
            reinterpret_cast<mbp::PatchInfo::AutoPatcherArgs *>(args);

    pi->addAutoPatcher(apName, *apArgs);
}

/*!
 * \brief Remove AutoPatcher from PatchInfo
 *
 * \param info CPatchInfo object
 * \param apName AutoPatcher name
 *
 * \sa PatchInfo::removeAutoPatcher()
 */
void mbp_patchinfo_remove_autopatcher(CPatchInfo *info, const char *apName)
{
    CAST(info);
    pi->removeAutoPatcher(apName);
}

/*!
 * \brief Get list of AutoPatcher names
 *
 * \note The returned array should be freed with `mbp_free_array()` when it
 *       is no longer needed.
 *
 * \param info CPatchInfo object
 *
 * \return NULL-terminated list of AutoPatcher names
 *
 * \sa PatchInfo::autoPatchers()
 */
char ** mbp_patchinfo_autopatchers(const CPatchInfo *info)
{
    CCAST(info);
    return vector_to_cstring_array(pi->autoPatchers());
}

/*!
 * \brief Get AutoPatcher arguments
 *
 * \param info CPatchInfo object
 * \param apName AutoPatcher name
 *
 * \return Arguments
 *
 * \sa PatchInfo::autoPatcherArgs()
 */
CStringMap * mbp_patchinfo_autopatcher_args(const CPatchInfo *info,
                                            const char *apName)
{
    CCAST(info);
    mbp::PatchInfo::AutoPatcherArgs *args =
            new mbp::PatchInfo::AutoPatcherArgs(pi->autoPatcherArgs(apName));
    return reinterpret_cast<CStringMap *>(args);
}

/*!
 * \brief Whether the patched file has a boot image
 *
 * \param info CPatchInfo object
 *
 * \return Whether the patched file has a boot image
 *
 * \sa PatchInfo::hasBootImage()
 */
bool mbp_patchinfo_has_boot_image(const CPatchInfo *info)
{
    CCAST(info);
    return pi->hasBootImage();
}

/*!
 * \brief Set whether the patched file has a boot image
 *
 * \param info CPatchInfo object
 * \param hasBootImage Has boot image
 *
 * \sa PatchInfo::setHasBootImage()
 */
void mbp_patchinfo_set_has_boot_image(CPatchInfo *info, bool hasBootImage)
{
    CAST(info);
    pi->setHasBootImage(hasBootImage);
}

/*!
 * \brief Whether boot images should be autodetected
 *
 * \param info CPatchInfo object
 *
 * \return Whether boot images should be autodetected
 *
 * \sa PatchInfo::autodetectBootImages()
 */
bool mbp_patchinfo_autodetect_boot_images(const CPatchInfo *info)
{
    CCAST(info);
    return pi->autodetectBootImages();
}

/*!
 * \brief Set whether boot images should be autodetected
 *
 * \param info CPatchInfo object
 * \param autoDetect Autodetect boot images
 *
 * \sa PatchInfo::setAutoDetectBootImages()
 */
void mbp_patchinfo_set_autodetect_boot_images(CPatchInfo *info,
                                              bool autoDetect)
{
    CAST(info);
    pi->setAutoDetectBootImages(autoDetect);
}

/*!
 * \brief List of manually specified boot images
 *
 * \note The returned array should be freed with `mbp_free_array()` when it
 *       is no longer needed.
 *
 * \param info CPatchInfo object
 *
 * \return A NULL-terminated array containing the manually specified boot
 *         images
 *
 * \sa PatchInfo::bootImages()
 */
char ** mbp_patchinfo_boot_images(const CPatchInfo *info)
{
    CCAST(info);
    return vector_to_cstring_array(pi->bootImages());
}

/*!
 * \brief Set list of manually specified boot images
 *
 * \param info CPatchInfo object
 * \param bootImages NULL-terminated list of boot images
 *
 * \sa PatchInfo::setBootImages()
 */
void mbp_patchinfo_set_boot_images(CPatchInfo *info, const char **bootImages)
{
    CAST(info);
    pi->setBootImages(cstring_array_to_vector(bootImages));
}

/*!
 * \brief Which ramdisk patcher to use
 *
 * \note The output data is dynamically allocated. It should be `free()`'d
 *       when it is no longer needed.
 *
 * \param info CPatchInfo object
 *
 * \return Which ramdisk patcher to use
 *
 * \sa PatchInfo::ramdisk()
 */
char * mbp_patchinfo_ramdisk(const CPatchInfo *info)
{
    CCAST(info);
    return string_to_cstring(pi->ramdisk());
}

/*!
 * \brief Set which ramdisk patcher to use
 *
 * \param info CPatchInfo object
 * \param ramdisk Which ramdisk patcher to use
 *
 * \sa PatchInfo::setRamdisk()
 */
void mbp_patchinfo_set_ramdisk(CPatchInfo *info, const char *ramdisk)
{
    CAST(info);
    pi->setRamdisk(ramdisk);
}

}
