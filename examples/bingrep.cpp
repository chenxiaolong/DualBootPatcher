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

#include "mbcommon/file/filename.h"
#include "mbcommon/file/posix.h"

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

template<typename SIntType>
static inline bool str_to_snum(const char *str, int base, SIntType *out)
{
    static_assert(std::is_signed<SIntType>::value,
                  "Integer type is not signed");
    static_assert(std::numeric_limits<SIntType>::min() >= LLONG_MIN
                  && std::numeric_limits<SIntType>::max() <= LLONG_MAX,
                  "Integer type to too large to handle");

    char *end;
    errno = 0;
    auto num = strtoll(str, &end, base);
    if (errno == ERANGE
            || num < std::numeric_limits<SIntType>::min()
            || num > std::numeric_limits<SIntType>::max()) {
        errno = ERANGE;
        return false;
    } else if (*str == '\0' || *end != '\0') {
        errno = EINVAL;
        return false;
    }
    *out = static_cast<SIntType>(num);
    return true;
}

template<typename UIntType>
static inline bool str_to_unum(const char *str, int base, UIntType *out)
{
    static_assert(!std::is_signed<UIntType>::value,
                  "Integer type is not unsigned");
    static_assert(std::numeric_limits<UIntType>::max() <= ULLONG_MAX,
                  "Integer type to too large to handle");

    char *end;
    errno = 0;
    auto num = strtoull(str, &end, base);
    if (errno == ERANGE
            || num > std::numeric_limits<UIntType>::max()) {
        errno = ERANGE;
        return false;
    } else if (*str == '\0' || *end != '\0') {
        errno = EINVAL;
        return false;
    }
    *out = static_cast<UIntType>(num);
    return true;
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

        buf[i / 2] = (hi << 4) | lo;
    }

    *data = buf;
    *data_size = size / 2;

    return true;
}

static int search_result_cb(struct MbFile *file, void *userdata,
                            uint64_t offset)
{
    (void) file;
    const char *name = static_cast<char *>(userdata);
    printf("%s: 0x%016" PRIx64 "\n", name, offset);
    return MB_FILE_OK;
}

static bool search(const char *name, struct MbFile *file,
                   int64_t start, int64_t end,
                   size_t bsize, const void *pattern,
                   size_t pattern_size, int64_t max_matches)
{
    int ret = mb_file_search(file, start, end, bsize, pattern, pattern_size,
                             max_matches, &search_result_cb,
                             const_cast<char *>(name));
    if (ret != MB_FILE_OK) {
        fprintf(stderr, "%s: Search failed: %s\n",
                name, mb_file_error_string(file));
        return false;
    }
    return true;
}

static bool search_stdin(int64_t start, int64_t end,
                         size_t bsize, const void *pattern,
                         size_t pattern_size, int64_t max_matches)
{
    MbFile *file = mb_file_new();
    if (!file) {
        fprintf(stderr, "Failed to allocate file: %s\n", strerror(errno));
        return false;
    }

    if (mb_file_open_FILE(file, stdin, false) < 0) {
        fprintf(stderr, "Failed to open stdin: %s\n",
                mb_file_error_string(file));
        mb_file_free(file);
        return false;
    }

    bool ret = search("stdin", file, start, end, bsize, pattern, pattern_size,
                      max_matches);

    mb_file_free(file);
    return ret;
}

static bool search_file(const char *path, int64_t start, int64_t end,
                        size_t bsize, const void *pattern,
                        size_t pattern_size, int64_t max_matches)
{
    MbFile *file = mb_file_new();
    if (!file) {
        fprintf(stderr, "Failed to allocate file: %s\n", strerror(errno));
        return false;
    }

    if (mb_file_open_filename(file, path, MB_FILE_OPEN_READ_ONLY) < 0) {
        fprintf(stderr, "%s: Failed to open file: %s\n",
                path, mb_file_error_string(file));
        mb_file_free(file);
        return false;
    }

    bool ret = search(path, file, start, end, bsize, pattern, pattern_size,
                      max_matches);

    mb_file_free(file);
    return ret;
}

int main(int argc, char *argv[])
{
    int64_t start = -1;
    int64_t end = -1;
    size_t bsize = 0;
    int64_t max_matches = -1;

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
        {"help",         no_argument,       0, 'h'},
        {"num-matches",  required_argument, 0, 'n'},
        {"hex",          required_argument, 0, 'p'},
        {"text",         required_argument, 0, 't'},
        // Arguments without short versions
        {"start-offset", required_argument, 0, OPT_START_OFFSET},
        {"end-offset",   required_argument, 0, OPT_END_OFFSET},
        {"buffer-size",  required_argument, 0, OPT_BUFFER_SIZE},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'n':
            if (!str_to_snum(optarg, 10, &max_matches)) {
                fprintf(stderr, "Invalid value for -n/--num-matches: %s\n",
                        optarg);
                return EXIT_FAILURE;
            }
            break;

        case 'p':
            hex_pattern = optarg;
            break;

        case 't':
            text_pattern = optarg;
            break;

        case OPT_START_OFFSET:
            if (!str_to_snum(optarg, 0, &start)) {
                fprintf(stderr, "Invalid value for --start-offset: %s\n",
                        optarg);
                return EXIT_FAILURE;
            }
            break;

        case OPT_END_OFFSET:
            if (!str_to_snum(optarg, 0, &end)) {
                fprintf(stderr, "Invalid value for --end-offset: %s\n",
                        optarg);
                return EXIT_FAILURE;
            }
            break;

        case OPT_BUFFER_SIZE:
            if (!str_to_unum(optarg, 10, &bsize)) {
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
