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
#include "mbcommon/file_util.h"
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

    auto open_ret = input_file.open(input_path, mb::FileOpenMode::ReadOnly);
    if (!open_ret) {
        fprintf(stderr, "%s: Failed to open for reading: %s\n",
                input_path, open_ret.error().message().c_str());
        return EXIT_FAILURE;
    }

    open_ret = sparse_file.open(&input_file);
    if (!open_ret) {
        fprintf(stderr, "%s: %s\n",
                input_path, open_ret.error().message().c_str());
        return EXIT_FAILURE;
    }

    open_ret = output_file.open(output_path, mb::FileOpenMode::WriteOnly);
    if (!open_ret) {
        fprintf(stderr, "%s: Failed to open for writing: %s\n",
                output_path, open_ret.error().message().c_str());
        return EXIT_FAILURE;
    }

    unsigned char buf[10240];

    while (true) {
        auto n_read = sparse_file.read(buf);
        if (!n_read) {
            fprintf(stderr, "%s: Failed to read file: %s\n",
                    input_path, n_read.error().message().c_str());
            return EXIT_FAILURE;
        } else if (n_read.value() == 0) {
            break;
        }

        if (auto r = mb::file_write_exact(
                output_file, mb::span(buf, n_read.value())); !r) {
            fprintf(stderr, "%s: Failed to write file: %s\n",
                    output_path, r.error().message().c_str());
            return EXIT_FAILURE;
        }
    }

    auto close_ret = output_file.close();
    if (!close_ret) {
        fprintf(stderr, "%s: Failed to close file: %s\n",
                output_path, close_ret.error().message().c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
