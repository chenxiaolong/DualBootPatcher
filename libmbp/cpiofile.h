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
#include <vector>

#include "libmbp_global.h"
#include "patchererror.h"


namespace mbp
{

class MBP_EXPORT CpioFile
{
public:
    CpioFile();
    ~CpioFile();

    PatcherError error() const;

    bool load(const std::vector<unsigned char> &data);
    bool createData(std::vector<unsigned char> *dataOut);

    bool exists(const std::string &name) const;
    bool remove(const std::string &name);

    std::vector<std::string> filenames() const;

    // File contents

    bool contents(const std::string &name,
                  std::vector<unsigned char> *dataOut) const;
    bool setContents(const std::string &name,
                     std::vector<unsigned char> data);

    // Adding new files

    bool addSymlink(const std::string &source, const std::string &target);
    bool addFile(const std::string &path, const std::string &name,
                 unsigned int perms);
    bool addFile(std::vector<unsigned char> contents,
                 const std::string &name, unsigned int perms);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}