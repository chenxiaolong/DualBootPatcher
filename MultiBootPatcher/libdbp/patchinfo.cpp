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

#include "patchinfo.h"

#include <unordered_map>

#include <boost/filesystem/path.hpp>
#include <boost/regex.hpp>

#include "patcherpaths.h"


/*! \cond INTERNAL */
class PatchInfo::Impl
{
public:
    typedef std::unordered_map<std::string, AutoPatcherArgs> AutoPatcherItems;

    Impl();

    std::string id;

    // The name of the ROM
    std::string name;

    // Regular expressions for matching the filename
    std::vector<std::string> regexes;

    // If the regexes are over-encompassing, these regexes are used to
    // exclude the filename
    std::vector<std::string> excludeRegexes;

    // List of regexes used for conditionals
    std::vector<std::string> condRegexes;

    // Whether the patchinfo has a <not-matched> elements (acts as the
    // "else" statement for the conditionals)
    bool hasNotMatchedElement;

    // ** For the variables below, use hashmap[Default] to get the default
    //    values and hashmap[regex] with the index in condRegexes to get
    //    the overridden values.
    //
    // NOTE: If the variable is a list, the "overridden" values are used
    //       in addition to the default values

    // List of autopatchers to use
    std::unordered_map<std::string, AutoPatcherItems> autoPatchers;

    // Whether or not the file contains (a) boot image(s)
    std::unordered_map<std::string, bool> hasBootImage;

    // Attempt to autodetect boot images (finds .img files and checks their headers)
    std::unordered_map<std::string, bool> autodetectBootImages;

    // List of manually specified boot images
    std::unordered_map<std::string, std::vector<std::string>> bootImages;

    // Ramdisk patcher to use
    std::unordered_map<std::string, std::string> ramdisk;

    // Name of alternative binary to use
    std::unordered_map<std::string, std::string> patchedInit;

    // Whether or not device checks/asserts should be kept
    std::unordered_map<std::string, bool> deviceCheck;

    // List of supported partition configurations
    std::unordered_map<std::string, std::vector<std::string>> supportedConfigs;
};
/*! \endcond */


/*! \brief Key for getting the default values */
const std::string PatchInfo::Default = "default";
/*! \brief Key for getting the `<not-matched>` values */
const std::string PatchInfo::NotMatched = "not-matched";

