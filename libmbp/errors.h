/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#ifdef __cplusplus
namespace mbp
{
#endif

#ifdef __cplusplus
enum class ErrorType : int
{
#else
enum ErrorType
{
#endif
    GenericError = 0,
    PatcherCreationError,
    IOError,
    BootImageError,
    CpioError,
    ArchiveError,
    XmlError,
    SupportedFileError,
    CancelledError,
    PatchingError,
};

#ifdef __cplusplus
enum class ErrorCode : int
{
#else
enum ErrorCode
{
#endif
    // Generic
    NoError = 0,
    //CustomError,
    UnknownError,

    // PatcherCreation
    PatcherCreateError,
    AutoPatcherCreateError,
    RamdiskPatcherCreateError,

    // I/O
    FileOpenError,
    FileReadError,
    FileWriteError,
    DirectoryNotExistError,

    // Boot image
    BootImageSmallerThanHeaderError,
    BootImageNoAndroidHeaderError,
    BootImageNoRamdiskGzipHeaderError,
    BootImageNoRamdiskAddressError,

    // cpio
    CpioFileAlreadyExistsError,
    CpioFileNotExistError,

    // Archive
    ArchiveReadOpenError,
    ArchiveReadDataError,
    ArchiveReadHeaderError,
    ArchiveWriteOpenError,
    ArchiveWriteDataError,
    ArchiveWriteHeaderError,
    ArchiveCloseError,
    ArchiveFreeError,

    // XML
    XmlParseFileError,

    // Supported files
    OnlyZipSupported,
    OnlyBootImageSupported,

    // Cancelled
    PatchingCancelled,

    // Patching errors

    // Primary upgrade patcher
    SystemCacheFormatLinesNotFound,

    // Patch File patcher
    ApplyPatchFileError,
};

#ifdef __cplusplus
}
#endif
