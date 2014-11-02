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

#include "patchererror.h"

#include <cassert>


class PatcherError::Impl {
public:
    ErrorType errorType;
    ErrorCode errorCode;

    // Patcher creation error
    std::string patcherName;

    // IO error, cpio error
    std::string filename;
};


PatcherError::PatcherError() : m_impl(new Impl())
{
    m_impl->errorType = ErrorType::GenericError;
    m_impl->errorCode = ErrorCode::NoError;
}

PatcherError::PatcherError(const PatcherError &error) : m_impl(error.m_impl)
{
}

PatcherError::PatcherError(PatcherError &&error) : m_impl(std::move(error.m_impl))
{
}

PatcherError::PatcherError(ErrorType errorType, ErrorCode errorCode) : m_impl(new Impl())
{
    m_impl->errorType = errorType;
    m_impl->errorCode = errorCode;
}

PatcherError::~PatcherError()
{
}

PatcherError & PatcherError::operator=(const PatcherError &other)
{
    m_impl = other.m_impl;
    return *this;
}

PatcherError & PatcherError::operator=(PatcherError &&other)
{
    m_impl = std::move(other.m_impl);
    return *this;
}

PatcherError PatcherError::createGenericError(ErrorCode error)
{
    switch (error) {
    case NoError:
    //case CustomError:
    case UnknownError:
    case ImplementationError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::GenericError, error);
    return pe;
}

PatcherError PatcherError::createPatcherCreationError(ErrorCode error,
                                                      std::string name)
{
    switch (error) {
    case PatcherCreateError:
    case AutoPatcherCreateError:
    case RamdiskPatcherCreateError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::PatcherCreationError, error);
    pe.m_impl->patcherName = std::move(name);
    return pe;
}

PatcherError PatcherError::createIOError(ErrorCode error, std::string filename)
{
    switch (error) {
    case FileOpenError:
    case FileReadError:
    case FileWriteError:
    case DirectoryNotExistError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::IOError, error);
    pe.m_impl->filename = std::move(filename);
    return pe;
}

PatcherError PatcherError::createBootImageError(ErrorCode error)
{
    switch (error) {
    case BootImageSmallerThanHeaderError:
    case BootImageNoAndroidHeaderError:
    case BootImageNoRamdiskGzipHeaderError:
    case BootImageNoRamdiskSizeError:
    case BootImageNoKernelSizeError:
    case BootImageNoRamdiskAddressError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::BootImageError, error);
    return pe;
}

PatcherError PatcherError::createCpioError(ErrorCode error,
                                           std::string filename)
{
    switch (error) {
    case CpioFileAlreadyExistsError:
    case CpioFileNotExistError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::CpioError, error);
    pe.m_impl->filename = std::move(filename);
    return pe;
}

PatcherError PatcherError::createArchiveError(ErrorCode error,
                                              std::string filename)
{
    switch (error) {
    case ArchiveReadOpenError:
    case ArchiveReadDataError:
    case ArchiveReadHeaderError:
    case ArchiveWriteOpenError:
    case ArchiveWriteDataError:
    case ArchiveWriteHeaderError:
    case ArchiveCloseError:
    case ArchiveFreeError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::ArchiveError, error);
    pe.m_impl->filename = std::move(filename);
    return pe;
}

PatcherError PatcherError::createXmlError(ErrorCode error, std::string filename)
{
    switch (error) {
    case XmlParseFileError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::XmlError, error);
    pe.m_impl->filename = std::move(filename);
    return pe;
}

PatcherError PatcherError::createSupportedFileError(ErrorCode error,
                                                    std::string name)
{
    switch (error) {
    case OnlyZipSupported:
    case OnlyBootImageSupported:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::SupportedFileError, error);
    pe.m_impl->patcherName = std::move(name);
    return pe;
}

PatcherError PatcherError::createCancelledError(ErrorCode error)
{
    switch (error) {
    case PatchingCancelled:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::CancelledError, error);
    return pe;
}

PatcherError PatcherError::createPatchingError(ErrorCode error)
{
    switch (error) {
    case SystemCacheFormatLinesNotFound:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::PatchingError, error);
    return pe;
}

PatcherError::ErrorType PatcherError::errorType()
{
    return m_impl->errorType;
}

PatcherError::ErrorCode PatcherError::errorCode()
{
    return m_impl->errorCode;
}

std::string PatcherError::patcherName() {
    return m_impl->patcherName;
}

std::string PatcherError::filename() {
    return m_impl->filename;
}
