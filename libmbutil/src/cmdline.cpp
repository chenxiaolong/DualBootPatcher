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

#include <fcntl.h>
#include <unistd.h>

#include "mbcommon/error_code.h"
#include "mbcommon/finally.h"
#include "mbcommon/string.h"


namespace mb::util
{

oc::result<KernelCmdlineArgs> kernel_cmdline()
{
    std::string args;

    {
        int fd = open("/proc/cmdline", O_RDONLY);
        if (fd < 0) {
            return ec_from_errno();
        }

        auto close_fd = finally([&fd] {
            close(fd);
        });

        char buf[10240];

        while (true) {
            ssize_t n = read(fd, buf, sizeof(buf));
            if (n < 0) {
                return ec_from_errno();
            } else if (n == 0) {
                break;
            } else {
                args.insert(args.end(), buf, buf + n);
            }
        }
    }

    if (!args.empty() && args.back() == '\n') {
        args.pop_back();
    }

    KernelCmdlineArgs result;

    for (auto const &item : split_sv(args, " ")) {
        if (item.empty()) {
            continue;
        }

        auto pos = item.find('=');
        if (pos != std::string_view::npos) {
            result.emplace(item.substr(0, pos), item.substr(pos + 1));
        } else {
            result.emplace(item, std::nullopt);
        }
    }

    return std::move(result);
}

}
