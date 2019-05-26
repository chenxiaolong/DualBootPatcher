/*
 * Copyright (C) 2016-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

namespace mb::util
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
 * \return Returns nothing if the initialization is successful or if the
 *         function has already been called. Returns std::errc::invalid_argument
 *         if \a argc or \a argv are invalid. Returns std::errc::not_enough_memory
 *         if memory allocation fails.
 */
oc::result<void> set_process_title_init(int argc, char *argv[])
{
    // Already called once
    if (args_mem_start) {
        return oc::success();
    }

    // Check for invalid parameters
    if (argc == 0 || !argv || !argv[0]) {
        return std::errc::invalid_argument;
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
        return std::errc::not_enough_memory;
    }

    argv[1] = nullptr;
    args_mem_start = argv[0];
    args_mem_size = static_cast<size_t>(end - args_mem_start);
    environ = environ_copy;

    return oc::success();
}

/*!
 * \brief Set title of process
 *
 * \param title Title
 *
 * \return Returns std::errc::invalid_argument if set_process_title_init()
 *         hasn't been called yet. Otherwise, returns the number of bytes that
 *         were actually used in the process title, which may be fewer than
 *         requested.
 */
oc::result<size_t> set_process_title(std::string_view title)
{
    // Don't do anything if set_process_title_init() hasn't been called
    if (!args_mem_start || args_mem_size == 0) {
        return std::errc::invalid_argument;
    }

    // Number of chars to copy, excluding NULL-terminator.
    size_t to_copy = title.size() > args_mem_size - 1
            ? args_mem_size - 1 : title.size();

    memcpy(args_mem_start, title.data(), to_copy);
    memset(args_mem_start + to_copy, 0, args_mem_size - to_copy);

    return to_copy;
}

}
