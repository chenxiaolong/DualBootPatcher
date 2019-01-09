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

#include "mbsystrace/signals.h"

#include <cstring>

#include "mbsystrace/signals_list_p.h"


namespace mb::systrace
{

/*!
 * \class Signal
 *
 * \brief Class representing a Unix signal
 */

/*!
 * \brief Construct an invalid Signal instance
 */
Signal::Signal() noexcept
    : m_num()
    , m_name()
    , m_valid(false)
{
}

/*!
 * \brief Construct a Signal instance from a signal number
 *
 * \param num Signal number
 *
 * \pre \ref operator bool() will indicate whether \p num is valid
 */
Signal::Signal(int num) noexcept
{
    for (auto it = detail::g_signals; it->name; ++it) {
        if (it->num == num) {
            m_num = it->num;
            m_name = it->name;
            m_valid = true;
            return;
        }
    }
}

/*!
 * \brief Construct a Signal instance from a signal name
 *
 * \param name Signal name. Only `<signal.h>` macro names (eg. `SIGKILL`) are
 *             recognized.
 *
 * \pre \ref operator bool() will indicate whether \p name is valid
 */
Signal::Signal(const char *name) noexcept
    : Signal()
{
    for (auto it = detail::g_signals; it->name; ++it) {
        if (strcmp(it->name, name) == 0) {
            m_num = it->num;
            m_name = name;
            m_valid = true;
            return;
        }
    }
}

/*!
 * \brief Signal number
 *
 * \return If valid, the signal number. Otherwise, an unspecified value.
 */
int Signal::num() const noexcept
{
    return m_num;
}

/*!
 * \brief Signal name
 *
 * \return If valid, the signal name (in `SIG...` form). Otherwise, an
 *         unspecified value.
 */
const char * Signal::name() const noexcept
{
    return m_name;
}

/*!
 * \brief Check whether the signal is valid
 */
Signal::operator bool() const noexcept
{
    return m_valid;
}

/*!
 * \brief Check whether this instance equals \p rhs
 */
bool Signal::operator==(const Signal &rhs) const noexcept
{
    return m_valid == rhs.m_valid
            && m_num == rhs.m_num;
}

/*!
 * \brief Check whether this instance does not equal \p rhs
 */
bool Signal::operator!=(const Signal &rhs) const noexcept
{
    return !(*this == rhs);
}

}
