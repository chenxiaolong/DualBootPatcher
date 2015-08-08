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

#include "patchinfo.h"

#include <algorithm>


namespace mbp
{

/*! \cond INTERNAL */
class PatchInfo::Impl
{
public:
    Impl();

    std::string id;

    // The name of the ROM
    std::string name;

    // Regular expressions for matching the filename
    std::vector<std::string> regexes;

    // If the regexes are over-encompassing, these regexes are used to
    // exclude the filename
    std::vector<std::string> excludeRegexes;

    typedef std::vector<std::pair<std::string, PatchInfo::AutoPatcherArgs>> AutoPatcherItems;

    // List of autopatchers to use
    std::vector<std::pair<std::string, PatchInfo::AutoPatcherArgs>> autoPatchers;

    // Whether or not the file contains (a) boot image(s)
    bool hasBootImage;

    // Attempt to autodetect boot images (finds .img files and checks their headers)
    bool autoDetectBootImages;

    // List of manually specified boot images
    std::vector<std::string> bootImages;

    // Ramdisk patcher to use
    std::string ramdiskPatcher;
};
/*! \endcond */


PatchInfo::Impl::Impl()
{
    // Default to having a boot image
    hasBootImage = true;

    // Default to autodetecting boot images in the zip file
    autoDetectBootImages = true;
}

/*!
 * \class PatchInfo
 * \brief Holds information describing how a file should be patched
 *
 * This class holds the bits and pieces indicating which parts of the patcher
 * should be used for patching a file as well as arguments for those components.
 *
 * The following types of regexes are used for matching files:
 *
 * - Regexes for matching a filename
 * - Exclusion regexes (to avoid making the regexes too complicated)
 *
 * The following parameters are used to configure that patcher:
 *
 * - List of AutoPatchers are their arguments
 * - Whether the file contains a boot image
 * - Whether to autodetect the boot images
 * - List of manually specified boot images
 * - Which ramdisk patcher to use
 * - List of supported/unsupported partition configurations
 */

PatchInfo::PatchInfo() : m_impl(new Impl())
{
}

PatchInfo::~PatchInfo()
{
}

/*!
 * \brief PatchInfo identifier
 *
 * This is usually similar to eg.
 * `[Device ID]/(ROMs|Kernels)/[ROM/Kernel Type]/[ROM/Kernel Name]`
 *
 * \return PatchInfo ID
 */
std::string PatchInfo::id() const
{
    return m_impl->id;
}

/*!
 * \brief Set the PatchInfo ID
 *
 * The identifier should be unique inside whichever container is used to store
 * the set of PatchInfo objects.
 *
 * \param id ID
 */
void PatchInfo::setId(std::string id)
{
    m_impl->id = std::move(id);
}

/*!
 * \brief Name of ROM or kernel
 *
 * This is the full name of the ROM or kernel this PatchInfo describes.
 *
 * \return ROM or kernel name
 */
std::string PatchInfo::name() const
{
    return m_impl->name;
}

/*!
 * \brief Set name of ROM or kernel
 *
 * \param name Name of ROM or kernel
 */
void PatchInfo::setName(std::string name)
{
    m_impl->name = std::move(name);
}

/*!
 * \brief List of filename matching regexes
 *
 * \return Regexes
 */
std::vector<std::string> PatchInfo::regexes() const
{
    return m_impl->regexes;
}

/*!
 * \brief Set the list of filename matching regexes
 *
 * \param regexes Regexes
 */
void PatchInfo::setRegexes(std::vector<std::string> regexes)
{
    m_impl->regexes = std::move(regexes);
}

/*!
 * \brief List of filename excluding regexes
 *
 * \return Exclusion regexes
 */
std::vector<std::string> PatchInfo::excludeRegexes() const
{
    return m_impl->excludeRegexes;
}

/*!
 * \brief Set the list of filename excluding regexes
 *
 * \param regexes Exclusion regexes
 */
void PatchInfo::setExcludeRegexes(std::vector<std::string> regexes)
{
    m_impl->excludeRegexes = std::move(regexes);
}

/*! \cond INTERNAL */
struct ByKey
{
    ByKey(std::string key) : mKey(key) {
    }

