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

#include "mbsparse/sparse_error.h"

#include <string>

namespace mb
{

char SparseFileError::ID = 0;

SparseFileError::SparseFileError(SparseFileErrorType type)
    : _type(type)
{
}

SparseFileError::SparseFileError(SparseFileErrorType type, std::string details)
    : _type(type), _details(std::move(details))
{
}

std::string SparseFileError::message() const
{
    std::string result;

    switch (_type) {
    case SparseFileErrorType::UnexpectedEndOfFile:
        result = "unexpected end of file";
        break;
    case SparseFileErrorType::ReadError:
        result = "failed to read underlying file";
        break;
    case SparseFileErrorType::SeekError:
        result = "failed to seek underlying file";
        break;
    case SparseFileErrorType::InvalidSparseMagic:
        result = "invalid sparse header magic";
        break;
    case SparseFileErrorType::InvalidSparseMajorVersion:
        result = "invalid sparse header major version";
        break;
    case SparseFileErrorType::InvalidSparseHeaderSize:
        result = "invalid sparse header size";
        break;
    case SparseFileErrorType::InvalidChunkHeaderSize:
        result = "invalid chunk header size";
        break;
    case SparseFileErrorType::InvalidChunkSize:
        result = "invalid chunk size";
        break;
    case SparseFileErrorType::InvalidChunkType:
        result = "invalid chunk type";
        break;
    case SparseFileErrorType::InvalidChunkBounds:
        result = "invalid chunk bounds";
        break;
    case SparseFileErrorType::InvalidRawChunk:
        result = "invalid 'raw' chunk";
        break;
    case SparseFileErrorType::InvalidFillChunk:
        result = "invalid 'fill' chunk";
        break;
    case SparseFileErrorType::InvalidSkipChunk:
        result = "invalid 'skip' chunk";
        break;
    case SparseFileErrorType::InvalidCrc32Chunk:
        result = "invalid 'crc32' chunk";
        break;
    case SparseFileErrorType::InternalError:
        result = "(internal error)";
        break;
    default:
        result = "(unknown sparse file error)";
        break;
    }

    if (!_details.empty()) {
        result += ": ";
        result += _details;
    }

    return result;
}

SparseFileErrorType SparseFileError::type() const
{
    return _type;
}

std::string SparseFileError::details() const
{
    return _details;
}

}
