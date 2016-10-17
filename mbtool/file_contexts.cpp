/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "file_contexts.h"

#include "mbutil/autoclose/file.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <cstdlib>

#include <getopt.h>


#define SELINUX_MAGIC_COMPILED_FCONTEXT         0xf97cff8a

#define SELINUX_COMPILED_FCONTEXT_NOPCRE_VERS   1
#define SELINUX_COMPILED_FCONTEXT_PCRE_VERS     2
#define SELINUX_COMPILED_FCONTEXT_MODE          3
#define SELINUX_COMPILED_FCONTEXT_PREFIX_LEN    4

#define SELINUX_COMPILED_FCONTEXT_MAX_VERS \
    SELINUX_COMPILED_FCONTEXT_PREFIX_LEN

namespace mb
{

typedef std::vector<std::pair<std::string, std::string>> ContextsList;

typedef std::unique_ptr<char, void (*)(void *)> ScopedCharPtr;

static bool skip_pcre(FILE *fp)
{
    size_t n;
    uint32_t entry_len;

    // Skip PCRE regex
    n = fread(&entry_len, 1, sizeof(entry_len), fp);
    if (n != sizeof(entry_len) || entry_len == 0) {
        return false;
    }

    if (fseek(fp, entry_len, SEEK_CUR) != 0) {
        return false;
    }

    // Skip PCRE study data
    n = fread(&entry_len, 1, sizeof(entry_len), fp);
    if (n != sizeof(entry_len) || entry_len == 0) {
        return false;
    }

    if (fseek(fp, entry_len, SEEK_CUR) != 0) {
        return false;
    }

    return true;
}

static bool load_file(FILE *fp, ContextsList *list)
{
    size_t n;
    uint32_t magic;
    uint32_t version;
    uint32_t entry_len;
    uint32_t stem_map_len;
    uint32_t regex_array_len;

    // Check magic
    n = fread(&magic, 1, sizeof(magic), fp);
    if (n != sizeof(magic) || magic != SELINUX_MAGIC_COMPILED_FCONTEXT) {
        return false;
    }

    // Check version
    n = fread(&version, 1, sizeof(version), fp);
    if (n != sizeof(version) || version > SELINUX_COMPILED_FCONTEXT_MAX_VERS) {
        return false;
    }

    // Skip regex version
    if (version >= SELINUX_COMPILED_FCONTEXT_PCRE_VERS) {
        n = fread(&entry_len, 1, sizeof(entry_len), fp);
        if (n != sizeof(entry_len)) {
            return false;
        }

        if (fseek(fp, entry_len, SEEK_CUR) != 0) {
            return false;
        }
    }

    // Skip stem map
    n = fread(&stem_map_len, 1, sizeof(stem_map_len), fp);
    if (n != sizeof(stem_map_len) || stem_map_len == 0) {
        return false;
    }

    for (uint32_t i = 0; i < stem_map_len; ++i) {
        uint32_t stem_len;

        // Length does not include NULL-terminator
        n = fread(&stem_len, 1, sizeof(stem_len), fp);
        if (n != sizeof(stem_len) || stem_len == 0) {
            return false;
        }

        if (stem_len < UINT32_MAX) {
            if (fseek(fp, stem_len + 1, SEEK_CUR) != 0) {
                return false;
            }
        } else {
            return false;
        }
    }

    // Load regexes
    n = fread(&regex_array_len, 1, sizeof(regex_array_len), fp);
    if (n != sizeof(regex_array_len) || regex_array_len == 0) {
        return false;
    }

    for (uint32_t i = 0; i < regex_array_len; ++i) {
        // Read context string
        n = fread(&entry_len, 1, sizeof(entry_len), fp);
        if (n != sizeof(uint32_t) || entry_len == 0) {
            return false;
        }

        ScopedCharPtr context(static_cast<char *>(malloc(entry_len)), &free);
        if (!context) {
            return false;
        }

        n = fread(context.get(), 1, entry_len, fp);
        if (n != entry_len) {
            return false;
        }

        if (context.get()[entry_len - 1] != '\0') {
            return false;
        }

        // Read regex string
        n = fread(&entry_len, 1, sizeof(entry_len), fp);
        if (n != sizeof(entry_len) || entry_len == 0) {
            return false;
        }

        ScopedCharPtr regex(static_cast<char *>(malloc(entry_len)), &free);
        if (!regex) {
            return false;
        }

        n = fread(regex.get(), 1, entry_len, fp);
        if (n != entry_len) {
            return false;
        }

        if (regex.get()[entry_len - 1] != '\0') {
            return false;
        }

        // Skip mode
        if (version >= SELINUX_COMPILED_FCONTEXT_MODE) {
            if (fseek(fp, sizeof(uint32_t), SEEK_CUR) != 0) {
                return false;
            }
        } else {
            if (fseek(fp, sizeof(mode_t), SEEK_CUR) != 0) {
                return false;
            }
        }

        // Skip stem ID
        if (fseek(fp, sizeof(uint32_t), SEEK_CUR) != 0) {
            return false;
        }

        // Skip meta chars
        if (fseek(fp, sizeof(uint32_t), SEEK_CUR) != 0) {
            return false;
        }

        if (version >= SELINUX_COMPILED_FCONTEXT_PREFIX_LEN) {
            // Skip prefix length
            if (fseek(fp, sizeof(uint32_t), SEEK_CUR) != 0) {
                return false;
            }
        }

        // Skip PCRE stuff
        if (!skip_pcre(fp)) {
            return false;
        }

        list->emplace_back(context.get(), regex.get());
    }

    return true;
}

FileContextsResult patch_file_contexts(const char *source_path,
                                       const char *target_path)
{
    ContextsList list;

    autoclose::file fp_old(autoclose::fopen(source_path, "rb"));
    if (!fp_old) {
        return FileContextsResult::ERRNO;
    }

    if (!load_file(fp_old.get(), &list)) {
        return FileContextsResult::PARSE_ERROR;
    }

    fp_old.reset();

    autoclose::file fp_new(autoclose::fopen(target_path, "w"));
    if (!fp_new) {
        return FileContextsResult::ERRNO;
    }

    for (auto const &pair : list) {
        auto const &context = pair.first;
        auto const &regex = pair.second;

        if (fputs(regex.c_str(), fp_new.get()) == EOF
                || fputc(' ', fp_new.get()) == EOF
                || fputs(context.c_str(), fp_new.get()) == EOF
                || fputc('\n', fp_new.get()) == EOF) {
            return FileContextsResult::ERRNO;
        }
    }

    return FileContextsResult::OK;
}

static void file_contexts_usage(FILE *stream)
{
    fprintf(stream,
            "Usage: file_contexts [OPTION]... <input> <output>\n\n"
            "Options:\n"
            "  -h, --help       Display this help message\n");
}

int file_contexts_main(int argc, char *argv[])
{
    int opt;

    static const char *short_options = "h";

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            file_contexts_usage(stdout);
            return EXIT_SUCCESS;

        default:
            file_contexts_usage(stderr);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind != 2) {
        file_contexts_usage(stderr);
        return EXIT_FAILURE;
    }

    const char *input = argv[optind];
    const char *output = argv[optind + 1];

    auto ret = patch_file_contexts(input, output);
    return ret == FileContextsResult::OK ? EXIT_SUCCESS : EXIT_FAILURE;
}

}
