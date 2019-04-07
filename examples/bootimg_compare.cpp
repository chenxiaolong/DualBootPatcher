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

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <CLI/CLI.hpp>

#ifdef _WIN32
#  include "mbcommon/locale.h"
#endif

#include "mbbootimg/entry.h"
#include "mbbootimg/header.h"
#include "mbbootimg/reader.h"

using namespace mb;
using namespace mb::bootimg;

#ifdef _WIN32
int wmain(int argc, wchar_t *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    CLI::App app;

    app.footer(
        "\n"
        "Exits with:\n"
        "  0 if boot images are equal\n"
        "  1 if an error occurs\n"
        "  2 if boot images are not equal\n"
    );

    std::string filename1;
    std::string filename2;

    app.add_option("file1", filename1, "First file")
            ->required()
            ->type_name("FILE");
    app.add_option("file2", filename2, "Second file")
            ->required()
            ->type_name("FILE");

    try {
#ifdef _WIN32
        std::vector<std::string> reversed_args;
        for (int i = argc - 1; i > 0; --i) {
            reversed_args.emplace_back(wcs_to_utf8(argv[i]).value());
        }
        app.name(wcs_to_utf8(argv[0]).value());
        app.parse(reversed_args);
#else
        app.parse(argc, argv);
#endif
    } catch (const CLI::ParseError &e) {
        return app.exit(e);
    }

#ifdef _WIN32
    auto native_filename1 = utf8_to_wcs(filename1).value();
    auto native_filename2 = utf8_to_wcs(filename2).value();
#else
    auto &native_filename1 = filename1;
    auto &native_filename2 = filename2;
#endif

    Reader reader1;
    Reader reader2;
    size_t entries = 0;

    // Set up reader formats
    if (auto r = reader1.enable_formats_all(); !r) {
        fprintf(stderr, "Failed to enable all boot image formats: %s\n",
                r.error().message().c_str());
        return EXIT_FAILURE;
    }
    if (auto r = reader2.enable_formats_all(); !r) {
        fprintf(stderr, "Failed to enable all boot image formats: %s\n",
                r.error().message().c_str());
        return EXIT_FAILURE;
    }

    // Open boot images
    if (auto r = reader1.open_filename(native_filename1); !r) {
        fprintf(stderr, "%s: Failed to open boot image for reading: %s\n",
                filename1.c_str(), r.error().message().c_str());
        return EXIT_FAILURE;
    }
    if (auto r = reader2.open_filename(native_filename2); !r) {
        fprintf(stderr, "%s: Failed to open boot image for reading: %s\n",
                filename2.c_str(), r.error().message().c_str());
        return EXIT_FAILURE;
    }

    // Read headers
    auto header1 = reader1.read_header();
    if (!header1) {
        fprintf(stderr, "%s: Failed to read header: %s\n",
                filename1.c_str(), header1.error().message().c_str());
        return EXIT_FAILURE;
    }
    auto header2 = reader2.read_header();
    if (!header2) {
        fprintf(stderr, "%s: Failed to read header: %s\n",
                filename2.c_str(), header2.error().message().c_str());
        return EXIT_FAILURE;
    }

    // Compare headers
    if (header1.value() != header2.value()) {
        return 2;
    }

    // Count entries in first boot image
    {
        while (true) {
            auto entry = reader1.read_entry();
            if (!entry) {
                if (entry.error() == ReaderError::EndOfEntries) {
                    break;
                }
                fprintf(stderr, "%s: Failed to read entry: %s\n",
                        filename1.c_str(), entry.error().message().c_str());
                return EXIT_FAILURE;
            }
            ++entries;
        }
    }

    // Compare each entry in second image to first
    {
        while (true) {
            auto entry2 = reader2.read_entry();
            if (!entry2) {
                if (entry2.error() == ReaderError::EndOfEntries) {
                    break;
                }
                fprintf(stderr, "%s: Failed to read entry: %s",
                        filename2.c_str(), entry2.error().message().c_str());
                return EXIT_FAILURE;
            }

            if (entries == 0) {
                // Too few entries in second image
                return 2;
            }
            --entries;

            // Find the same entry in first image
            auto entry1 = reader1.go_to_entry(entry2.value().type());
            if (!entry1) {
                if (entry1.error() == ReaderError::EndOfEntries) {
                    // Cannot be equal if entry is missing
                    return 2;
                } else {
                    fprintf(stderr, "%s: Failed to seek to entry: %s\n",
                            filename1.c_str(), entry1.error().message().c_str());
                    return EXIT_FAILURE;
                }
            }

            // Compare entries
            if (entry1.value() != entry2.value()) {
                return 2;
            }

            // Compare data
            unsigned char buf1[10240];
            unsigned char buf2[10240];

            while (true) {
                auto n1 = reader1.read_data(buf1);
                if (!n1) {
                    fprintf(stderr, "%s: Failed to read data: %s\n",
                            filename1.c_str(), n1.error().message().c_str());
                    return EXIT_FAILURE;
                }

                auto n2 = reader2.read_data(buf2);
                if (!n2) {
                    fprintf(stderr, "%s: Failed to read data: %s\n",
                            filename2.c_str(), n2.error().message().c_str());
                    return EXIT_FAILURE;
                }

                if (n1.value() == 0 && n2.value() == 0) {
                    break;
                } else if (n1.value() != n2.value()
                        || memcmp(buf1, buf2, n1.value()) != 0) {
                    // Data is not equivalent
                    return 2;
                }
            }
        }
    }

    return EXIT_SUCCESS;
}
