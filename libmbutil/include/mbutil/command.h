/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <cstddef>

namespace mb
{
namespace util
{

/*!
 * \brief Raw command output callback
 *
 * \note \a data is always NULL-terminated. It is safe to pass \a data to
 * functions expecting a C-style string. However, \a size must be used for the
 * handling of binary data. \a data will not always contain a full line of
 * output.
 *
 * If \a size is 0, then EOF has been reached.
 *
 * \param data Bytes read from the stream
 * \param size Number of bytes read
 * \param error stderr if true, else stdout
 * \param userdata User-provided pointer
 */
typedef void (*CmdRawCb)(const char *data, size_t size, bool error,
                         void *userdata);

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
 * \param userdata User-provided pointer
 */
typedef void (*CmdLineCb)(const char *line, bool error, void *userdata);

struct CommandCtxPriv;

struct CommandCtx
{
    /*! Path to executable */
    const char *path = nullptr;
    /*! Argument list */
    const char * const *argv = nullptr;
    /*! Environment variable list */
    const char * const *envp = nullptr;
    /*! Directory to chroot before execution */
    const char *chroot_dir = nullptr;
    /*! Whether to redirect stdio fds */
    bool redirect_stdio = false;

    // Logging

    bool log_argv = false;
    bool log_envp = false;

    // Private variables

    CommandCtxPriv *_priv = nullptr;
};

bool command_start(struct CommandCtx *ctx);
int command_wait(struct CommandCtx *ctx);

bool command_raw_reader(struct CommandCtx *ctx, CmdRawCb cb, void *userdata);
bool command_line_reader(struct CommandCtx *ctx, CmdLineCb cb, void *userdata);

void command_line_reader(const char *data, size_t size, bool error,
                         void *userdata);

int run_command(const char *path,
                const char * const *argv,
                const char * const *envp,
                const char *chroot_dir,
                CmdLineCb cb,
                void *userdata);

}
}
