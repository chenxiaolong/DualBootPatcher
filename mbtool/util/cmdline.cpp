/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "util/cmdline.h"

#include <vector>
#include <cstring>

#include "util/file.h"
#include "util/string.h"

namespace mb
{
namespace util
{

bool kernel_cmdline_get_option(const std::string &option,
                               std::string *out)
{
    std::string line;
    if (!file_first_line("/proc/cmdline", &line)) {
        return false;
    }
    std::vector<char> linebuf(line.begin(), line.end());
    linebuf.resize(linebuf.size() + 1);

    char *temp;
    char *token;

    token = strtok_r(linebuf.data(), " ", &temp);
    while (token != NULL) {
        if (starts_with(token, option)) {
            char *p = token + option.size();
            if (*p == '\0' || *p == ' ') {
                // No value
                *out = "";
                return true;
            } else if (*p == '=') {
                // Has value
                char *end = strchr(linebuf.data(), ' ');
                if (end) {
                    // Not at end of string
                    size_t len = end - p - 1;
                    std::vector<char> buf(len);
                    std::memcpy(buf.data(), p + 1, len);
                    out->assign(buf.begin(), buf.end());
                    return true;
                } else {
                    // End of string
                    *out = p + 1;
                    return true;
                }
            } else {
                // Option did not match entire string
            }
        }

        token = strtok_r(NULL, " ", &temp);
    }

    return false;
}

}
}