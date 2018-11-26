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

#include "mbbootimg/writer_error.h"

#include <string>

namespace mb::bootimg
{

struct WriterErrorCategory : std::error_category
{
    const char * name() const noexcept override;

    std::string message(int ev) const override;
};

const std::error_category & writer_error_category()
{
    static WriterErrorCategory c;
    return c;
}

std::error_code make_error_code(WriterError e)
{
    return {static_cast<int>(e), writer_error_category()};
}

const char * WriterErrorCategory::name() const noexcept
{
    return "writer";
}

std::string WriterErrorCategory::message(int ev) const
{
    switch (static_cast<WriterError>(ev)) {
    case WriterError::InvalidState:
        return "invalid state";
    case WriterError::UnknownOption:
        return "unknown option";
    case WriterError::NoFormatRegistered:
        return "no format registered";
    case WriterError::EndOfEntries:
        return "end of entries";
    default:
        return "(unknown writer error)";
    }
}

}
