/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbpio/private/string.h"

#include <memory>

#include <cstdarg>

namespace io
{
namespace priv
{

std::string format(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    std::size_t size = vsnprintf(nullptr, 0, fmt, ap) + 1;
    va_end(ap);

    std::unique_ptr<char[]> buf(new char[size]);

    va_start(ap, fmt);
    vsnprintf(buf.get(), size, fmt, ap);
    va_end(ap);

    return std::string(buf.get(), buf.get() + size - 1);
}

}
}
