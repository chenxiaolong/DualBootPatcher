/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "libmbpio/error.h"

namespace io
{

thread_local Error gError;
thread_local std::string gErrorString;

Error lastError()
{
    return gError;
}

std::string lastErrorString()
{
    return gErrorString;
}

void setLastError(Error error, std::string errorString)
{
    gError = error;
    gErrorString = std::move(errorString);
}

void setLastError(Error error)
{
    gError = error;
}

void setLastErrorString(std::string error)
{
    gErrorString = std::move(error);
}

}
