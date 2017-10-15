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

#include "mbcommon/error/error_info.h"

namespace mb
{

enum class SparseFileErrorType
{
    UnexpectedEndOfFile         = 10,
    ReadError                   = 11,
    SeekError                   = 12,

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

class MB_EXPORT SparseFileError : public ErrorInfo<SparseFileError>
{
public:
    static char ID;

    SparseFileError(SparseFileErrorType type);
    SparseFileError(SparseFileErrorType type, std::string details);

    std::string message() const override;

    SparseFileErrorType type() const;
    std::string details() const;

private:
    SparseFileErrorType _type;
    std::string _details;
};

}
