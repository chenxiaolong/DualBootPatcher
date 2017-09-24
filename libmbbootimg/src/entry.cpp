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

#include "mbbootimg/entry_p.h"


namespace mb
{
namespace bootimg
{

Entry::Entry()
    : _priv_ptr(new EntryPrivate())
{
}

Entry::Entry(const Entry &entry)
    : _priv_ptr(new EntryPrivate(*entry._priv_ptr))
{
}

Entry::Entry(Entry &&entry) noexcept
    : Entry()
{
    _priv_ptr.swap(entry._priv_ptr);
}

Entry::~Entry() = default;

Entry & Entry::operator=(const Entry &entry)
{
    *_priv_ptr = EntryPrivate(*entry._priv_ptr);
    return *this;
}

Entry & Entry::operator=(Entry &&entry) noexcept
{
    _priv_ptr.swap(entry._priv_ptr);
    *entry._priv_ptr = EntryPrivate();
    return *this;
}

bool Entry::operator==(const Entry &rhs) const
{
    auto const &field1 = _priv_ptr->field;
    auto const &field2 = rhs._priv_ptr->field;

    return field1.type == field2.type
            && field1.name == field2.name
            && field1.size == field2.size;
}

bool Entry::operator!=(const Entry &rhs) const
{
    return !(*this == rhs);
}

void Entry::clear()
{
    MB_PRIVATE(Entry);
    *priv = EntryPrivate();
}

// Fields

optional<int> Entry::type() const
{
    MB_PRIVATE(const Entry);
    return priv->field.type;
}

void Entry::set_type(optional<int> type)
{
    MB_PRIVATE(Entry);
    priv->field.type = std::move(type);
}

optional<std::string> Entry::name() const
{
    MB_PRIVATE(const Entry);
    return priv->field.name;
}

void Entry::set_name(optional<std::string> name)
{
    MB_PRIVATE(Entry);
    priv->field.name = std::move(name);
}

optional<uint64_t> Entry::size() const
{
    MB_PRIVATE(const Entry);
    return priv->field.size;
}

void Entry::set_size(optional<uint64_t> size)
{
    MB_PRIVATE(Entry);
    priv->field.size = std::move(size);

}

}
}
