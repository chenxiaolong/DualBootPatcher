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

#include "mbutil/command.h"

#include <array>

#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#include "mblog/logging.h"
#include "mbutil/string.h"

#define LOG_TAG "mbutil/command"

#define LOG_COMMANDS 0

namespace mb::util
{

static inline int safely_close(int &fd)
{
    int ret = 0;

    if (fd >= 0) {
        ret = close(fd);
        fd = -1;
    }

    return ret;
}

static inline void log_command(const std::string &path,
                               const std::vector<std::string> argv,
                               const std::optional<std::vector<std::string>> envp)
{
    LOGD("Running executable: %s", path.c_str());

    if (!argv.empty()) {
        std::string buf;
        bool first = true;

        buf += "Arguments: [ ";
        for (auto const &arg : argv) {
            if (first) {
                first = false;
            } else {
                buf += ", ";
            }
            buf += arg;
        }
        buf += " ]";

        LOGD("%s", buf.c_str());
    }

    if (envp && !envp->empty()) {
        LOGD("Environment:");

        for (auto const &env : *envp) {
            LOGD("- %s", env.c_str());
        }
    }
}

struct CommandCtxPriv
{
    pid_t pid;
    std::array<int, 2> stdout_pipe;
    std::array<int, 2> stderr_pipe;
};

static void initialize_priv(CommandCtxPriv &priv)
{
    priv.pid = -1;
    priv.stdout_pipe[0] = -1;
    priv.stdout_pipe[1] = -1;
    priv.stderr_pipe[0] = -1;
    priv.stderr_pipe[1] = -1;
}

bool command_start(CommandCtx &ctx)
{
    if (ctx._priv || ctx.path.empty()
            || ctx.argv.empty() || ctx.argv[0].empty()) {
        errno = EINVAL;
        goto error;
    }

    ctx._priv.reset(new CommandCtxPriv());
    initialize_priv(*ctx._priv);

    log_command(ctx.path,
                ctx.log_argv ? ctx.argv : std::vector<std::string>{},
                ctx.log_envp ? ctx.envp : std::optional<std::vector<std::string>>{});

    // Create stdout/stderr pipe if output callback is provided
    if (ctx.redirect_stdio) {
        if (pipe(ctx._priv->stdout_pipe.data()) < 0) {
            ctx._priv->stdout_pipe[0] = -1;
            ctx._priv->stdout_pipe[1] = -1;
            goto error;
        }
        if (pipe(ctx._priv->stderr_pipe.data()) < 0) {
            ctx._priv->stderr_pipe[0] = -1;
            ctx._priv->stderr_pipe[1] = -1;
            goto error;
        }

        // Make read ends non-blocking
        if (fcntl(ctx._priv->stdout_pipe[0], F_SETFL, O_NONBLOCK) < 0
                || fcntl(ctx._priv->stderr_pipe[0], F_SETFL, O_NONBLOCK) < 0) {
            goto error;
        }
    }

    ctx._priv->pid = fork();
    if (ctx._priv->pid < 0) {
        LOGE("Failed to fork: %s", strerror(errno));
        goto error;
    } else if (ctx._priv->pid == 0) {
        std::vector<const char *> c_argv;
        for (auto const &arg : ctx.argv) {
            c_argv.push_back(arg.c_str());
        }
        c_argv.push_back(nullptr);

        std::vector<const char *> c_envp;
        if (ctx.envp) {
            for (auto const &env : *ctx.envp) {
                c_envp.push_back(env.c_str());
            }
            c_envp.push_back(nullptr);
        }

        // Close read end of the pipes
        if (ctx.redirect_stdio) {
            safely_close(ctx._priv->stdout_pipe[0]);
            safely_close(ctx._priv->stderr_pipe[0]);
        }

        // Chroot if needed
        if (!ctx.chroot_dir.empty()) {
            if (chdir(ctx.chroot_dir.c_str()) < 0) {
                LOGE("%s: Failed to chdir: %s",
                     ctx.chroot_dir.c_str(), strerror(errno));
                goto child_error;
            }
            if (chroot(ctx.chroot_dir.c_str()) < 0) {
                LOGE("%s: Failed to chroot: %s",
                     ctx.chroot_dir.c_str(), strerror(errno));
                goto child_error;
            }
        }

        // Reassign stdout/stderr fds
        if (ctx.redirect_stdio) {
            if (dup2(ctx._priv->stdout_pipe[1], STDOUT_FILENO) < 0) {
                LOGE("Failed to redirect stdout: %s", strerror(errno));
                goto child_error;
            }
            if (dup2(ctx._priv->stderr_pipe[1], STDERR_FILENO) < 0) {
                LOGE("Failed to redirect stderr: %s", strerror(errno));
                goto child_error;
            }

            safely_close(ctx._priv->stdout_pipe[1]);
            safely_close(ctx._priv->stderr_pipe[1]);
        }

        if (ctx.envp) {
            execvpe(ctx.path.c_str(),
                    const_cast<char * const *>(c_argv.data()),
                    const_cast<char * const *>(c_envp.data()));
        } else {
            execvp(ctx.path.c_str(),
                   const_cast<char * const *>(c_argv.data()));
        }

        LOGE("%s: Failed to exec: %s", ctx.path.c_str(), strerror(errno));

    child_error:
        if (ctx.redirect_stdio) {
            safely_close(ctx._priv->stdout_pipe[0]);
            safely_close(ctx._priv->stdout_pipe[1]);
            safely_close(ctx._priv->stderr_pipe[0]);
            safely_close(ctx._priv->stderr_pipe[1]);
        }

        _exit(127);
    } else {
        // Close write ends of the pipes
        if (ctx.redirect_stdio) {
            safely_close(ctx._priv->stdout_pipe[1]);
            safely_close(ctx._priv->stderr_pipe[1]);
        }
    }

    return true;

error:
    if (ctx._priv && ctx.redirect_stdio) {
        safely_close(ctx._priv->stdout_pipe[0]);
        safely_close(ctx._priv->stdout_pipe[1]);
        safely_close(ctx._priv->stderr_pipe[0]);
        safely_close(ctx._priv->stderr_pipe[1]);
    }
    ctx._priv.reset();

    return false;
}

int command_wait(CommandCtx &ctx)
{
    // Check if process was started
    if (!ctx._priv) {
        errno = EINVAL;
        return -1;
    }

    // Close read ends of the pipe
    if (ctx.redirect_stdio) {
        safely_close(ctx._priv->stdout_pipe[0]);
        safely_close(ctx._priv->stderr_pipe[0]);
    }

    // Wait for child to finish
    int status;
    do {
        if (waitpid(ctx._priv->pid, &status, 0) < 0) {
            LOGE("Failed to waitpid: %s", strerror(errno));
            status = -1;
            break;
        }
    } while (!WIFEXITED(status) && !WIFSIGNALED(status));

    ctx._priv.reset();

    return status;
}

bool command_raw_reader(CommandCtx &ctx, CmdRawCb cb, void *userdata)
{
    bool ret = true;
    char buf[8192];

    struct pollfd fds[2];
    const int fds_size = sizeof(fds) / sizeof(fds[0]);

    fds[0].fd = ctx._priv->stdout_pipe[0];
    fds[0].events = POLLIN;
    fds[1].fd = ctx._priv->stderr_pipe[0];
    fds[1].events = POLLIN;

    while (true) {
        bool done = true;

        for (int i = 0; i < fds_size; ++i) {
            fds[i].revents = 0;
            done = done && (fds[i].fd < 0);
        }

        if (done) {
            break;
        }

        if (poll(fds, fds_size, -1) <= 0) {
            continue;
        }

        for (int i = 0; i < fds_size; ++i) {
            bool is_stderr = fds[i].fd == ctx._priv->stderr_pipe[0];

            if (fds[i].revents & POLLHUP) {
                // EOF/pipe closed. The fd will be closed later
                fds[i].fd = -1;
                fds[i].events = 0;

                // Final call for EOF
                cb(buf, 0, is_stderr, userdata);
            } else if (fds[i].revents & POLLIN) {
                ssize_t n = read(fds[i].fd, buf, sizeof(buf) - 1);
                if (n < 0) {
                    if (errno != EAGAIN
                            && errno != EWOULDBLOCK
                            && errno != EINTR) {
                        // Read failed; disable FD
                        fds[i].fd = -1;
                        fds[i].events = 0;
                        ret = false;
                    }
                } else {
                    // NULL-terminate
                    buf[n] = '\0';

                    // Potential EOF (n == 0) here on some non-Linux systems,
                    // which is fine since it's not possible to reach both the
                    // POLLHUP block and this block
                    cb(buf, static_cast<size_t>(n), is_stderr, userdata);
                }
            }
        }
    }

    return ret;
}

struct CommandLineReaderCtx
{
    std::array<char, 4096> stdout_buf;
    std::array<char, 4096> stderr_buf;
    size_t stdout_used = 0;
    size_t stderr_used = 0;

