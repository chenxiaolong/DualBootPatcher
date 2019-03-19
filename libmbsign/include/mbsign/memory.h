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

#include <memory>
#include <type_traits>

#include <cstdint>

#include "mbcommon/common.h"
#include "mbcommon/type_traits.h"

namespace mb::sign
{

MB_EXPORT void * secure_alloc(size_t size) noexcept;
MB_EXPORT void secure_free(void *ptr) noexcept;

template<typename T, class = std::enable_if_t<std::is_trivial_v<T>>>
using SecureUniquePtr = std::unique_ptr<T, TypeFn<secure_free>>;

template<typename T>
inline SecureUniquePtr<T> make_secure_unique_ptr() noexcept
{
    return SecureUniquePtr<T>(static_cast<T *>(secure_alloc(sizeof(T))));
}

template<typename T, class = std::enable_if_t<std::is_trivial_v<T>>>
inline SecureUniquePtr<T> make_secure_array(size_t n) noexcept
{
    if (n > SIZE_MAX / sizeof(T)) {
        return nullptr;
    }

    return SecureUniquePtr<T>(static_cast<T *>(secure_alloc(n * sizeof(T))));
}

}
