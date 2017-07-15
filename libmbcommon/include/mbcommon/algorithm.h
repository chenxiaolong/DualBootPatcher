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

#pragma once

#include <algorithm>
#include <functional>

namespace mb
{

#if defined(__cpp_constexpr) && __cpp_constexpr-0 >= 201304

template<class ForwardIt, class T, class Compare=std::less<>>
ForwardIt binary_find(ForwardIt first, ForwardIt last, const T &value,
                      Compare comp={})
{
    ForwardIt it = std::lower_bound(first, last, value, comp);
    return (it != last && !bool(comp(value, *it))) ? it : last;
}

#else

template<class ForwardIt, class T>
ForwardIt binary_find(ForwardIt first, ForwardIt last, const T &value)
{
    ForwardIt it = std::lower_bound(first, last, value);
    return (it != last && !(value < *it)) ? it : last;
}

template<class ForwardIt, class T, class Compare>
ForwardIt binary_find(ForwardIt first, ForwardIt last, const T &value,
                      Compare comp)
{
    ForwardIt it = std::lower_bound(first, last, value, comp);
    return (it != last && !bool(comp(value, *it))) ? it : last;
}

#endif

}
