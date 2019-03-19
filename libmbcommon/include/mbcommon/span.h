/*
 * Copyright (C) 2018-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

// Implementation of C++20's std::span

#pragma once

#include <algorithm>
#include <array>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>

#include <cstddef>

#if __cplusplus > 201703
#  error Replace mb::span with std::span after switching to C++20
#endif

namespace mb::detail
{

constexpr const std::size_t dynamic_extent =
    std::numeric_limits<std::size_t>::max();

template<class T, std::size_t Extent = dynamic_extent>
class span;

template<class T>
struct is_span_impl : std::false_type {};

template<class T, std::size_t Extent>
struct is_span_impl<span<T, Extent>> : std::true_type {};

template<class T>
struct is_span : is_span_impl<std::remove_cv_t<T>> {};

template<class T>
struct is_std_array_impl : std::false_type {};

template<class T, std::size_t N>
struct is_std_array_impl<std::array<T, N>> : std::true_type {};

template<class T>
struct is_std_array : is_std_array_impl<std::remove_cv_t<T>> {};

template<class T, class ElementType, class = void>
struct is_container : std::false_type {};

template<class T, class ElementType>
struct is_container<
    T,
    ElementType,
    std::void_t<
        std::enable_if_t<!is_span<T>::value>,
        std::enable_if_t<!is_std_array<T>::value>,
        std::enable_if_t<!std::is_array_v<T>>,
        decltype(std::data(std::declval<T>())),
        decltype(std::size(std::declval<T>())),
        std::enable_if_t<
            std::is_convertible_v<
                std::remove_pointer_t<decltype(std::data(std::declval<T &>()))>(*)[],
                ElementType(*)[]
            >
        >
    >
> : std::true_type {};

// https://en.cppreference.com/w/cpp/header/span
template<class T, std::size_t Extent>
class span
{
public:
    // https://en.cppreference.com/w/cpp/container/span

    using element_type           = T;
    using value_type             = std::remove_cv_t<T>;
    using pointer                = T *;
    using reference              = T &;
    using index_type             = std::size_t;
    using difference_type        = std::ptrdiff_t;
    using iterator               = pointer;
    using const_iterator         = const T *;
    using reverse_iterator       = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    static constexpr std::size_t extent = Extent;

    // https://en.cppreference.com/w/cpp/container/span/span

    template<
        bool B = Extent == 0 || Extent == dynamic_extent,
        class = std::enable_if_t<B>
    >
    constexpr span() noexcept
        : data_(nullptr)
        , size_(0)
    {
    }

    constexpr span(pointer ptr, index_type count)
        : data_(ptr)
        , size_(count)
    {
    }

    constexpr span(pointer firstElem, pointer lastElem)
        : data_(firstElem)
        , size_(static_cast<std::size_t>(lastElem - firstElem))
    {
    }

    template<
        std::size_t N,
        class = std::enable_if_t<Extent == dynamic_extent || Extent == N>
    >
    constexpr span(element_type (&arr)[N]) noexcept
        : data_(std::data(arr))
        , size_(N)
    {}

    template<
        std::size_t N,
        class = std::enable_if_t<
            (Extent == dynamic_extent || Extent == N)
                && std::is_convertible_v<value_type(*)[], element_type(*)[]>
        >
    >
    constexpr span(std::array<value_type, N> &arr) noexcept
        : data_(std::data(arr))
        , size_(N)
    {}

    template<
        std::size_t N,
        class = std::enable_if_t<
            (Extent == dynamic_extent || Extent == N)
                && std::is_convertible_v<value_type(*)[], element_type(*)[]>
        >
    >
    constexpr span(const std::array<value_type, N> &arr) noexcept
        : data_(std::data(arr))
        , size_(N)
    {}

    template<
        class Container,
        class = std::enable_if_t<is_container<Container, element_type>::value>
    >
    constexpr span(Container &cont)
        : data_(std::data(cont))
        , size_(std::size(cont))
    {}

    template<
        class Container,
        class = std::enable_if_t<
            std::is_const_v<element_type>
                && is_container<Container, element_type>::value
        >
    >
    constexpr span(const Container &cont)
        : data_(std::data(cont))
        , size_(std::size(cont))
    {}

    template<
        class U,
        std::size_t N,
        class = std::enable_if_t<
            (Extent == dynamic_extent || Extent == N)
                && std::is_convertible_v<U(*)[], element_type(*)[]>
        >
    >
    constexpr span(const span<U, N> &other) noexcept
        : data_(other.data())
        , size_(other.size())
    {
    }

    constexpr span(const span &other) noexcept = default;

    ~span() noexcept = default;

    // https://en.cppreference.com/w/cpp/container/span/operator%3D

    constexpr span & operator=(const span &other) noexcept = default;

    // https://en.cppreference.com/w/cpp/container/span/first

    template<std::size_t Count>
    constexpr span<element_type, Count>
    first() const
    {
        return { data(), Count };
    }

    constexpr span<element_type, dynamic_extent>
    first(std::size_t count) const
    {
        return { data(), count };
    }

    // https://en.cppreference.com/w/cpp/container/span/last

    template<std::size_t Count>
    constexpr span<element_type, Count>
    last() const
    {
        return { data() + (size() - Count), Count };
    }

    constexpr span<element_type, dynamic_extent>
    last(std::size_t count) const
    {
        return { data() + (size() - count), count };
    }

    // https://en.cppreference.com/w/cpp/container/span/subspan

    template<std::size_t Offset, std::size_t Count = dynamic_extent>
    constexpr span<
        element_type,
        ((Count != dynamic_extent)
            ? Count
            : ((Extent != dynamic_extent)
                ? (Extent - Offset)
                : dynamic_extent))
    >
    subspan() const
    {
        return {
            data() + Offset,
            Count == dynamic_extent ? size() - Offset : Count
        };
    }

    constexpr span<element_type, dynamic_extent>
    subspan(std::size_t offset, std::size_t count = dynamic_extent) const
    {
        return span<element_type, dynamic_extent>(
            data() + offset,
            count == dynamic_extent ? size() - offset : count
        );
    }

    // https://en.cppreference.com/w/cpp/container/span/size

    constexpr index_type size() const noexcept
    {
        return size_;
    }

    // https://en.cppreference.com/w/cpp/container/span/size_bytes

    constexpr index_type size_bytes() const noexcept
    {
        return size() * sizeof(element_type);
    }

    // https://en.cppreference.com/w/cpp/container/span/empty

    [[nodiscard]] constexpr bool empty() const noexcept
    {
        return size() == 0;
    }

    // https://en.cppreference.com/w/cpp/container/span/front

    constexpr reference front() const noexcept
    {
        return *begin();
    }

    // https://en.cppreference.com/w/cpp/container/span/back

    constexpr reference back() const noexcept
    {
        return *(end() - 1);
    }

    // https://en.cppreference.com/w/cpp/container/span/operator_at

    constexpr reference operator[](index_type idx) const
    {
        return data()[idx];
    }

    constexpr reference operator()(index_type idx) const
    {
        return data()[idx];
    }

    // https://en.cppreference.com/w/cpp/container/span/data

    constexpr pointer data() const noexcept
    {
        return data_;
    }

    // https://en.cppreference.com/w/cpp/container/span/begin

    constexpr iterator begin() const noexcept
    {
        return { data() };
    }

    constexpr const_iterator cbegin() const noexcept
    {
        return { data() };
    }

    // https://en.cppreference.com/w/cpp/container/span/end

    constexpr iterator end() const noexcept
    {
        return { data() + size() };
    }

    constexpr const_iterator cend() const noexcept
    {
        return { data() + size() };
    }

    // https://en.cppreference.com/w/cpp/container/span/rbegin

    constexpr reverse_iterator rbegin() const noexcept
    {
        return { end() };
    }

    constexpr const_reverse_iterator crbegin() const noexcept
    {
        return { cend() };
    }

    // https://en.cppreference.com/w/cpp/container/span/rend

    constexpr reverse_iterator rend() const noexcept
    {
        return { begin() };
    }

    constexpr const_reverse_iterator crend() const noexcept
    {
        return { cbegin() };
    }

private:
    pointer data_;
    index_type size_;
};

// https://en.cppreference.com/w/cpp/container/span/as_bytes

template<class T, std::size_t N>
inline constexpr span<
    const std::byte,
    ((N == dynamic_extent) ? dynamic_extent : (sizeof(T) * N))
>
as_bytes(span<T, N> spn) noexcept
{
    return {
        reinterpret_cast<std::byte const *>(spn.data()),
        spn.size_bytes()
    };
}

template<
    class T, std::size_t N,
    class = std::enable_if_t<!std::is_const_v<T>>
>
inline constexpr span<
    std::byte,
    ((N == dynamic_extent) ? dynamic_extent : (sizeof(T) * N))
>
as_writable_bytes(span<T, N> s) noexcept
{
    return {
        reinterpret_cast<std::byte *>(s.data()),
        s.size_bytes()
    };
}

// https://en.cppreference.com/w/cpp/container/span/deduction_guides

template<class T, std::size_t N>
span(T (&)[N]) -> span<T, N>;

template<class T, std::size_t N>
span(std::array<T, N> &) -> span<T, N>;

template<class T, std::size_t N>
span(const std::array<T, N> &) -> span<const T, N>;

template<class Container>
span(Container &) -> span<typename Container::value_type>;

template<class Container>
span(const Container &) -> span<const typename Container::value_type>;

// libmbcommon-specific additions to convert an object to a byte span

template<
    class T,
    class = std::enable_if_t<std::is_trivial_v<T>>
>
inline constexpr span<const std::byte, sizeof(T)>
as_bytes(const T &t) noexcept
{
    return span<const std::byte, sizeof(t)>(
            reinterpret_cast<const std::byte *>(&t), sizeof(t));
}

template<
    class T,
    class = std::void_t<
        std::enable_if_t<std::is_trivial_v<T>>,
        std::enable_if_t<!std::is_const_v<T>>
    >
>
inline constexpr span<std::byte, sizeof(T)>
as_writable_bytes(T &t) noexcept
{
    return span<std::byte, sizeof(t)>(
            reinterpret_cast<std::byte *>(&t), sizeof(t));
}

}

namespace mb
{

using detail::dynamic_extent;

using detail::span;

using detail::as_bytes;
using detail::as_writable_bytes;

}  // namespace nonstd
