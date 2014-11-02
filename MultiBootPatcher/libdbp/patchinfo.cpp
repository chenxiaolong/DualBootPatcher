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


class PatchInfo::Impl
{
public:
    Impl();

    std::string path;

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
    std::unordered_map<std::string, PatchInfo::AutoPatcherItems> autoPatchers;

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


const std::string PatchInfo::Default = "default";
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

// --------------------------------

PatchInfo::PatchInfo() : m_impl(new Impl())
{
}

PatchInfo::~PatchInfo()
{
}

std::string PatchInfo::path() const
{
    return m_impl->path;
}

void PatchInfo::setPath(std::string path)
{
    m_impl->path = std::move(path);
}

std::string PatchInfo::name() const
{
    return m_impl->name;
}

void PatchInfo::setName(std::string name)
{
    m_impl->name = std::move(name);
}

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

std::vector<std::string> PatchInfo::regexes() const
{
    return m_impl->regexes;
}

void PatchInfo::setRegexes(std::vector<std::string> regexes)
{
    m_impl->regexes = std::move(regexes);
}

std::vector<std::string> PatchInfo::excludeRegexes() const
{
    return m_impl->excludeRegexes;
}

void PatchInfo::setExcludeRegexes(std::vector<std::string> regexes)
{
    m_impl->excludeRegexes = std::move(regexes);
}

std::vector<std::string> PatchInfo::condRegexes() const
{
    return m_impl->condRegexes;
}

void PatchInfo::setCondRegexes(std::vector<std::string> regexes)
{
    m_impl->condRegexes = std::move(regexes);
}

bool PatchInfo::hasNotMatched() const
{
    return m_impl->hasNotMatchedElement;
}

void PatchInfo::setHasNotMatched(bool hasElem)
{
    m_impl->hasNotMatchedElement = hasElem;
}

PatchInfo::AutoPatcherItems PatchInfo::autoPatchers(const std::string &key) const
{
    bool hasKey = m_impl->autoPatchers.find(key) != m_impl->autoPatchers.end();
    bool hasDefault = m_impl->autoPatchers.find(Default) != m_impl->autoPatchers.end();

    AutoPatcherItems items;

    if (hasDefault) {
        auto &aps = m_impl->autoPatchers[Default];
        items.insert(items.end(), aps.begin(), aps.end());
    }

    if (key != Default && hasKey) {
        auto &aps = m_impl->autoPatchers[key];
        items.insert(items.end(), aps.begin(), aps.end());
    }

    return items;
}

void PatchInfo::setAutoPatchers(const std::string &key,
                                AutoPatcherItems autoPatchers)
{
    m_impl->autoPatchers[key] = std::move(autoPatchers);
}

bool PatchInfo::hasBootImage(const std::string &key) const
{
    if (m_impl->hasBootImage.find(key) != m_impl->hasBootImage.end()) {
        return m_impl->hasBootImage[key];
    }

    return bool();
}

void PatchInfo::setHasBootImage(const std::string &key, bool hasBootImage)
{
    m_impl->hasBootImage[key] = hasBootImage;
}

bool PatchInfo::autodetectBootImages(const std::string &key) const
{
    if (m_impl->autodetectBootImages.find(key)
            != m_impl->autodetectBootImages.end()) {
        return m_impl->autodetectBootImages[key];
    }

    return bool();
}

void PatchInfo::setAutoDetectBootImages(const std::string &key, bool autoDetect)
{
    m_impl->autodetectBootImages[key] = autoDetect;
}

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

void PatchInfo::setBootImages(const std::string &key,
                              std::vector<std::string> bootImages)
{
    m_impl->bootImages[key] = std::move(bootImages);
}

std::string PatchInfo::ramdisk(const std::string &key) const
{
    if (m_impl->ramdisk.find(key) != m_impl->ramdisk.end()) {
        return m_impl->ramdisk[key];
    }

    return std::string();
}

void PatchInfo::setRamdisk(const std::string &key, std::string ramdisk)
{
    m_impl->ramdisk[key] = std::move(ramdisk);
}

std::string PatchInfo::patchedInit(const std::string &key) const
{
    if (m_impl->patchedInit.find(key) != m_impl->patchedInit.end()) {
        return m_impl->patchedInit[key];
    }

    return std::string();
}

void PatchInfo::setPatchedInit(const std::string &key, std::string init)
{
    m_impl->patchedInit[key] = std::move(init);
}

bool PatchInfo::deviceCheck(const std::string &key) const
{
    if (m_impl->deviceCheck.find(key) != m_impl->deviceCheck.end()) {
        return m_impl->deviceCheck[key];
    }

    return bool();
}

void PatchInfo::setDeviceCheck(const std::string &key, bool deviceCheck)
{
    m_impl->deviceCheck[key] = deviceCheck;
}

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

void PatchInfo::setSupportedConfigs(const std::string &key,
                                    std::vector<std::string> configs)
{
    m_impl->supportedConfigs[key] = std::move(configs);
}

PatchInfo * PatchInfo::findMatchingPatchInfo(PatcherPaths *pp,
                                             Device *device,
                                             const std::string &filename)
{
    if (device == nullptr) {
        return nullptr;
    }

    if (filename.empty()) {
        return nullptr;
    }

    std::string noPath = boost::filesystem::path(filename).filename().string();

    for (PatchInfo *info : pp->patchInfos(device)) {
        for (auto const &regex : info->regexes()) {
            if (boost::regex_search(noPath, boost::regex(regex))) {
                bool skipCurInfo = false;

                // If the regex matches, make sure the filename isn't matched
                // by one of the exclusion regexes
                for (auto const &excludeRegex : info->excludeRegexes()) {
                    if (boost::regex_search(noPath, boost::regex(excludeRegex))) {
                        skipCurInfo = true;
                        break;
                    }
                }

                if (skipCurInfo) {
                    break;
                }

                return info;
            }
        }
    }

    return nullptr;
}
