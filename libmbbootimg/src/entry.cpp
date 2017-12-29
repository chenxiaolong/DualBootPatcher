/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mbbootimg/entry.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>


namespace mb::bootimg
{

Entry::Entry() = default;

Entry::~Entry() = default;

bool Entry::operator==(const Entry &rhs) const
{
    return m_type == rhs.m_type
            && m_name == rhs.m_name
            && m_size == rhs.m_size;
}

bool Entry::operator!=(const Entry &rhs) const
{
    return !(*this == rhs);
}

void Entry::clear()
{
    m_type = {};
    m_name = {};
    m_size = {};
}

// Fields

std::optional<int> Entry::type() const
{
    return m_type;
}

void Entry::set_type(std::optional<int> type)
{
    m_type = std::move(type);
}

std::optional<std::string> Entry::name() const
{
    return m_name;
}

void Entry::set_name(std::optional<std::string> name)
{
    m_name = std::move(name);
}

std::optional<uint64_t> Entry::size() const
{
    return m_size;
}

void Entry::set_size(std::optional<uint64_t> size)
{
    m_size = std::move(size);

}

}
