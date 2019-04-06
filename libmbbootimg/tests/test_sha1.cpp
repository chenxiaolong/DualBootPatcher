/*
 * Copyright (C) 2019  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <gmock/gmock.h>

#include <vector>

#include "mbbootimg/sha1_p.h"

using namespace mb;
using namespace mb::bootimg::detail;
using namespace testing;


TEST(Sha1Test, CheckTestVectors)
{
    ASSERT_THAT(SHA1::hash("abc"_uchars),
                ElementsAre(0xa9, 0x99, 0x3e, 0x36, 0x47,
                            0x06, 0x81, 0x6a, 0xba, 0x3e,
                            0x25, 0x71, 0x78, 0x50, 0xc2,
                            0x6c, 0x9c, 0xd0, 0xd8, 0x9d));

    ASSERT_THAT(SHA1::hash(""_uchars),
                ElementsAre(0xda, 0x39, 0xa3, 0xee, 0x5e,
                            0x6b, 0x4b, 0x0d, 0x32, 0x55,
                            0xbf, 0xef, 0x95, 0x60, 0x18,
                            0x90, 0xaf, 0xd8, 0x07, 0x09));

    ASSERT_THAT(SHA1::hash("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"_uchars),
                ElementsAre(0x84, 0x98, 0x3e, 0x44, 0x1c,
                            0x3b, 0xd2, 0x6e, 0xba, 0xae,
                            0x4a, 0xa1, 0xf9, 0x51, 0x29,
                            0xe5, 0xe5, 0x46, 0x70, 0xf1));

    ASSERT_THAT(SHA1::hash("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmnhijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu"_uchars),
                ElementsAre(0xa4, 0x9b, 0x24, 0x46, 0xa0,
                            0x2c, 0x64, 0x5b, 0xf4, 0x19,
                            0xf9, 0x95, 0xb6, 0x70, 0x91,
                            0x25, 0x3a, 0x04, 0xa2, 0x59));

    std::vector<unsigned char> buf(1'000'000);
    std::fill(buf.begin(), buf.end(), 'a');

    ASSERT_THAT(SHA1::hash(buf),
                ElementsAre(0x34, 0xaa, 0x97, 0x3c, 0xd4,
                            0xc4, 0xda, 0xa4, 0xf6, 0x1e,
                            0xeb, 0x2b, 0xdb, 0xad, 0x27,
                            0x31, 0x65, 0x34, 0x01, 0x6f));
}