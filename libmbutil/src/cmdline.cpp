/*
 * Copyright (C) 2014-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/cmdline.h"

#include "mbcommon/string.h"

#include "mbutil/file.h"


namespace mb::util
{

oc::result<KernelCmdlineArgs> kernel_cmdline()
{
    OUTCOME_TRY(data, file_read_all("/proc/cmdline"));

    if (!data.empty() && data.back() == '\n') {
        data.pop_back();
    }

    KernelCmdlineArgs result;

    for (auto const &item : split_sv(data, " ")) {
        if (item.empty()) {
            continue;
        }

        if (auto pos = item.find('='); pos != std::string_view::npos) {
            result.emplace(item.substr(0, pos), item.substr(pos + 1));
        } else {
            result.emplace(item, std::nullopt);
        }
    }

    return std::move(result);
}

}
