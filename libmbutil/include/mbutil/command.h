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

#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <cstddef>

namespace mb::util
{

/*!
 * \brief Raw command output callback
 *
 * If \a data.empty(), then EOF has been reached.
 *
 * \param data Data read from the stream
 * \param error stderr if true, else stdout
 */
using CmdRawCb = std::function<void(std::string_view data, bool error)>;

/*!
 * \brief Line command output callback
 *
 * \note If an entire line has been read, then the last character of \a line
 * will be the newline character. Otherwise, if the line is too long to fit in
 * the internal buffer or if the last line does not end with a newline
 * character, then \a line will not end with a newline character.
 *
 * If \a size is 0, then EOF has been reached.
 *
 * \param line Line that was read
 * \param error stderr if true, else stdout
 */
using CmdLineCb = std::function<void(std::string_view line, bool error)>;

struct CommandCtxPriv;

struct CommandCtx
{
    /*! Path to executable */
    std::string path;
    /*! Argument list */
    std::vector<std::string> argv;
    /*! Environment variable list */
    std::optional<std::vector<std::string>> envp;
    /*! Directory to chroot before execution */
    std::string chroot_dir;
    /*! Whether to redirect stdio fds */
    bool redirect_stdio = false;

    // Logging

    bool log_argv = false;
    bool log_envp = false;

    // Private variables

    std::unique_ptr<CommandCtxPriv> _priv;
};

bool command_start(CommandCtx &ctx);
int command_wait(CommandCtx &ctx);

bool command_raw_reader(CommandCtx &ctx, const CmdRawCb &cb);
bool command_line_reader(CommandCtx &ctx, const CmdLineCb &cb);

int run_command(const std::string &path,
                const std::vector<std::string> &argv,
                const std::optional<std::vector<std::string>> &envp,
                const std::string &chroot_dir,
                const CmdLineCb &cb);

}
