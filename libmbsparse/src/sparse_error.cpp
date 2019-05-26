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

namespace mb::sparse
{

struct SparseFileErrorCategory : std::error_category
{
    const char * name() const noexcept override;

    std::string message(int ev) const override;
};

const char * SparseFileErrorCategory::name() const noexcept
{
    return "sparse_file_error";
}

std::string SparseFileErrorCategory::message(int ev) const
{
    switch (static_cast<SparseFileError>(ev)) {
    case SparseFileError::InvalidSparseMagic:
        return "invalid sparse header magic";
    case SparseFileError::InvalidSparseMajorVersion:
        return "invalid sparse header major version";
    case SparseFileError::InvalidSparseHeaderSize:
        return "invalid sparse header size";
    case SparseFileError::InvalidChunkHeaderSize:
        return "invalid chunk header size";
    case SparseFileError::InvalidChunkSize:
        return "invalid chunk size";
    case SparseFileError::InvalidChunkType:
        return "invalid chunk type";
    case SparseFileError::InvalidChunkBounds:
        return "invalid chunk bounds";
    case SparseFileError::InvalidRawChunk:
        return "invalid 'raw' chunk";
    case SparseFileError::InvalidFillChunk:
        return "invalid 'fill' chunk";
    case SparseFileError::InvalidSkipChunk:
        return "invalid 'skip' chunk";
    case SparseFileError::InvalidCrc32Chunk:
        return "invalid 'crc32' chunk";
    case SparseFileError::InternalError:
        return "(internal error)";
    default:
        return "(unknown sparse file error)";
    }
}

const std::error_category & sparse_file_error_category()
{
    static SparseFileErrorCategory c;
    return c;
}

std::error_code make_error_code(SparseFileError e)
{
    return {static_cast<int>(e), sparse_file_error_category()};
}

}
