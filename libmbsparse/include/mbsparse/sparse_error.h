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

#pragma once

#include "mbcommon/common.h"

#include <system_error>

namespace mb::sparse
{

enum class SparseFileError
{
    // Sparse header errors
    InvalidSparseMagic          = 20,
    InvalidSparseMajorVersion   = 21,
    InvalidSparseHeaderSize     = 22,
    InvalidChunkHeaderSize      = 23,

    // Chunk errors
    InvalidChunkSize            = 30,
    InvalidChunkType            = 31,
    InvalidChunkBounds          = 32,
    InvalidRawChunk             = 33,
    InvalidFillChunk            = 34,
    InvalidSkipChunk            = 35,
    InvalidCrc32Chunk           = 36,

    InternalError               = 40,
};

MB_EXPORT std::error_code make_error_code(SparseFileError e);

MB_EXPORT const std::error_category & sparse_file_error_category();

}

namespace std
{
    template<>
    struct MB_EXPORT is_error_code_enum<mb::sparse::SparseFileError> : true_type
    {
    };
}