    CmdLineCb cb;
    void *userdata;
};

static void command_line_reader_cb(const char *data, size_t size, bool error,
                                   void *userdata)
{
    CommandLineReaderCtx *ctx = static_cast<CommandLineReaderCtx *>(userdata);

    size_t cap = (error ? ctx->stderr_buf : ctx->stdout_buf).size() - 1;
    char *buf = (error ? ctx->stderr_buf : ctx->stdout_buf).data();
    size_t &used = error ? ctx->stderr_used : ctx->stdout_used;

    // Reached EOF
    if (size == 0 && used > 0) {
        // NULL-terminate the buffer
        buf[used] = '\0';
        ctx->cb(buf, error, ctx->userdata);
        used = 0;
    }

    while (size > 0) {
        // Find available space
        size_t avail = cap - used;

        // Copy as much as we can into the buffer
        size_t n = size > avail ? avail : size;
        memcpy(buf + used, data, n);

        // Advance pointer
        data += n;
        size -= n;
        used += n;

        // NULL-terminate the buffer
        buf[used] = '\0';

        // Call cb for each contained line
        char *ptr = buf;
        char *newline;
        while ((newline = strchr(ptr, '\n'))) {
            // Temporarily NULL-terminate
            char c = *(newline + 1);
            *(newline + 1) = '\0';

            // Yay
            ctx->cb(ptr, error, ctx->userdata);

            // Restore character
            *(newline + 1) = c;

            // Advance pointer for next search
            ptr = newline + 1;
        }

        // ptr now points to the character after the last newline
        size_t consumed = static_cast<size_t>(ptr - buf);
        if (consumed == 0 && used == cap) {
            // If nothing was consumed and the buffer is full, then the line is
            // too long.
            ctx->cb(buf, error, ctx->userdata);
            consumed = used;
        }

        // Move unconsumed data to the beginning of the buffer
        if (consumed > 0) {
            used -= consumed;
            memmove(buf, ptr, used);
        }
    }
}

bool command_line_reader(CommandCtx &ctx, CmdLineCb cb, void *userdata)
{
    CommandLineReaderCtx reader_ctx;
    reader_ctx.cb = cb;
    reader_ctx.userdata = userdata;

    return command_raw_reader(ctx, &command_line_reader_cb, &reader_ctx);
}

int run_command(const std::string &path,
                const std::vector<std::string> &argv,
                const std::optional<std::vector<std::string>> &envp,
                const std::string &chroot_dir,
                CmdLineCb cb,
                void *userdata)
{
    CommandCtx ctx;
    ctx.path = path;
    ctx.argv = argv;
    ctx.envp = envp;
    ctx.chroot_dir = chroot_dir;
    ctx.redirect_stdio = !!cb;
#if LOG_COMMANDS
    ctx.log_argv = true;
    ctx.log_envp = true;
#else
    ctx.log_argv = false;
    ctx.log_envp = false;
#endif

    if (!command_start(ctx)) {
        return -1;
    }

    command_line_reader(ctx, cb, userdata);

    return command_wait(ctx);
}

}
