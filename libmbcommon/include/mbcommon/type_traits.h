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

#include <cstddef>

namespace mb
{

// Based on https://stackoverflow.com/a/43526780

// Get nth template argument type

template <std::size_t N, typename T0, typename... Ts>
struct TypeN
{
    using type = typename TypeN<N - 1, Ts...>::type;
};

template <typename T0, typename... Ts>
struct TypeN<0, T0, Ts...>
{
    using type = T0;
};

// Get nth function argument type

template <std::size_t, typename>
struct ArgN;

template <std::size_t N, typename R, typename... As>
struct ArgN<N, R(As...)>
{
    using type = typename TypeN<N, As...>::type;
};

template <std::size_t N, typename R, typename... As>
struct ArgN<N, R(As...) noexcept>
{
    using type = typename TypeN<N, As...>::type;
};

// Get function return type

template <typename>
struct ReturnType;

template <typename R, typename... As>
struct ReturnType<R(As...)>
{
    using type = R;
};

template <typename R, typename... As>
struct ReturnType<R(As...) noexcept>
{
    using type = R;
};

#ifdef _WIN32
template <typename R, typename... As>
struct ReturnType<__stdcall R(As...)>
{
    using type = R;
};

template <typename R, typename... As>
struct ReturnType<__stdcall R(As...) noexcept>
{
    using type = R;
};
#endif

}
