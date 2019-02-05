/*
 * Copyright (C) 2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <utility>

#include <unistd.h>

#include "mbcommon/common.h"
#include "mbcommon/error.h"


namespace mb::util
{

class UniqueFd final
{
public:
    UniqueFd() = default;

    explicit UniqueFd(int fd)
    {
        reset(fd);
    }

    ~UniqueFd()
    {
        reset();
    }

    UniqueFd(UniqueFd &&other) noexcept
    {
        reset(other.release());
    }

    UniqueFd & operator=(UniqueFd &&rhs) noexcept
    {
        reset(rhs.release());
        return *this;
    }

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(UniqueFd)

    void reset(int fd = -1)
    {
        ErrorRestorer restorer;

        if (m_fd != -1) {
            ::close(m_fd);
        }

        m_fd = fd;
    }

    int get() const
    {
        return m_fd;
    }

    operator int() const
    {
        return get();
    }

    [[nodiscard]] int release()
    {
        return std::exchange(m_fd, -1);
    }

private:
    int m_fd = -1;
};

}
