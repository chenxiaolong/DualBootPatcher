/*
 * Copyright (C) 2017-2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file_util.h"

#include <limits>
#include <string>
#include <string_view>
#include <type_traits>

#include <cerrno>
#include <cinttypes>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "CLI/CLI.hpp"

#ifdef _WIN32
#  include "mbcommon/file/win32.h"
#else
#  include "mbcommon/file/fd.h"
#endif

#include "mbcommon/file/standard.h"
#include "mbcommon/integer.h"

static int ascii_to_hex(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    } else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    } else {
        return -1;
    }
}

static bool hex_to_binary(std::string_view hex, std::string &data)
{
    if (hex.size() & 1) {
        errno = EINVAL;
        return false;
    }

    data.clear();
    data.reserve(hex.size() / 2);

    for (auto it = hex.begin(); it != hex.end(); it += 2) {
        int hi = ascii_to_hex(*it);
        int lo = ascii_to_hex(*(it + 1));

        if (hi < 0 || lo < 0) {
            errno = EINVAL;
            return false;
        }

        data.push_back(static_cast<char>((hi << 4) | lo));
    }

    return true;
}

static bool search(const char *name, mb::File &file,
                   std::optional<uint64_t> start,
                   std::optional<uint64_t> end,
                   std::string_view pattern,
                   std::optional<uint64_t> max_matches)
{
    using namespace std::placeholders;

    if (start) {
        if (auto r = file.seek(static_cast<int64_t>(*start), SEEK_SET); !r) {
            fprintf(stderr, "%s: Failed to seek file: %s\n",
                    name, r.error().message().c_str());
            return false;
        }
    } else {
        start = 0;
    }

    mb::FileSearcher searcher(
            &file, mb::as_uchars(pattern.data(), pattern.size()));

    while (true) {
        if (max_matches) {
            if (*max_matches == 0) {
                break;
            }
            --*max_matches;
        }

        if (auto r = searcher.next()) {
            if (r.value()) {
                if (end && (*r.value() + pattern.size()) > *end) {
                    // Artificial EOF
                    break;
                } else {
                    printf("%s: 0x%016" PRIx64 "\n", name, *r.value() + *start);
                }
            } else {
                // No more matches
                break;
            }
        } else {
            fprintf(stderr, "%s: Search failed: %s\n",
                    name, r.error().message().c_str());
            return false;
        }
    }

    return true;
}

static bool search_stdin(std::optional<uint64_t> start,
                         std::optional<uint64_t> end,
                         std::string_view pattern,
                         std::optional<uint64_t> max_matches)
{
#ifdef _WIN32
    auto handle = GetStdHandle(STD_INPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Failed to get stdin handle\n");
        return false;
    }

    mb::Win32File file;
    auto ret = file.open(handle, false, false);
#else
    mb::FdFile file;
    auto ret = file.open(STDIN_FILENO, false);
#endif

    if (!ret) {
        fprintf(stderr, "Failed to open stdin: %s\n",
                ret.error().message().c_str());
        return false;
    }

    return search("stdin", file, start, end, pattern, max_matches);
}

static bool search_file(const char *path,
                        std::optional<uint64_t> start,
                        std::optional<uint64_t> end,
                        std::string_view pattern,
                        std::optional<uint64_t> max_matches)
{
    mb::StandardFile file;

    auto ret = file.open(path, mb::FileOpenMode::ReadOnly);
    if (!ret) {
        fprintf(stderr, "%s: Failed to open file: %s\n",
                path, ret.error().message().c_str());
        return false;
    }

    return search(path, file, start, end, pattern, max_matches);
}

int main(int argc, char *argv[])
{
    CLI::App app;

    std::string pattern;
    auto pattern_group = app.add_option_group("Pattern", "Search pattern");
    pattern_group->required();
    pattern_group->add_option("-p,--hex", pattern,
                              "Search file for hex pattern")
            ->type_name("PATTERN")
            ->transform(CLI::Validator([](auto &input) {
                std::string result;
                if (!hex_to_binary(input, result)) {
                    return "invalid hex pattern";
                }
                input.swap(result);
                return "";
            }, "HEX"));
    pattern_group->add_option("-t,--text", pattern,
                              "Search file for text pattern")
            ->type_name("PATTERN");

    std::optional<uint64_t> max_matches;
    app.add_option("-n,--num-matches", max_matches,
                   "Maximum number of matches")
            ->type_name("NUM");

    std::optional<uint64_t> start;
    app.add_option("--start-offset", start,
                   "Starting boundary offset for search")
            ->type_name("OFFSET");

    std::optional<uint64_t> end;
    app.add_option("--end-offset", end,
                   "Ending boundary offset for search")
            ->type_name("OFFSET");

    std::vector<std::string> files;
    app.add_option("file", files, "Files to search")
            ->type_name("FILE");

    CLI11_PARSE(app, argc, argv);

    bool ret = true;

    if (files.empty()) {
        ret = search_stdin(start, end, pattern, max_matches);
    } else {
        for (auto const &f : files) {
            if (!search_file(f.c_str(), start, end, pattern, max_matches)) {
                ret = false;
            }
        }
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
