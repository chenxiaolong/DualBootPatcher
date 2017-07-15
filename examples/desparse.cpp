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

#include <memory>
#include <string>

#include <cstdlib>
#include <cstdio>

#include "mbcommon/file/standard.h"
#include "mbsparse/sparse.h"

int main(int argc, char *argv[])
{
    if (argc != 3) {
        std::fprintf(stderr, "Usage: %s <input file> <output file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *input_path = argv[1];
    const char *output_path = argv[2];

    mb::StandardFile input_file;
    mb::StandardFile output_file;
    mb::sparse::SparseFile sparse_file;

    if (input_file.open(input_path, mb::FileOpenMode::READ_ONLY)
            != mb::FileStatus::OK) {
        fprintf(stderr, "%s: Failed to open for reading: %s\n",
                input_path, input_file.error_string().c_str());
        return EXIT_FAILURE;
    }

    if (sparse_file.open(&input_file) != mb::FileStatus::OK) {
        fprintf(stderr, "%s: %s\n",
                input_path, sparse_file.error_string().c_str());
        return EXIT_FAILURE;
    }

    if (output_file.open(output_path, mb::FileOpenMode::WRITE_ONLY)
            != mb::FileStatus::OK) {
        fprintf(stderr, "%s: Failed to open for writing: %s\n",
                output_path, output_file.error_string().c_str());
        return EXIT_FAILURE;
    }

    size_t n_read;
    char buf[10240];
    mb::FileStatus ret;
    while ((ret = sparse_file.read(buf, sizeof(buf), &n_read))
            == mb::FileStatus::OK && n_read > 0) {
        char *ptr = buf;
        size_t n_written;

        while (n_read > 0) {
            if (output_file.write(buf, n_read, &n_written)
                    != mb::FileStatus::OK) {
                fprintf(stderr, "%s: Failed to write file: %s\n",
                        output_path, output_file.error_string().c_str());
                return EXIT_FAILURE;
            }
            n_read -= n_written;
            ptr += n_written;
        }
    }

    if (ret != mb::FileStatus::OK) {
        fprintf(stderr, "%s: Failed to read file: %s\n",
                input_path, sparse_file.error_string().c_str());
        return EXIT_FAILURE;
    }

    if (output_file.close() != mb::FileStatus::OK) {
        fprintf(stderr, "%s: Failed to close file: %s\n",
                output_path, output_file.error_string().c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
