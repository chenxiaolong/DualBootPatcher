/*
 * Copyright (C) 2014-2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#ifdef __cplusplus
namespace mbp
{
#endif

#ifdef __cplusplus
enum class ErrorCode : int
{
#else
enum ErrorCode
{
#endif
    // Generic
    NoError = 0,
    MemoryAllocationError = 1,

    // PatcherCreation
    PatcherCreateError = 50,
    AutoPatcherCreateError = 51,

    // I/O
    FileOpenError = 100,
    FileCloseError = 101,
    FileReadError = 102,
    FileWriteError = 103,
    FileSeekError = 104,
    FileTellError = 105,

    // Archive
    ArchiveReadOpenError = 200,
    ArchiveReadDataError = 201,
    ArchiveReadHeaderError = 202,
    ArchiveWriteOpenError = 210,
    ArchiveWriteDataError = 211,
    ArchiveWriteHeaderError = 212,
    ArchiveCloseError = 220,
    ArchiveFreeError = 221,

    // Cancelled
    PatchingCancelled = 300,
};

#ifdef __cplusplus
}
#endif
