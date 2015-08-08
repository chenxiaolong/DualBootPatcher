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

#pragma once

#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "libmbp_global.h"

#include "device.h"


namespace mbp
{

class MBP_EXPORT PatchInfo
{
public:
    explicit PatchInfo();
    ~PatchInfo();

    typedef std::unordered_map<std::string, std::string> AutoPatcherArgs;

    std::string id() const;
    void setId(std::string id);

    std::string name() const;
    void setName(std::string name);

    std::vector<std::string> regexes() const;
    void setRegexes(std::vector<std::string> regexes);

    std::vector<std::string> excludeRegexes() const;
    void setExcludeRegexes(std::vector<std::string> regexes);

    std::vector<std::string> condRegexes() const;
    void setCondRegexes(std::vector<std::string> regexes);

    // ** For the variables below, use hashmap[Default] to get the default
    //    values and hashmap[regex] with the index in condRegexes to get
    //    the overridden values.
    //
    // NOTE: If the variable is a list, the "overridden" values are used
    //       in addition to the default values

    void addAutoPatcher(const std::string &apName, AutoPatcherArgs args);
    void removeAutoPatcher(const std::string &apName);
    std::vector<std::string> autoPatchers() const;
    AutoPatcherArgs autoPatcherArgs(const std::string &apName) const;

    bool hasBootImage() const;
    void setHasBootImage(bool hasBootImage);

    bool autodetectBootImages() const;
    void setAutoDetectBootImages(bool autoDetect);

    std::vector<std::string> bootImages() const;
    void setBootImages(std::vector<std::string> bootImages);

    std::string ramdisk() const;
    void setRamdisk(std::string ramdisk);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}
