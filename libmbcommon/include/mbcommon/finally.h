/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/common.h"

namespace mb
{

// Perform action once this goes out of scope, essentially acting as the
// "finally" part of a try-finally block (in eg. Java)
template <typename F>
class Finally
{
public:
    MB_DISABLE_DEFAULT_CONSTRUCTOR(Finally)
    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(Finally)

    Finally(F f)
        : _f(std::move(f))
        , _dismissed(false)
    {
    }

    Finally(Finally &&other)
        : _f(std::move(other._f))
        , _dismissed(other._dismissed)
    {
        other.dismiss();
    }

    ~Finally() noexcept
    {
        // TODO: Uncomment if we're ever able to support exceptions
        //static_assert(noexcept(_f()), "Finally block must be noexcept");
        if (!_dismissed) {
            _f();
        }
    }

    Finally & operator=(Finally &&rhs)
    {
        _f = std::move(rhs._f);
        _dismissed = rhs._dismissed;
        rhs.dismiss();
        return *this;
    }

    void dismiss() noexcept
    {
        _dismissed = true;
    }

private:
    F _f;
    bool _dismissed;
};

template <typename F>
Finally<F> finally(F f)
{
    return { std::move(f) };
}

}
