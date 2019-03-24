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

#include <gtest/gtest.h>

#include "mbcommon/version.h"

using namespace mb;

TEST(VersionTest, CompareVersion)
{
    // Empty versions
    ASSERT_EQ(version_compare("", ""), 0);
    ASSERT_LE(version_compare("", "0.0.0"), 0);
    ASSERT_GE(version_compare("0.0.0", ""), 0);

    // Equality
    ASSERT_EQ(version_compare("0", "0"), 0);
    ASSERT_EQ(version_compare("0.0", "0.0"), 0);
    ASSERT_EQ(version_compare("0.0.0", "0.0.0"), 0);

    // Same number of components
    ASSERT_LE(version_compare("0", "1"), 0);
    ASSERT_LE(version_compare("0.0", "1.0"), 0);
    ASSERT_LE(version_compare("0.0.0", "1.0.0"), 0);
    ASSERT_GE(version_compare("1", "0"), 0);
    ASSERT_GE(version_compare("1.0", "0.0"), 0);
    ASSERT_GE(version_compare("1.0.0", "0.0.0"), 0);

    // Fewer components on the LHS
    ASSERT_LE(version_compare("0", "1.0"), 0);
    ASSERT_LE(version_compare("0", "1.0.0"), 0);
    ASSERT_GE(version_compare("1", "0.0"), 0);
    ASSERT_GE(version_compare("1", "0.0.0"), 0);

    // Fewer components on the RHS
    ASSERT_LE(version_compare("0.0", "1"), 0);
    ASSERT_LE(version_compare("0.0.0", "1"), 0);
    ASSERT_GE(version_compare("1.0", "0"), 0);
    ASSERT_GE(version_compare("1.0.0", "0"), 0);

    // Non-numeric components lexicographically compared
    ASSERT_EQ(version_compare("a", "a"), 0);
    ASSERT_LE(version_compare("a", "b"), 0);
    ASSERT_GE(version_compare("b", "a"), 0);
    ASSERT_EQ(version_compare("1.0a", "1.0a"), 0);
    ASSERT_LE(version_compare("1.0a", "1.0b"), 0);
    ASSERT_GE(version_compare("1.0b", "1.0a"), 0);
}
