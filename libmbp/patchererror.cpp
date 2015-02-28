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

#include "patchererror.h"

#include <cassert>


namespace mbp
{

/*! \cond INTERNAL */
class PatcherError::Impl {
public:
    ErrorType errorType;
    ErrorCode errorCode;

    // Patcher creation error
    std::string patcherId;

    // IO error, cpio error
    std::string filename;
};
/*! \endcond */


/*!
 * \class PatcherError
 * \brief Provides a simple way of holding error information
 *
 * See errors.h for a list of available error types and error codes.
 */


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

PatcherError::PatcherError(ErrorType errorType, ErrorCode errorCode)
    : m_impl(new Impl())
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

PatcherError::operator bool() const
{
    return m_impl->errorType == ErrorType::GenericError
            && m_impl->errorCode == ErrorCode::NoError;
}

/*! \cond INTERNAL */
PatcherError PatcherError::createGenericError(ErrorCode error)
{
    switch (error) {
    case ErrorCode::NoError:
    //case ErrorCode::CustomError:
    case ErrorCode::UnknownError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::GenericError, error);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createPatcherCreationError(ErrorCode error,
                                                      std::string name)
{
    switch (error) {
    case ErrorCode::PatcherCreateError:
    case ErrorCode::AutoPatcherCreateError:
    case ErrorCode::RamdiskPatcherCreateError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::PatcherCreationError, error);
    pe.m_impl->patcherId = std::move(name);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createIOError(ErrorCode error,
                                         std::string filename)
{
    switch (error) {
    case ErrorCode::FileOpenError:
    case ErrorCode::FileReadError:
    case ErrorCode::FileWriteError:
    case ErrorCode::DirectoryNotExistError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::IOError, error);
    pe.m_impl->filename = std::move(filename);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createBootImageError(ErrorCode error)
{
    switch (error) {
    case ErrorCode::BootImageSmallerThanHeaderError:
    case ErrorCode::BootImageNoAndroidHeaderError:
    case ErrorCode::BootImageNoRamdiskGzipHeaderError:
    case ErrorCode::BootImageNoRamdiskAddressError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::BootImageError, error);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createCpioError(ErrorCode error,
                                           std::string filename)
{
    switch (error) {
    case ErrorCode::CpioFileAlreadyExistsError:
    case ErrorCode::CpioFileNotExistError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::CpioError, error);
    pe.m_impl->filename = std::move(filename);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createArchiveError(ErrorCode error,
                                              std::string filename)
{
    switch (error) {
    case ErrorCode::ArchiveReadOpenError:
    case ErrorCode::ArchiveReadDataError:
    case ErrorCode::ArchiveReadHeaderError:
    case ErrorCode::ArchiveWriteOpenError:
    case ErrorCode::ArchiveWriteDataError:
    case ErrorCode::ArchiveWriteHeaderError:
    case ErrorCode::ArchiveCloseError:
    case ErrorCode::ArchiveFreeError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::ArchiveError, error);
    pe.m_impl->filename = std::move(filename);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createXmlError(ErrorCode error,
                                          std::string filename)
{
    switch (error) {
    case ErrorCode::XmlParseFileError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::XmlError, error);
    pe.m_impl->filename = std::move(filename);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createSupportedFileError(ErrorCode error,
                                                    std::string name)
{
    switch (error) {
    case ErrorCode::OnlyZipSupported:
    case ErrorCode::OnlyBootImageSupported:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::SupportedFileError, error);
    pe.m_impl->patcherId = std::move(name);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createCancelledError(ErrorCode error)
{
    switch (error) {
    case ErrorCode::PatchingCancelled:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::CancelledError, error);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createPatchingError(ErrorCode error)
{
    switch (error) {
    case ErrorCode::SystemCacheFormatLinesNotFound:
    case ErrorCode::ApplyPatchFileError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(ErrorType::PatchingError, error);
    return pe;
}
/*! \endcond */

/*!
 * \brief Get error type
 *
 * \return Error type
 *
 * \sa errors.h
 */
ErrorType PatcherError::errorType() const
{
    return m_impl->errorType;
}

/*!
 * \brief Get error code
 *
 * \return Error code
 *
 * \sa errors.h
 */
ErrorCode PatcherError::errorCode() const
{
    return m_impl->errorCode;
}

/*!
 * \brief Id of patcher that caused the error
 *
 * \Note This is valid only if the error type is PatcherCreationError or
 *       SupportedFileError.
 *
 * \return Patcher id
 */
std::string PatcherError::patcherId() const
{
    return m_impl->patcherId;
}

/*!
 * \brief File the error refers to
 *
 * \Note This is valid only if the error type is IOError, CpioError,
 *       ArchiveError, or XmlError.
 *
 * \return Filename
 */
std::string PatcherError::filename() const
{
    return m_impl->filename;
}

}