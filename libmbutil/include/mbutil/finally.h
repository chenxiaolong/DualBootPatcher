/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

namespace mb
{
namespace util
{

// Perform action once this goes out of scope, essentially acting as the
// "finally" part of a try-finally block (in eg. Java)
template <typename F>
class Finally {
public:
    Finally(F f) : _f(f)
    {
    }

    ~Finally()
    {
        _f();
    }

private:
    F _f;
};

template <typename F>
Finally<F> finally(F f)
{
    return Finally<F>(f);
}

}
}