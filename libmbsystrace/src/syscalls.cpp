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

#include "mbsystrace/syscalls.h"

#include <cstring>

#include "mbsystrace/syscalls_list_p.h"


namespace mb::systrace
{

/*!
 * \class SysCall
 *
 * \brief Class representing a syscall number
 */

/*!
 * \brief Construct an invalid SysCall instance
 */
SysCall::SysCall() noexcept
    : m_num()
    , m_abi()
    , m_name()
    , m_valid(false)
{
}

/*!
 * \brief Construct a SysCall instance from a syscall number
 *
 * \param num Syscall number
 * \param abi Syscall ABI
 *
 * \pre \ref operator bool() will indicate whether \p num and \p abi are valid
 */
SysCall::SysCall(SysCallNum num, ArchAbi abi) noexcept
    : SysCall()
{
    auto i = static_cast<std::underlying_type_t<ArchAbi>>(abi);

    for (auto it = detail::g_syscalls_map[i]; it->name; ++it) {
        if (it->num == num) {
            m_num = num;
            m_abi = abi;
            m_name = it->name;
            m_valid = true;
            return;
        }
    }
}

/*!
 * \brief Construct a SysCall instance from a syscall name
 *
 * \param name Syscall name. Only `<sys/syscall.h>` macro names without the
 *             `SYS_` or `__NR_` prefixes (eg. `read`) are recognized.
 * \param abi Syscall ABI
 *
 * \pre \ref operator bool() will indicate whether \p name and \p abi are valid
 */
SysCall::SysCall(const char *name, ArchAbi abi) noexcept
    : SysCall()
{
    auto i = static_cast<std::underlying_type_t<ArchAbi>>(abi);

    for (auto it = detail::g_syscalls_map[i]; it->name; ++it) {
        if (strcmp(it->name, name) == 0) {
            m_num = it->num;
            m_abi = abi;
            m_name = name;
            m_valid = true;
            return;
        }
    }
}

/*!
 * \brief Syscall number
 *
 * \return If valid, the syscall number. Otherwise, an unspecified value.
 */
SysCallNum SysCall::num() const noexcept
{
    return m_num;
}

/*!
 * \brief Syscall ABI
 *
 * \return If valid, the syscall ABI. Otherwise, an unspecified value.
 */
ArchAbi SysCall::abi() const noexcept
{
    return m_abi;
}

/*!
 * \brief Syscall name
 *
 * \return If valid, the syscall name. Otherwise, an unspecified value.
 */
const char * SysCall::name() const noexcept
{
    return m_name;
}

/*!
 * \brief Check whether the syscall is valid
 */
SysCall::operator bool() const noexcept
{
    return m_valid;
}

/*!
 * \brief Check whether this instance equals \p rhs
 */
bool SysCall::operator==(const SysCall &rhs) const noexcept
{
    return m_valid == rhs.m_valid
            && m_num == rhs.m_num
            && m_abi == rhs.m_abi;
}

/*!
 * \brief Check whether this instance does not equal \p rhs
 */
bool SysCall::operator!=(const SysCall &rhs) const noexcept
{
    return !(*this == rhs);
}

}
