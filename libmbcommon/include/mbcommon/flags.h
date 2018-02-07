/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
 * Copyright (C) 2016  The Qt Company Ltd.
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

#include <type_traits>

#include <cstdio>

// Based on the QFlags code from Qt5

/*! \cond INTERNAL */
namespace mb
{

template<typename Enum>
class Flags
{
    static_assert(std::is_enum_v<Enum>,
                  "Flags is only usable on enumeration types.");

    struct Private;
    using Zero = int(Private::*);
    using Underlying = std::underlying_type_t<Enum>;

public:
    using enum_type = Enum;

    constexpr inline Flags(Enum f) noexcept
        : _value(static_cast<Underlying>(f))
    {
    }

    constexpr inline Flags(Zero = nullptr) noexcept
        : _value(0)
    {
    }

    constexpr inline Flags & operator&=(Flags f) noexcept
    {
        _value &= f._value;
        return *this;
    }

    constexpr inline Flags & operator&=(Enum f) noexcept
    {
        _value &= static_cast<Underlying>(f);
        return *this;
    }

    constexpr inline Flags & operator|=(Flags f) noexcept
    {
        _value |= f._value;
        return *this;
    }

    constexpr inline Flags & operator|=(Enum f) noexcept
    {
        _value |= static_cast<Underlying>(f);
        return *this;
    }

    constexpr inline Flags & operator^=(Flags f) noexcept
    {
        _value ^= f._value;
        return *this;
    }

    constexpr inline Flags & operator^=(Enum f) noexcept
    {
        _value ^= static_cast<Underlying>(f);
        return *this;
    }

    constexpr inline operator Underlying() const noexcept
    {
        return _value;
    }

    constexpr inline Flags operator|(Flags f) const noexcept
    {
        return static_cast<Enum>(_value | f._value);
    }

    constexpr inline Flags operator|(Enum f) const noexcept
    {
        return static_cast<Enum>(_value | static_cast<Underlying>(f));
    }

    constexpr inline Flags operator^(Flags f) const noexcept
    {
        return static_cast<Enum>(_value ^ f._value);
    }

    constexpr inline Flags operator^(Enum f) const noexcept
    {
        return static_cast<Enum>(_value ^ static_cast<Underlying>(f));
    }

    constexpr inline Flags operator&(Flags f) const noexcept
    {
        return static_cast<Enum>(_value & f);
    }

    constexpr inline Flags operator&(Enum f) const noexcept
    {
        return static_cast<Enum>(_value & static_cast<Underlying>(f));
    }

    constexpr inline Flags operator~() const noexcept
    {
        return static_cast<Enum>(~_value);
    }

    constexpr inline bool operator!() const noexcept
    {
        return !_value;
    }

    constexpr inline bool test_flag(Enum f) const noexcept
    {
        return (_value & static_cast<Underlying>(f)) == static_cast<Underlying>(f)
                && (static_cast<Underlying>(f) != 0
                        || _value == static_cast<Underlying>(f));
    }

    constexpr inline Flags & set_flag(Enum f, bool on = true) noexcept
    {
        return on ? (*this |= f)
                : (*this &= static_cast<Enum>(~static_cast<Underlying>(f)));
    }

private:
    Underlying _value;
};

#define MB_DECLARE_FLAGS(FLAGS, ENUM)\
    typedef ::mb::Flags<ENUM> FLAGS;

#define MB_DECLARE_OPERATORS_FOR_FLAGS(FLAGS) \
    constexpr inline ::mb::Flags<FLAGS::enum_type> \
    operator|(FLAGS::enum_type f1, FLAGS::enum_type f2) noexcept \
    { \
        return ::mb::Flags<FLAGS::enum_type>(f1) | f2; \
    } \
    constexpr inline ::mb::Flags<FLAGS::enum_type> \
    operator|(FLAGS::enum_type f1, ::mb::Flags<FLAGS::enum_type> f2) noexcept \
    { \
        return f2 | f1; \
    } \
    constexpr inline ::mb::Flags<FLAGS::enum_type> \
    operator&(FLAGS::enum_type f1, FLAGS::enum_type f2) noexcept \
    { \
        return ::mb::Flags<FLAGS::enum_type>(f1) & f2; \
    } \
    constexpr inline ::mb::Flags<FLAGS::enum_type> \
    operator&(FLAGS::enum_type f1, ::mb::Flags<FLAGS::enum_type> f2) noexcept \
    { \
        return f2 & f1; \
    }

}
/*! \endcond */
