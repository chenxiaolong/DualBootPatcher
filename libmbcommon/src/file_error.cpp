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

#include "mbcommon/file_error.h"

#include <string>

namespace mb
{

char FileError::ID = 0;

FileError::FileError(FileErrorType type)
    : _type(type)
{
}

FileError::FileError(FileErrorType type, std::string details)
    : _type(type), _details(std::move(details))
{
}

std::string FileError::message() const
{
    std::string result;

    switch (_type) {
    case FileErrorType::ArgumentOutOfRange:
        result = "argument out of range";
        break;
    case FileErrorType::CannotConvertEncoding:
        result = "cannot convert string encoding";
        break;
    case FileErrorType::InvalidState:
        result = "invalid state";
        break;
    case FileErrorType::ObjectMoved:
        result = "object was moved";
        break;
    case FileErrorType::UnsupportedRead:
        result = "read not supported";
        break;
    case FileErrorType::UnsupportedWrite:
        result = "write not supported";
        break;
    case FileErrorType::UnsupportedSeek:
        result = "seek not supported";
        break;
    case FileErrorType::UnsupportedTruncate:
        result = "truncate not supported";
        break;
    case FileErrorType::IntegerOverflow:
        result = "integer overflowed";
        break;
    default:
        result = "(unknown file error)";
        break;
    }

    if (!_details.empty()) {
        result += ": ";
        result += _details;
    }

    return result;
}

FileErrorType FileError::type() const
{
    return _type;
}

std::string FileError::details() const
{
    return _details;
}

}
