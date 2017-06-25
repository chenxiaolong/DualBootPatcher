/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
        const char *name = token;
        const char *value = nullptr;

        char *equals = strchr(token, '=');
        if (equals) {
            *equals = '\0';
            value = equals + 1;
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
    const char *option;
    bool exists;
    std::string value;
};

static CmdlineIterAction get_option_cb(const char *name, const char *value,
                                       void *userdata)
{
    Ctx *ctx = static_cast<Ctx *>(userdata);

    if (strcmp(name, ctx->option) == 0) {
        if (value) {
            ctx->exists = true;
            ctx->value = value;
        } else {
            ctx->exists = false;
        }
        return CmdlineIterAction::Stop;
    }

    return CmdlineIterAction::Continue;
}

bool kernel_cmdline_get_option(const char *option, std::string *out)
{
    Ctx ctx;
    ctx.option = option;

    if (!kernel_cmdline_iter(get_option_cb, &ctx)) {
        return false;
    }

    if (ctx.exists) {
        out->swap(ctx.value);
        return true;
    } else {
        return false;
    }
}

}
}
