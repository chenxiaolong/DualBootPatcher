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

#include "mbbootimg/format/segment_error_p.h"

#include <string>

namespace mb::bootimg
{

struct SegmentErrorCategory : std::error_category
{
    const char * name() const noexcept override;

    std::string message(int ev) const override;
};

const std::error_category & segment_error_category()
{
    static SegmentErrorCategory c;
    return c;
}

std::error_code make_error_code(SegmentError e)
{
    return {static_cast<int>(e), segment_error_category()};
}

const char * SegmentErrorCategory::name() const noexcept
{
    return "segment_reader_writer";
}

std::string SegmentErrorCategory::message(int ev) const
{
    switch (static_cast<SegmentError>(ev)) {
    case SegmentError::AddEntryInIncorrectState:
        return "adding entry in incorrect state";
    case SegmentError::EntryWouldOverflowOffset:
        return "entry would overflow offset";
    case SegmentError::ReadWouldOverflowInteger:
        return "read would overflow integer";
    case SegmentError::WriteWouldOverflowInteger:
        return "write would overflow integer";
    case SegmentError::InvalidEntrySize:
        return "invalid entry size";
    default:
        return "(unknown segment reader/writer error)";
    }
}

}
