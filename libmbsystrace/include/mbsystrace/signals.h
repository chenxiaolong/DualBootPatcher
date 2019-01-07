/*
 * Copyright (C) 2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/common.h"


namespace mb::systrace
{

class MB_EXPORT Signal final
{
public:
    Signal() noexcept;
    Signal(int num) noexcept;
    Signal(const char *name) noexcept;

    int num() const noexcept;
    const char * name() const noexcept;

    operator bool() const noexcept;

    bool operator==(const Signal &rhs) const noexcept;
    bool operator!=(const Signal &rhs) const noexcept;

private:
    int m_num;
    const char *m_name;
    bool m_valid;
};

}
