/*
 * Copyright (C) 2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include <cstring>

#include <gmock/gmock.h>

int main(int argc, char *argv[])
{
    if (argc >= 2) {
        if (strcmp(argv[1], "echo") == 0) {
            for (int i = 2; i < argc; ++i) {
                if (i > 2) {
                    fputc(' ', stdout);
                }
                fputs(argv[i], stdout);
            }
            fputc('\n', stdout);
            return EXIT_SUCCESS;
        } else if (strcmp(argv[1], "true") == 0) {
            return EXIT_SUCCESS;
        }
    }

    testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}
