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


/*! \cond INTERNAL */
class PatcherError::Impl {
public:
    MBP::ErrorType errorType;
    MBP::ErrorCode errorCode;

    // Patcher creation error
    std::string patcherName;

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
    m_impl->errorType = MBP::ErrorType::GenericError;
    m_impl->errorCode = MBP::ErrorCode::NoError;
}

PatcherError::PatcherError(const PatcherError &error) : m_impl(error.m_impl)
{
}

PatcherError::PatcherError(PatcherError &&error) : m_impl(std::move(error.m_impl))
{
}

PatcherError::PatcherError(MBP::ErrorType errorType, MBP::ErrorCode errorCode)
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

/*! \cond INTERNAL */
PatcherError PatcherError::createGenericError(MBP::ErrorCode error)
{
    switch (error) {
    case MBP::ErrorCode::NoError:
    //case MBP::ErrorCode::CustomError:
    case MBP::ErrorCode::UnknownError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(MBP::ErrorType::GenericError, error);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createPatcherCreationError(MBP::ErrorCode error,
                                                      std::string name)
{
    switch (error) {
    case MBP::ErrorCode::PatcherCreateError:
    case MBP::ErrorCode::AutoPatcherCreateError:
    case MBP::ErrorCode::RamdiskPatcherCreateError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(MBP::ErrorType::PatcherCreationError, error);
    pe.m_impl->patcherName = std::move(name);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createIOError(MBP::ErrorCode error,
                                         std::string filename)
{
    switch (error) {
    case MBP::ErrorCode::FileOpenError:
    case MBP::ErrorCode::FileReadError:
    case MBP::ErrorCode::FileWriteError:
    case MBP::ErrorCode::DirectoryNotExistError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(MBP::ErrorType::IOError, error);
    pe.m_impl->filename = std::move(filename);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createBootImageError(MBP::ErrorCode error)
{
    switch (error) {
    case MBP::ErrorCode::BootImageSmallerThanHeaderError:
    case MBP::ErrorCode::BootImageNoAndroidHeaderError:
    case MBP::ErrorCode::BootImageNoRamdiskGzipHeaderError:
    case MBP::ErrorCode::BootImageNoRamdiskAddressError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(MBP::ErrorType::BootImageError, error);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createCpioError(MBP::ErrorCode error,
                                           std::string filename)
{
    switch (error) {
    case MBP::ErrorCode::CpioFileAlreadyExistsError:
    case MBP::ErrorCode::CpioFileNotExistError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(MBP::ErrorType::CpioError, error);
    pe.m_impl->filename = std::move(filename);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createArchiveError(MBP::ErrorCode error,
                                              std::string filename)
{
    switch (error) {
    case MBP::ErrorCode::ArchiveReadOpenError:
    case MBP::ErrorCode::ArchiveReadDataError:
    case MBP::ErrorCode::ArchiveReadHeaderError:
    case MBP::ErrorCode::ArchiveWriteOpenError:
    case MBP::ErrorCode::ArchiveWriteDataError:
    case MBP::ErrorCode::ArchiveWriteHeaderError:
    case MBP::ErrorCode::ArchiveCloseError:
    case MBP::ErrorCode::ArchiveFreeError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(MBP::ErrorType::ArchiveError, error);
    pe.m_impl->filename = std::move(filename);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createXmlError(MBP::ErrorCode error,
                                          std::string filename)
{
    switch (error) {
    case MBP::ErrorCode::XmlParseFileError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(MBP::ErrorType::XmlError, error);
    pe.m_impl->filename = std::move(filename);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createSupportedFileError(MBP::ErrorCode error,
                                                    std::string name)
{
    switch (error) {
    case MBP::ErrorCode::OnlyZipSupported:
    case MBP::ErrorCode::OnlyBootImageSupported:
        break;
    default:
        assert(false);
    }

    PatcherError pe(MBP::ErrorType::SupportedFileError, error);
    pe.m_impl->patcherName = std::move(name);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createCancelledError(MBP::ErrorCode error)
{
    switch (error) {
    case MBP::ErrorCode::PatchingCancelled:
        break;
    default:
        assert(false);
    }

    PatcherError pe(MBP::ErrorType::CancelledError, error);
    return pe;
}
/*! \endcond */

/*! \cond INTERNAL */
PatcherError PatcherError::createPatchingError(MBP::ErrorCode error)
{
    switch (error) {
    case MBP::ErrorCode::SystemCacheFormatLinesNotFound:
    case MBP::ErrorCode::ApplyPatchFileError:
        break;
    default:
        assert(false);
    }

    PatcherError pe(MBP::ErrorType::PatchingError, error);
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
MBP::ErrorType PatcherError::errorType() const
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
MBP::ErrorCode PatcherError::errorCode() const
{
    return m_impl->errorCode;
}

/*!
 * \brief Name of patcher that caused the error
 *
 * \Note This is valid only if the error type is PatcherCreationError or
 *       SupportedFileError.
 *
 * \return Patcher name
 */
std::string PatcherError::patcherName() const
{
    return m_impl->patcherName;
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
