/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/flags.h"

/*! \cond INTERNAL */
namespace mb
{
namespace detail
{

enum class FileState : uint8_t
{
    New     = 1u << 0,
    Opened  = 1u << 1,
    Moved   = 1u << 2,
};
MB_DECLARE_FLAGS(FileStates, FileState)
MB_DECLARE_OPERATORS_FOR_FLAGS(FileStates)

}
}
/*! \endcond */
