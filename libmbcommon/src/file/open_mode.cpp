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

#include "mbcommon/file/open_mode.h"

/*!
 * \file mbcommon/file/open_mode.h
 * \brief File open modes
 */

namespace mb
{

/*!
 * \enum FileOpenMode
 *
 * \brief Possible file open modes
 */

/*!
 * \var FileOpenMode::ReadOnly
 *
 * \brief Open file for reading.
 *
 * The file pointer is set to the beginning of the file.
 */

/*!
 * \var FileOpenMode::ReadWrite
 *
 * \brief Open file for reading and writing.
 *
 * The file pointer is set to the beginning of the file.
 */

/*!
 * \var FileOpenMode::WriteOnly
 *
 * \brief Truncate file and open for writing.
 *
 * The file pointer is set to the beginning of the file.
 */

/*!
 * \var FileOpenMode::ReadWriteTrunc
 *
 * \brief Truncate file and open for reading and writing.
 *
 * The file pointer is set to the beginning of the file.
 */

/*!
 * \var FileOpenMode::Append
 *
 * \brief Open file for appending.
 *
 * The file pointer is set to the end of the file.
 */

/*!
 * \var FileOpenMode::ReadAppend
 *
 * \brief Open file for reading and appending.
 *
 * The file pointer is initially set to the beginning of the file, but writing
 * always occurs at the end of the file.
 */

}
