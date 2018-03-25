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

#pragma once

#include "mbcommon/outcome.h"

namespace mb::util
{

struct FileOpErrorInfo
{
    std::string path;
    std::error_code ec;

    std::string message() const;
};

inline const std::error_code & make_error_code(const FileOpErrorInfo &ei)
{
    return ei.ec;
}

[[noreturn]]
inline void throw_as_system_error_with_payload(const FileOpErrorInfo &ei)
{
    (void) ei;
    OUTCOME_THROW_EXCEPTION(std::system_error(make_error_code(ei)));
}

template <typename T>
using FileOpResult = oc::result<T, FileOpErrorInfo>;

}
