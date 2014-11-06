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

#include "cwrapper/cpatchererror.h"

#include <cstdlib>
#include <cstring>

#include "patchererror.h"


/*!
 * \file cpatchererror.h
 * \brief C Wrapper for PatcherError
 *
 * Please see the documentation for PatcherError from the C++ API for more
 * details. The C functions directly correspond to the PatcherError member
 * functions.
 *
 * \sa PatcherError
 */

/*!
 * \typedef CPatcherError
 * \brief C wrapper for PatcherError object
 */

extern "C" {

    /*!
     * \brief Destroys a CPatcherError object.
     *
     * \param info CPatcherError to destroy
     */
    void mbp_error_destroy(CPatcherError *error)
    {
        delete reinterpret_cast<PatcherError *>(error);
    }

    /*!
     * \brief Get error type
     *
     * \return Error type
     *
     * \sa PatcherError::errorType()
     */
    ErrorType mbp_error_error_type(const CPatcherError *error)
    {
        return reinterpret_cast<const PatcherError *>(error)->errorType();
    }

    /*!
     * \brief Get error code
     *
     * \return Error code
     *
     * \sa PatcherError::errorCode()
     */
    ErrorCode mbp_error_error_code(const CPatcherError *error)
    {
        return reinterpret_cast<const PatcherError *>(error)->errorCode();
    }

    /*!
     * \brief Name of patcher that caused the error
     *
     * \note The returned string is dynamically allocated. It should be free()'d
     *       when it is no longer needed.
     *
     * \return Patcher name
     *
     * \sa PatcherError::patcherName()
     */
    char * mbp_error_patcher_name(const CPatcherError *error)
    {
        const PatcherError *pe = reinterpret_cast<const PatcherError *>(error);
        return strdup(pe->patcherName().c_str());
    }

    /*!
     * \brief File the error refers to
     *
     * \note The returned string is dynamically allocated. It should be free()'d
     *       when it is no longer needed.
     *
     * \return Filename
     *
     * \sa PatcherError::filename()
     */
    char * mbp_error_filename(const CPatcherError *error)
    {
        const PatcherError *pe = reinterpret_cast<const PatcherError *>(error);
        return strdup(pe->filename().c_str());
    }

}
