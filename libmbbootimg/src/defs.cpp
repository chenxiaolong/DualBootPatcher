/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbbootimg/defs.h"

// NOTE: This file is for documentation only.

/*!
 * \file mbbootimg/defs.h
 * \brief Boot image reader API
 */

// Formats documentation

/*!
 * \defgroup MB_BI_FORMAT_CODES File format codes
 *
 * \brief Supported format codes
 */

/*!
 * \def MB_BI_FORMAT_BASE_MASK
 * \ingroup MB_BI_FORMAT_CODES
 *
 * \brief Mask to get base format code from variant format codes
 */

/*!
 * \def MB_BI_FORMAT_ANDROID
 * \ingroup MB_BI_FORMAT_CODES
 *
 * \brief Format code for Android boot images
 */

/*!
 * \def MB_BI_FORMAT_BUMP
 * \ingroup MB_BI_FORMAT_CODES
 *
 * \brief Format code for Bump'ed Android boot images
 */

/*!
 * \def MB_BI_FORMAT_LOKI
 * \ingroup MB_BI_FORMAT_CODES
 *
 * \brief Format code for Loki'd Android boot images
 */

/*!
 * \def MB_BI_FORMAT_MTK
 * \ingroup MB_BI_FORMAT_CODES
 *
 * \brief Format code for MTK-style Android boot images
 */

/*!
 * \def MB_BI_FORMAT_SONY_ELF
 * \ingroup MB_BI_FORMAT_CODES
 *
 * \brief Format code for Sony ELF boot images
 */

/*!
 * \defgroup MB_BI_FORMAT_NAMES File format names
 *
 * \brief Supported format names
 */

/*!
 * \def MB_BI_FORMAT_NAME_ANDROID
 * \ingroup MB_BI_FORMAT_NAMES
 *
 * \brief Format name for Android boot images
 */

/*!
 * \def MB_BI_FORMAT_NAME_BUMP
 * \ingroup MB_BI_FORMAT_NAMES
 *
 * \brief Format name for Bump'ed Android boot images
 */

/*!
 * \def MB_BI_FORMAT_NAME_LOKI
 * \ingroup MB_BI_FORMAT_NAMES
 *
 * \brief Format name for Loki'd Android boot images
 */

/*!
 * \def MB_BI_FORMAT_NAME_MTK
 * \ingroup MB_BI_FORMAT_NAMES
 *
 * \brief Format name for MTK-style Android boot images
 */

/*!
 * \def MB_BI_FORMAT_NAME_SONY_ELF
 * \ingroup MB_BI_FORMAT_NAMES
 *
 * \brief Format name for Sony ELF boot images
 */

// Return values documentation

/*!
 * \defgroup MB_BI_RETURN_VALS Return values
 *
 * \brief Common return values for most functions
 */

/*!
 * \def MB_BI_EOF
 * \ingroup MB_BI_RETURN_VALS
 *
 * \brief Reached EOF
 *
 * Reached end-of-file.
 */

/*!
 * \def MB_BI_OK
 * \ingroup MB_BI_RETURN_VALS
 *
 * \brief Success error code
 *
 * Success error code.
 */

/*!
 * \def MB_BI_RETRY
 * \ingroup MB_BI_RETURN_VALS
 *
 * \brief Reattempt operation
 *
 * The operation should be reattempted.
 */

/*!
 * \def MB_BI_WARN
 * \ingroup MB_BI_RETURN_VALS
 *
 * \brief Warning
 *
 * The operation raised a warning. The instance can still be used although the
 * functionality may be degraded.
 */

/*!
 * \def MB_BI_FAILED
 * \ingroup MB_BI_RETURN_VALS
 *
 * \brief Non-fatal error
 *
 * The operation failed non-fatally. The instance can still be used for further
 * operations.
 */

/*!
 * \def MB_BI_FATAL
 * \ingroup MB_BI_RETURN_VALS
 *
 * \brief Fatal error
 *
 * The operation failed fatally. The instance can no longer be used and should
 * be freed.
 */

/*!
 * \def MB_BI_UNSUPPORTED
 * \ingroup MB_BI_RETURN_VALS
 *
 * \brief Operation not supported
 *
 * The operation is not supported.
 */

// Error codes documentation

/*!
 * \defgroup MB_BI_ERROR_CODES Error codes
 *
 * \brief Possible error codes
 */

/*!
 * \def MB_BI_ERROR_NONE
 * \ingroup MB_BI_ERROR_CODES
 *
 * \brief No error
 */

/*!
 * \def MB_BI_ERROR_INVALID_ARGUMENT
 * \ingroup MB_BI_ERROR_CODES
 *
 * \brief An invalid argument was provided
 */

/*!
 * \def MB_BI_ERROR_UNSUPPORTED
 * \ingroup MB_BI_ERROR_CODES
 *
 * \brief The operation is not supported
 */

/*!
 * \def MB_BI_ERROR_FILE_FORMAT
 * \ingroup MB_BI_ERROR_CODES
 *
 * \brief The file does not conform to the file format
 */

/*!
 * \def MB_BI_ERROR_PROGRAMMER_ERROR
 * \ingroup MB_BI_ERROR_CODES
 *
 * \brief The function were called in an invalid state
 */

/*!
 * \def MB_BI_ERROR_INTERNAL_ERROR
 * \ingroup MB_BI_ERROR_CODES
 *
 * \brief Internal error in the library
 */
