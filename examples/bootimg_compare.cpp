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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <getopt.h>

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"

static void usage(FILE *stream, const char *prog_name)
{
    fprintf(stream, "Usage: %s <file1> <file2>\n"
                    "\n"
                    "Exits with:\n"
                    "  0 if boot images are equal\n"
                    "  1 if an error occurs\n"
                    "  2 if boot images are not equal\n",
                    prog_name);
}

int main(int argc, char *argv[])
{
    int opt;

    static const char short_options[] = "h";

    static struct option long_options[] = {
        {"help", no_argument, nullptr, 'h'},
        {nullptr, 0, nullptr, 0},
    };

    int long_index = 0;

    while ((opt = getopt_long(argc, argv, short_options,
                              long_options, &long_index)) != -1) {
        switch (opt) {
        case 'h':
            usage(stdout, argv[0]);
            return EXIT_SUCCESS;

        default:
            usage(stderr, argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (argc - optind != 2) {
        usage(stderr, argv[0]);
        return EXIT_FAILURE;
    }

    using namespace mb::bootimg;

    const char *filename1 = argv[optind];
    const char *filename2 = argv[optind + 1];

    Reader reader1;
    Reader reader2;
    Header header1;
    Header header2;
    Entry entry1;
    Entry entry2;
    size_t entries = 0;

    // Set up reader formats
    auto ret = reader1.enable_format_all();
    if (!ret) {
        fprintf(stderr, "Failed to enable all boot image formats: %s\n",
                ret.error().message().c_str());
        return EXIT_FAILURE;
    }
    ret = reader2.enable_format_all();
    if (!ret) {
        fprintf(stderr, "Failed to enable all boot image formats: %s\n",
                ret.error().message().c_str());
        return EXIT_FAILURE;
    }

    // Open boot images
    ret = reader1.open_filename(filename1);
    if (!ret) {
        fprintf(stderr, "%s: Failed to open boot image for reading: %s\n",
                filename1, ret.error().message().c_str());
        return EXIT_FAILURE;
    }
    ret = reader2.open_filename(filename2);
    if (!ret) {
        fprintf(stderr, "%s: Failed to open boot image for reading: %s\n",
                filename2, ret.error().message().c_str());
        return EXIT_FAILURE;
    }

    // Read headers
    ret = reader1.read_header(header1);
    if (!ret) {
        fprintf(stderr, "%s: Failed to read header: %s\n",
                filename1, ret.error().message().c_str());
        return EXIT_FAILURE;
    }
    ret = reader2.read_header(header2);
    if (!ret) {
        fprintf(stderr, "%s: Failed to read header: %s\n",
                filename2, ret.error().message().c_str());
        return EXIT_FAILURE;
    }

    // Compare headers
    if (header1 != header2) {
        return 2;
    }

    // Count entries in first boot image
    {
        while (true) {
            ret = reader1.read_entry(entry1);
            if (!ret) {
                if (ret.error() == ReaderError::EndOfEntries) {
                    break;
                }
                fprintf(stderr, "%s: Failed to read entry: %s\n",
                        filename1, ret.error().message().c_str());
                return EXIT_FAILURE;
            }
            ++entries;
        }
    }

    // Compare each entry in second image to first
    {
        while (true) {
            ret = reader2.read_entry(entry2);
            if (!ret) {
                if (ret.error() == ReaderError::EndOfEntries) {
                    break;
                }
                fprintf(stderr, "%s: Failed to read entry: %s",
                        filename2, ret.error().message().c_str());
                return EXIT_FAILURE;
            }

            if (entries == 0) {
                // Too few entries in second image
                return 2;
            }
            --entries;

            // Find the same entry in first image
            ret = reader1.go_to_entry(entry1, *entry2.type());
            if (!ret) {
                if (ret.error() == ReaderError::EndOfEntries) {
                    // Cannot be equal if entry is missing
                    return 2;
                } else {
                    fprintf(stderr, "%s: Failed to seek to entry: %s\n",
                            filename1, ret.error().message().c_str());
                    return EXIT_FAILURE;
                }
            }

            // Compare data
            char buf1[10240];
            char buf2[10240];

            while (true) {
                auto n1 = reader1.read_data(buf1, sizeof(buf1));
                if (!n1) {
                    fprintf(stderr, "%s: Failed to read data: %s\n", filename1,
                            n1.error().message().c_str());
                    return EXIT_FAILURE;
                } else if (n1.value() == 0) {
                    break;
                }

                auto n2 = reader2.read_data(buf2, n1.value());
                if (!n2) {
                    fprintf(stderr, "%s: Failed to read data: %s\n", filename2,
                            n2.error().message().c_str());
                    return EXIT_FAILURE;
                }

                if (n1.value() != n2.value()
                        || memcmp(buf1, buf2, n1.value()) != 0) {
                    // Data is not equivalent
                    return 2;
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
