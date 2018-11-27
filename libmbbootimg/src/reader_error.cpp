/*
 * Copyright (C) 2017-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbbootimg/reader_error.h"

#include <string>

namespace mb::bootimg
{

struct ReaderErrorCategory : std::error_category
{
    const char * name() const noexcept override;

    std::string message(int ev) const override;
};

const std::error_category & reader_error_category()
{
    static ReaderErrorCategory c;
    return c;
}

std::error_code make_error_code(ReaderError e)
{
    return {static_cast<int>(e), reader_error_category()};
}

const char * ReaderErrorCategory::name() const noexcept
{
    return "reader";
}

std::string ReaderErrorCategory::message(int ev) const
{
    switch (static_cast<ReaderError>(ev)) {
    case ReaderError::InvalidState:
        return "invalid state";
    case ReaderError::UnknownOption:
        return "unknown option";
    case ReaderError::NoFormatsRegistered:
        return "no formats registered";
    case ReaderError::UnknownFileFormat:
        return "unknown file format";
    case ReaderError::EndOfEntries:
        return "end of entries";
    case ReaderError::UnsupportedGoTo:
        return "go to entry not supported";
    default:
        return "(unknown reader error)";
    }
}

}
