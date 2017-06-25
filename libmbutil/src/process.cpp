/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbutil/process.h"

#include <cerrno>
#include <cstdlib>
#include <cstring>

#include <unistd.h>

#include "mbutil/string.h"

namespace mb
{
namespace util
{

static char * args_mem_start = nullptr;
static size_t args_mem_size = 0;

/*!
 * \brief Initialize state before setting process name
 *
 * \warning Call this function as early as possible in the \a main() function
 *          and do not modify \a argv or \a environ beforehand. \a argv will be
 *          modified by this function, so make a copy before using it, but
 *          \a environ will be automatically cloned and set appropriately.
 *
 * \param argc MUST be the \a argc from \a main()
 * \param argv MUST be the \a argv from \a main()
 *
 * \return Returns true if the initialization is successful or if the function
 *         has already been called. Returns false and sets errno to EINVAL if
 *         \a argc or \a argv are invalid. Returns false and sets errno
 *         appropriately if memory allocation fails.
 */
bool set_process_title_init(int argc, char *argv[])
{
    // Already called once
    if (args_mem_start) {
        return true;
    }

    // Check for invalid parameters
    if (argc == 0 || !argv || !argv[0]) {
        errno = EINVAL;
        return false;
    }

    // The arguments and environment are stored in a contiguous block of memory,
    // so find the end of that block
    char *end = nullptr;
    for (int i = 0; i < argc || (i >= argc && argv[i]); ++i) {
        if (!end || end == argv[i]) {
            end = argv[i] + strlen(argv[i]) + 1;
        }
    }
    for (size_t i = 0; environ[i]; ++i) {
        if (end == environ[i]) {
            end = environ[i] + strlen(environ[i]) + 1;
        }
    }

    // Make a copy of the environment, which is a NULL-terminated array of
    // strings
    char **environ_copy = dup_cstring_list(environ);
    if (!environ_copy) {
        return false;
    }

    argv[1] = nullptr;
    args_mem_start = argv[0];
    args_mem_size = end - args_mem_start;
    environ = environ_copy;

    return true;
}

/*!
 * \brief Set title of process
 *
 * \param str Title
 * \param size Size of title
 * \param size_out Actual size that was set (can be NULL)
 *
 * \return Returns false and sets errno to EINVAL if set_process_title_init()
 *         hasn't been called yet. Otherwise, returns true and \a size_out, if
 *         non-NULL, is set to the number of bytes that were actually used in
 *         the process title, which may be fewer than requested.
 */
bool set_process_title(const char *str, size_t size, size_t *size_out)
{
    // Don't do anything if set_process_title_init() hasn't been called
    if (!args_mem_start || args_mem_size == 0) {
        errno = EINVAL;
        return false;
    }

    // Number of chars to copy, excluding NULL-terminator.
    size_t to_copy = size > args_mem_size - 1 ? args_mem_size - 1 : size;

    memcpy(args_mem_start, str, to_copy);
    memset(args_mem_start + to_copy, 0, args_mem_size - to_copy);

    if (size_out) {
        *size_out = to_copy;
    }

    return true;
}

/*!
 * \brief Set title of process using format string
 *
 * \param exceeded_out Number of bytes longer than the max process title size
 * \param fmt Format string
 *
 * \return Whether the process title was successfully set
 */
bool set_process_title_v(size_t *exceeded_out, const char *fmt, ...)
{
    char buf[1024];

    va_list ap;
    va_start(ap, fmt);
    int ret = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (ret < 0) {
        return false;
    }

    size_t n = (size_t) ret >= sizeof(buf) ? sizeof(buf) - 1 : ret;
    size_t n_set;
    if (!set_process_title(buf, n, &n_set)) {
        return false;
    }

    if (exceeded_out) {
        *exceeded_out = n - n_set;
    }

    return true;
}

}
}
