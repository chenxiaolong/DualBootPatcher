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

#include "cwrapper/cpatchererror.h"

#include <cassert>

#include "cwrapper/private/util.h"

#include "patchererror.h"


#define CAST(x) \
    assert(x != nullptr); \
    PatcherError *pe = reinterpret_cast<PatcherError *>(x);
#define CCAST(x) \
    assert(x != nullptr); \
    const PatcherError *pe = reinterpret_cast<const PatcherError *>(x);


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

extern "C" {

/*!
 * \brief Destroys a CPatcherError object.
 *
 * \param info CPatcherError to destroy
 */
void mbp_error_destroy(CPatcherError *error)
{
    CAST(error);
    delete pe;
}

/*!
 * \brief Get error type
 *
 * \return Error type
 *
 * \sa PatcherError::errorType()
 */
/* ErrorType */ int mbp_error_error_type(const CPatcherError *error)
{
    CCAST(error);
    return static_cast<int>(pe->errorType());
}

/*!
 * \brief Get error code
 *
 * \return Error code
 *
 * \sa PatcherError::errorCode()
 */
/* ErrorCode */ int mbp_error_error_code(const CPatcherError *error)
{
    CCAST(error);
    return static_cast<int>(pe->errorCode());
}

/*!
 * \brief Id of patcher that caused the error
 *
 * \note The returned string is dynamically allocated. It should be free()'d
 *       when it is no longer needed.
 *
 * \return Patcher name
 *
 * \sa PatcherError::patcherId()
 */
char * mbp_error_patcher_id(const CPatcherError *error)
{
    CCAST(error);
    return string_to_cstring(pe->patcherId());
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
    CCAST(error);
    return string_to_cstring(pe->filename());
}

}
