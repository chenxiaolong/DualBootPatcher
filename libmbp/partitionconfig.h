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

#pragma once

#include <memory>
#include <vector>

#include "libmbp_global.h"


class MBP_EXPORT PartitionConfig
{
public:
    PartitionConfig();
    ~PartitionConfig();

    std::string name() const;
    void setName(std::string name);

    std::string description() const;
    void setDescription(std::string description);

    std::string kernel() const;
    void setKernel(std::string kernel);

    std::string id() const;
    void setId(std::string id);

    std::string targetSystem() const;
    void setTargetSystem(std::string path);

    std::string targetCache() const;
    void setTargetCache(std::string path);

    std::string targetData() const;
    void setTargetData(std::string path);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
