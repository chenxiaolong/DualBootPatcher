/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <vector>

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/file.h"
#include "mbutil/string.h"

#define LOG_TAG "mbutil/cmdline"

namespace mb
{
namespace util
{

bool kernel_cmdline_iter(CmdlineIterFn fn, void *userdata)
{
    char buf[10240];

    int fd = open("/proc/cmdline", O_RDONLY);
    if (fd < 0) {
        LOGE("%s: Failed to open file: %s", "/proc/cmdline", strerror(errno));
        return false;
    }

    ssize_t n = read(fd, buf, sizeof(buf) - 1);
    if (n < 0) {
        LOGE("%s: Failed to read file: %s", "/proc/cmdline", strerror(errno));
        close(fd);
        return false;
    }

    close(fd);

    // Truncate newline and NULL-terminate
    if (n > 0 && buf[n - 1] == '\n') {
        --n;
    }
    buf[n] = '\0';

    LOGD("Kernel cmdline: %s", buf);

    char *save_ptr;
    char *token;

    for (token = strtok_r(buf, " ", &save_ptr); token;
            token = strtok_r(nullptr, " ", &save_ptr)) {
        std::string name;
        optional<std::string> value;

        char *equals = strchr(token, '=');
        if (equals) {
            name = {token, equals};
            value = {equals + 1};
        } else {
            name = token;
        }

        CmdlineIterAction action = fn(name, value, userdata);
        switch (action) {
        case CmdlineIterAction::Continue:
            continue;
        case CmdlineIterAction::Stop:
            break;
        case CmdlineIterAction::Error:
        default:
            return false;
        }
    }

    return true;
}

struct Ctx
{
    std::string option;
    optional<std::string> value;
};

static CmdlineIterAction get_option_cb(const std::string &name,
                                       const optional<std::string> &value,
                                       void *userdata)
{
    Ctx *ctx = static_cast<Ctx *>(userdata);

    if (name == ctx->option) {
        ctx->value = value;
        return CmdlineIterAction::Stop;
    }

    return CmdlineIterAction::Continue;
}

bool kernel_cmdline_get_option(const std::string &option,
                               optional<std::string> &value)
{
    Ctx ctx;
    ctx.option = option;

    if (!kernel_cmdline_iter(get_option_cb, &ctx)) {
        return false;
    }

    value = std::move(ctx.value);
    return true;
}

}
}
