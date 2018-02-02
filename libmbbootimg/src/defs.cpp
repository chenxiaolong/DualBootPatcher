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

#include "mbbootimg/defs.h"

// NOTE: This file is for documentation only.

/*!
 * \file mbbootimg/defs.h
 * \brief Boot image reader API
 */

namespace mb::bootimg
{

// Formats documentation

/*!
 * \defgroup MB_BI_FORMAT_CODES File format codes
 *
 * \brief Supported format codes
 */

/*!
 * \def FORMAT_BASE_MASK
 * \ingroup MB_BI_FORMAT_CODES
 *
 * \brief Mask to get base format code from variant format codes
 */

/*!
 * \def FORMAT_ANDROID
 * \ingroup MB_BI_FORMAT_CODES
 *
 * \brief Format code for Android boot images
 */

/*!
 * \def FORMAT_BUMP
 * \ingroup MB_BI_FORMAT_CODES
 *
 * \brief Format code for Bump'ed Android boot images
 */

/*!
 * \def FORMAT_LOKI
 * \ingroup MB_BI_FORMAT_CODES
 *
 * \brief Format code for Loki'd Android boot images
 */

/*!
 * \def FORMAT_MTK
 * \ingroup MB_BI_FORMAT_CODES
 *
 * \brief Format code for MTK-style Android boot images
 */

/*!
 * \def FORMAT_SONY_ELF
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
 * \def FORMAT_NAME_ANDROID
 * \ingroup MB_BI_FORMAT_NAMES
 *
 * \brief Format name for Android boot images
 */

/*!
 * \def FORMAT_NAME_BUMP
 * \ingroup MB_BI_FORMAT_NAMES
 *
 * \brief Format name for Bump'ed Android boot images
 */

/*!
 * \def FORMAT_NAME_LOKI
 * \ingroup MB_BI_FORMAT_NAMES
 *
 * \brief Format name for Loki'd Android boot images
 */

/*!
 * \def FORMAT_NAME_MTK
 * \ingroup MB_BI_FORMAT_NAMES
 *
 * \brief Format name for MTK-style Android boot images
 */

/*!
 * \def FORMAT_NAME_SONY_ELF
 * \ingroup MB_BI_FORMAT_NAMES
 *
 * \brief Format name for Sony ELF boot images
 */

}