    bool operator()(const std::pair<std::string, PatchInfo::AutoPatcherArgs> &elem) const {
        return mKey == elem.first;
    }
private:
    std::string mKey;
};
/*! \endcond */

/*!
 * \brief Add AutoPatcher to PatchInfo
 *
 * \param apName AutoPatcher name
 * \param args AutoPatcher arguments
 */
void PatchInfo::addAutoPatcher(const std::string &apName,
                               PatchInfo::AutoPatcherArgs args)
{
    m_impl->autoPatchers.push_back(
            std::make_pair(apName, std::move(args)));
}

/*!
 * \brief Remove AutoPatcher from PatchInfo
 *
 * \param apName AutoPatcher name
 */
void PatchInfo::removeAutoPatcher(const std::string &apName)
{
    auto it = std::find_if(m_impl->autoPatchers.begin(),
                           m_impl->autoPatchers.end(),
                           ByKey(apName));
    if (it != m_impl->autoPatchers.end()) {
        m_impl->autoPatchers.erase(it);
    }
}

/*!
 * \brief Get list of AutoPatcher names
 *
 * \return List of AutoPatcher names
 */
std::vector<std::string> PatchInfo::autoPatchers() const
{
    std::vector<std::string> items;

    for (auto const &ap : m_impl->autoPatchers) {
        items.push_back(ap.first);
    }

    return items;
}

/*!
 * \brief Get AutoPatcher arguments
 *
 * \param apName AutoPatcher name
 *
 * \return Arguments (empty if AutoPatcher does not exist or if there are no
 *         arguments)
 */
PatchInfo::AutoPatcherArgs PatchInfo::autoPatcherArgs(const std::string &apName) const
{
    auto &items = m_impl->autoPatchers;
    auto it = std::find_if(items.begin(), items.end(), ByKey(apName));
    if (it != items.end()) {
        return it->second;
    }

    return AutoPatcherArgs();
}

/*!
 * \brief Whether the patched file has a boot image
 *
 * \return Whether the patched file has a boot image
 */
bool PatchInfo::hasBootImage() const
{
    return m_impl->hasBootImage;
}

/*!
 * \brief Set whether the patched file has a boot image
 *
 * \param hasBootImage Has boot image
 */
void PatchInfo::setHasBootImage(bool hasBootImage)
{
    m_impl->hasBootImage = hasBootImage;
}

/*!
 * \brief Whether boot images should be autodetected
 *
 * \return Whether boot images should be autodetected
 */
bool PatchInfo::autodetectBootImages() const
{
    return m_impl->autoDetectBootImages;
}

/*!
 * \brief Set whether boot images should be autodetected
 *
 * \param autoDetect Autodetect boot images
 */
void PatchInfo::setAutoDetectBootImages(bool autoDetect)
{
    m_impl->autoDetectBootImages = autoDetect;
}

/*!
 * \brief List of manually specified boot images
 *
 * \return List of manually specified boot images
 */
std::vector<std::string> PatchInfo::bootImages() const
{
    return m_impl->bootImages;
}

/*!
 * \brief Set list of manually specified boot images
 *
 * \param bootImages List of boot images
 */
void PatchInfo::setBootImages(std::vector<std::string> bootImages)
{
    m_impl->bootImages = std::move(bootImages);
}

/*!
 * \brief Which ramdisk patcher to use
 *
 * \return Which ramdisk patcher to use
 */
std::string PatchInfo::ramdisk() const
{
    return m_impl->ramdiskPatcher;
}

/*!
 * \brief Set which ramdisk patcher to use
 *
 * \param ramdisk Which ramdisk patcher to use
 */
void PatchInfo::setRamdisk(std::string ramdisk)
{
    m_impl->ramdiskPatcher = std::move(ramdisk);
}

}
