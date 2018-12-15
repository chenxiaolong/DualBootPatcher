/*
 * Copyright (C) 2017-2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <getopt.h>

#include "mbcommon/file/standard.h"
#include "mbcommon/file/posix.h"
#include "mbcommon/integer.h"

static void usage(FILE *stream, const char *prog_name)
{
    fprintf(stream, "Usage: %s {-p <hex> | -t <text>} [option...] [<file>...]\n"
                    "\n"
                    "Options:\n"
                    "  -p, --hex <hex pattern>\n"
                    "                  Search file for hex pattern\n"
                    "  -t, --text <text pattern>\n"
                    "                  Search file for text pattern\n"
                    "  -n, --num-matches\n"
                    "                  Maximum number of matches\n"
                    "  --start-offset  Starting boundary offset for search\n"
                    "  --end-offset    Ending boundary offset for search\n",
                    prog_name);
}

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

    mb::FileSearcher searcher(&file, pattern.data(), pattern.size());

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
    mb::PosixFile file;

    auto ret = file.open(stdin, false);
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
    std::optional<uint64_t> start;
    std::optional<uint64_t> end;
    std::optional<uint64_t> max_matches;
    std::optional<std::string> pattern;

    int opt;

    // Arguments with no short options
    enum : int
    {
        OPT_START_OFFSET         = CHAR_MAX + 1,
        OPT_END_OFFSET           = CHAR_MAX + 2,
    };

    static const char short_options[] = "hn:p:t:";

    static struct option long_options[] = {
        // Arguments with short versions
        {"help",         no_argument,       nullptr, 'h'},
        {"num-matches",  required_argument, nullptr, 'n'},
        {"hex",          required_argument, nullptr, 'p'},
        {"text",         required_argument, nullptr, 't'},
        // Arguments without short versions
        {"start-offset", required_argument, nullptr, OPT_START_OFFSET},
        {"end-offset",   required_argument, nullptr, OPT_END_OFFSET},
        {nullptr,        0,                 nullptr, 0},
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'n': {
            uint64_t value;
            if (!mb::str_to_num(optarg, 10, value)) {
                fprintf(stderr, "Invalid value for -n/--num-matches: %s\n",
                        optarg);
                return EXIT_FAILURE;
            }
            max_matches = value;
            break;
        }

        case 'p': {
            pattern = "";
            if (!hex_to_binary(optarg, *pattern)) {
                fprintf(stderr, "Invalid hex pattern: %s\n", strerror(errno));
                return EXIT_FAILURE;
            }
            break;
        }

        case 't':
            pattern = optarg;
            break;

        case OPT_START_OFFSET: {
            uint64_t value;
            if (!mb::str_to_num(optarg, 0, value)) {
                fprintf(stderr, "Invalid value for --start-offset: %s\n",
                        optarg);
                return EXIT_FAILURE;
            }
            start = value;
            break;
        }

        case OPT_END_OFFSET: {
            uint64_t value;
            if (!mb::str_to_num(optarg, 0, value)) {
                fprintf(stderr, "Invalid value for --end-offset: %s\n",
                        optarg);
                return EXIT_FAILURE;
            }
            end = value;
            break;
        }

        case 'h':
            usage(stdout, argv[0]);
            return EXIT_SUCCESS;

        default:
            usage(stderr, argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (!pattern) {
        fprintf(stderr, "No pattern provided\n");
        return EXIT_FAILURE;
    }

    bool ret = true;

    if (optind == argc) {
        ret = search_stdin(start, end, *pattern, max_matches);
    } else {
        for (int i = optind; i < argc; ++i) {
            bool ret2 = search_file(argv[i], start, end, *pattern, max_matches);
            if (!ret2) {
                ret = false;
            }
        }
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