PatchInfo::Impl::Impl()
{
    // No <not-matched> element
    hasNotMatchedElement = false;

    // Default to having a boot image
    hasBootImage[PatchInfo::Default] = true;

    // Default to autodetecting boot images in the zip file
    autodetectBootImages[PatchInfo::Default] = true;

    // Don't remove device checks
    deviceCheck[PatchInfo::Default] = true;

    // Allow all configs
    supportedConfigs[PatchInfo::Default].push_back("all");
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
 * The following parameter keys are used for configuring the patching process
 * for a particular group of files:
 *
 * - Default: Applies to all matched files
 * - Conditional regexes: Applies to the subset of files that match these
 *   conditional regexes
 * - Not matched: Applies to files that don't match the conditional regexes
 *
 * The following parameters are used to configure that patcher:
 *
 * - List of AutoPatchers are their arguments
 * - Whether the file contains a boot image
 * - Whether to autodetect the boot images
 * - List of manually specified boot images
 * - Which ramdisk patcher to use
 * - Which patched init binary to use (if needed)
 * - Whether device model asserts in the updater-script file should be nullified
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
 * `[Device codename]/(ROMs|Kernels)/[ROM/Kernel Type]/[ROM/Kernel Name]`
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
 * \brief Get the parameter key for a filename
 *
 * This returns the corresponding conditional regex if one matches the filename.
 * Otherwise, it will return PatchInfo::NotMatched if the PatchInfo has a
 * <not-matched> element. If it doesn't have a <not-matched> element,
 * PatchInfo::Default is returned.
 *
 * \param fileName Filename
 *
 * \return Conditional regex OR PatchInfo::NotMatched OR PatchInfo::Default
 */
std::string PatchInfo::keyFromFilename(const std::string &fileName) const
{
    std::string noPath = boost::filesystem::path(fileName).filename().string();

    // The conditional regex is the key if <matches> elements are used
    // in the patchinfo xml files
    for (auto &regex : m_impl->condRegexes) {
        if (boost::regex_search(noPath, boost::regex(regex))) {
            return regex;
        }
    }

    // If none of those are matched, try <not-matched>
    if (m_impl->hasNotMatchedElement) {
        return NotMatched;
    }

    // Otherwise, use the defaults
    return Default;
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

/*!
 * \brief List of conditional regexes for parameters
 *
 * \return Conditional regexes
 */
std::vector<std::string> PatchInfo::condRegexes() const
{
    return m_impl->condRegexes;
}

/*!
 * \brief Set the list of conditional regexes
 *
 * \param regexes Conditional regexes
 */
void PatchInfo::setCondRegexes(std::vector<std::string> regexes)
{
    m_impl->condRegexes = std::move(regexes);
}

/*!
 * \brief Check if the PatchInfo has a <not-matched> element
 *
 * \return Whether the PatchInfo has a <not-matched> element
 */
bool PatchInfo::hasNotMatched() const
{
    return m_impl->hasNotMatchedElement;
}

/*!
 * \brief Set whether the PatchInfo has a <not-matched> element
 *
 * \param hasElem Has not matched element
 */
void PatchInfo::setHasNotMatched(bool hasElem)
{
    m_impl->hasNotMatchedElement = hasElem;
}

/*!
 * \brief Add AutoPatcher to PatchInfo
 *
 * \param key Parameter key
 * \param apName AutoPatcher name
 * \param args AutoPatcher arguments
 */
void PatchInfo::addAutoPatcher(const std::string &key,
                               const std::string &apName,
                               PatchInfo::AutoPatcherArgs args)
{
    Impl::AutoPatcherItems &items = m_impl->autoPatchers[key];
    items[apName] = std::move(args);
}

/*!
 * \brief Remove AutoPatcher from PatchInfo
 *
 * \param key Parameter key
 * \param apName AutoPatcher name
 */
void PatchInfo::removeAutoPatcher(const std::string &key,
                                  const std::string &apName)
{
    if (m_impl->autoPatchers.find(key) == m_impl->autoPatchers.end()) {
        return;
    }

    Impl::AutoPatcherItems &items = m_impl->autoPatchers[key];
    auto it = items.find(apName);
    if (it != items.end()) {
        items.erase(it);
    }
}

/*!
 * \brief Get list of AutoPatcher names
 *
 * \param key Parameter key
 *
 * \return List of AutoPatcher names
 */
std::vector<std::string> PatchInfo::autoPatchers(const std::string &key) const
{
    bool hasKey = m_impl->autoPatchers.find(key) != m_impl->autoPatchers.end();
    bool hasDefault = m_impl->autoPatchers.find(Default) != m_impl->autoPatchers.end();

    std::vector<std::string> items;

    if (hasDefault) {
        auto &aps = m_impl->autoPatchers[Default];
        for (auto const &ap : aps) {
            items.push_back(ap.first);
        }
    }

    if (key != Default && hasKey) {
        auto &aps = m_impl->autoPatchers[key];
        for (auto const &ap : aps) {
            items.push_back(ap.first);
        }
    }

    return items;
}

/*!
 * \brief Get AutoPatcher arguments
 *
 * \param key Parameter key
 * \param apName AutoPatcher name
 *
 * \return Arguments (empty if AutoPatcher does not exist or if there are no
 *         arguments)
 */
PatchInfo::AutoPatcherArgs PatchInfo::autoPatcherArgs(const std::string &key,
                                                      const std::string &apName) const
{
    if (m_impl->autoPatchers.find(key) != m_impl->autoPatchers.end()) {
        Impl::AutoPatcherItems &items = m_impl->autoPatchers[key];
        auto it = items.find(apName);
        if (it != items.end()) {
            return it->second;
        }
    }

    return AutoPatcherArgs();
}

/*!
 * \brief Whether the patched file has a boot image
 *
 * \param key Parameter key
 *
 * \return Whether the patched file has a boot image
 */
bool PatchInfo::hasBootImage(const std::string &key) const
{
    if (m_impl->hasBootImage.find(key) != m_impl->hasBootImage.end()) {
        return m_impl->hasBootImage[key];
    }

    return bool();
}

/*!
 * \brief Set whether the patched file has a boot image
 *
 * \param key Parameter key
 * \param hasBootImage Has boot image
 */
void PatchInfo::setHasBootImage(const std::string &key, bool hasBootImage)
{
    m_impl->hasBootImage[key] = hasBootImage;
}

/*!
 * \brief Whether boot images should be autodetected
 *
 * \param key Parameter key
 *
 * \return Whether boot images should be autodetected
 */
bool PatchInfo::autodetectBootImages(const std::string &key) const
{
    if (m_impl->autodetectBootImages.find(key)
            != m_impl->autodetectBootImages.end()) {
        return m_impl->autodetectBootImages[key];
    }

    return bool();
}

/*!
 * \brief Set whether boot images should be autodetected
 *
 * \param key Parameter key
 * \param autoDetect Autodetect boot images
 */
void PatchInfo::setAutoDetectBootImages(const std::string &key, bool autoDetect)
{
    m_impl->autodetectBootImages[key] = autoDetect;
}

/*!
 * \brief List of manually specified boot images
 *
 * \param key Parameter key
 *
 * \return List of manually specified boot images
 */
std::vector<std::string> PatchInfo::bootImages(const std::string &key) const
{
    bool hasKey = m_impl->bootImages.find(key) != m_impl->bootImages.end();
    bool hasDefault = m_impl->bootImages.find(Default) != m_impl->bootImages.end();

    std::vector<std::string> items;

    if (hasDefault) {
        auto &imgs = m_impl->bootImages[Default];
        items.insert(items.end(), imgs.begin(), imgs.end());
    }

    if (key != Default && hasKey) {
        auto &imgs = m_impl->bootImages[key];
        items.insert(items.end(), imgs.begin(), imgs.end());
    }

    return items;
}

/*!
 * \brief Set list of manually specified boot images
 *
 * \param key Parameter key
 * \param bootImages List of boot images
 */
void PatchInfo::setBootImages(const std::string &key,
                              std::vector<std::string> bootImages)
{
    m_impl->bootImages[key] = std::move(bootImages);
}

/*!
 * \brief Which ramdisk patcher to use
 *
 * \param key Parameter key
 *
 * \return Which ramdisk patcher to use
 */
std::string PatchInfo::ramdisk(const std::string &key) const
{
    if (m_impl->ramdisk.find(key) != m_impl->ramdisk.end()) {
        return m_impl->ramdisk[key];
    }

    return std::string();
}

/*!
 * \brief Set which ramdisk patcher to use
 *
 * \param key Parameter key
 * \param ramdisk Which ramdisk patcher to use
 */
void PatchInfo::setRamdisk(const std::string &key, std::string ramdisk)
{
    m_impl->ramdisk[key] = std::move(ramdisk);
}

/*!
 * \brief Which patched init binary to use
 *
 * \param key Parameter key
 *
 * \return Which patched init binary to use or empty string if no patched init
 *         binary should be used
 */
std::string PatchInfo::patchedInit(const std::string &key) const
{
    if (m_impl->patchedInit.find(key) != m_impl->patchedInit.end()) {
        return m_impl->patchedInit[key];
    }

    return std::string();
}

/*!
 * \brief Set which patched init binary to use
 *
 * \param key Parameter key
 * \param init Patched init binary
 */
void PatchInfo::setPatchedInit(const std::string &key, std::string init)
{
    m_impl->patchedInit[key] = std::move(init);
}

/*!
 * \brief Whether device model checks should be kept
 *
 * \param key Parameter key
 *
 * \return Whether device model checks should be kept
 */
bool PatchInfo::deviceCheck(const std::string &key) const
{
    if (m_impl->deviceCheck.find(key) != m_impl->deviceCheck.end()) {
        return m_impl->deviceCheck[key];
    }

    return bool();
}

/*!
 * \brief Set whether device model checks should be kept
 *
 * \param key Parameter key
 * \param deviceCheck Keep device model checks
 */
void PatchInfo::setDeviceCheck(const std::string &key, bool deviceCheck)
{
    m_impl->deviceCheck[key] = deviceCheck;
}

/*!
 * \brief List of supported partition configurations
 *
 * \param key Parameter key
 *
 * \return List of supported PartitionConfig IDs
 */
std::vector<std::string> PatchInfo::supportedConfigs(const std::string &key) const
{
    bool hasKey = m_impl->supportedConfigs.find(key) != m_impl->supportedConfigs.end();
    bool hasDefault = m_impl->supportedConfigs.find(Default) != m_impl->supportedConfigs.end();

    std::vector<std::string> items;

    if (hasDefault) {
        auto &configs = m_impl->supportedConfigs[Default];
        items.insert(items.end(), configs.begin(), configs.end());
    }

    if (key != Default && hasKey) {
        auto &configs = m_impl->supportedConfigs[key];
        items.insert(items.end(), configs.begin(), configs.end());
    }

    return items;
}

/*!
 * \brief Set list of supported partition configurations
 *
 * \param key Parameter key
 * \param configs List of supported PartitionConfig IDs
 */
void PatchInfo::setSupportedConfigs(const std::string &key,
                                    std::vector<std::string> configs)
{
    m_impl->supportedConfigs[key] = std::move(configs);
}
