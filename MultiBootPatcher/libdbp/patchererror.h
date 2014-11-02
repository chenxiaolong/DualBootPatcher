/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

#ifndef PATCHERERROR_H
#define PATCHERERROR_H

#include <memory>

#include "libdbp_global.h"


class LIBDBPSHARED_EXPORT PatcherError
{
public:
    enum ErrorType {
        GenericError,
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

    enum ErrorCode {
        // Generic
        NoError,
        //CustomError,
        UnknownError,
        ImplementationError,

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
        BootImageNoRamdiskSizeError,
        BootImageNoKernelSizeError,
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
    };

    static PatcherError createGenericError(ErrorCode error);
    static PatcherError createPatcherCreationError(ErrorCode error, std::string name);
    static PatcherError createIOError(ErrorCode error, std::string filename);
    static PatcherError createBootImageError(ErrorCode error);
    static PatcherError createCpioError(ErrorCode error, std::string filename);
    static PatcherError createArchiveError(ErrorCode error, std::string filename);
    static PatcherError createXmlError(ErrorCode error, std::string filename);
    static PatcherError createSupportedFileError(ErrorCode error, std::string name);
    static PatcherError createCancelledError(ErrorCode error);
    static PatcherError createPatchingError(ErrorCode error);

    ErrorType errorType();
    ErrorCode errorCode();

    std::string patcherName();
    std::string filename();

    PatcherError();
    PatcherError(const PatcherError &error);
    PatcherError(PatcherError &&error);
    ~PatcherError();
    PatcherError & operator=(const PatcherError &other);
    PatcherError & operator=(PatcherError &&other);


private:
    PatcherError(ErrorType errorType, ErrorCode errorCode);

    class Impl;
    std::shared_ptr<Impl> m_impl;
};

#endif // PATCHERERROR_H
