/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
                    "  --end-offset    Ending boundary offset for search\n"
                    "  --buffer-size   Buffer size\n",
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

static bool hex_to_binary(const char *hex, void **data, size_t *data_size)
{
    size_t size = strlen(hex);
    unsigned char *buf;

    if (size & 1) {
        errno = EINVAL;
        return false;
    }

    buf = static_cast<unsigned char *>(malloc(size / 2));
    if (!buf) {
        return false;
    }

    for (size_t i = 0; i < size; i += 2) {
        int hi = ascii_to_hex(hex[i]);
        int lo = ascii_to_hex(hex[i + 1]);

        if (hi < 0 || lo < 0) {
            free(buf);
            errno = EINVAL;
            return false;
        }

        buf[i / 2] = static_cast<unsigned char>((hi << 4) | lo);
    }

    *data = buf;
    *data_size = size / 2;

    return true;
}

static mb::oc::result<mb::FileSearchAction>
search_result_cb(const char *name, mb::File &file, uint64_t offset)
{
    (void) file;
    printf("%s: 0x%016" PRIx64 "\n", name, offset);
    return mb::FileSearchAction::Continue;
}

static bool search(const char *name, mb::File &file,
                   std::optional<uint64_t> start,
                   std::optional<uint64_t> end,
                   size_t bsize, const void *pattern,
                   size_t pattern_size, std::optional<uint64_t> max_matches)
{
    using namespace std::placeholders;

    auto ret = mb::file_search(file, start, end, bsize, pattern, pattern_size,
                               max_matches,
                               std::bind(search_result_cb, name, _1, _2));
    if (!ret) {
        fprintf(stderr, "%s: Search failed: %s\n",
                name, ret.error().message().c_str());
        return false;
    }
    return true;
}

static bool search_stdin(std::optional<uint64_t> start,
                         std::optional<uint64_t> end,
                         size_t bsize, const void *pattern,
                         size_t pattern_size,
                         std::optional<uint64_t> max_matches)
{
    mb::PosixFile file;

    auto ret = file.open(stdin, false);
    if (!ret) {
        fprintf(stderr, "Failed to open stdin: %s\n",
                ret.error().message().c_str());
        return false;
    }

    return search("stdin", file, start, end, bsize, pattern, pattern_size,
                  max_matches);
}

static bool search_file(const char *path,
                        std::optional<uint64_t> start,
                        std::optional<uint64_t> end,
                        size_t bsize, const void *pattern,
                        size_t pattern_size,
                        std::optional<uint64_t> max_matches)
{
    mb::StandardFile file;

    auto ret = file.open(path, mb::FileOpenMode::ReadOnly);
    if (!ret) {
        fprintf(stderr, "%s: Failed to open file: %s\n",
                path, ret.error().message().c_str());
        return false;
    }

    return search(path, file, start, end, bsize, pattern, pattern_size,
                  max_matches);
}

int main(int argc, char *argv[])
{
    std::optional<uint64_t> start;
    std::optional<uint64_t> end;
    size_t bsize = 0;
    std::optional<uint64_t> max_matches;

    const char *text_pattern = nullptr;
    const char *hex_pattern = nullptr;
    void *pattern = nullptr;
    size_t pattern_size = 0;
    bool pattern_needs_free = false;

    int opt;

    // Arguments with no short options
    enum : int
    {
        OPT_START_OFFSET         = CHAR_MAX + 1,
        OPT_END_OFFSET           = CHAR_MAX + 2,
        OPT_BUFFER_SIZE          = CHAR_MAX + 3,
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
        {"buffer-size",  required_argument, nullptr, OPT_BUFFER_SIZE},
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

        case 'p':
            hex_pattern = optarg;
            break;

        case 't':
            text_pattern = optarg;
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

        case OPT_BUFFER_SIZE:
            if (!mb::str_to_num(optarg, 10, bsize)) {
                fprintf(stderr, "Invalid value for --buffer-size: %s\n",
                        optarg);
                return EXIT_FAILURE;
            }
            break;

        case 'h':
            usage(stdout, argv[0]);
            return EXIT_SUCCESS;

        default:
            usage(stderr, argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (!hex_pattern && !text_pattern) {
        fprintf(stderr, "No pattern provided\n");
        return EXIT_FAILURE;
    } else if (hex_pattern) {
        if (!hex_to_binary(hex_pattern, &pattern, &pattern_size)) {
            fprintf(stderr, "Invalid hex pattern: %s\n", strerror(errno));
            return EXIT_FAILURE;
        }
        pattern_needs_free = true;
    } else if (text_pattern) {
        pattern = const_cast<char *>(text_pattern);
        pattern_size = strlen(text_pattern);
    }

    bool ret = true;

    if (optind == argc) {
        ret = search_stdin(start, end, bsize, pattern, pattern_size,
                           max_matches);
    } else {
        for (int i = optind; i < argc; ++i) {
            bool ret2 = search_file(argv[i], start, end, bsize, pattern,
                                    pattern_size, max_matches);
            if (!ret2) {
                ret = false;
            }
        }
    }

    if (pattern_needs_free) {
        free(pattern);
    }

    return ret ? EXIT_SUCCESS : EXIT_FAILURE;
}
