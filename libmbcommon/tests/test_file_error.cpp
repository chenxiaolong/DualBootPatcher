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

#include <gmock/gmock.h>

#include "mbcommon/file_error.h"


TEST(FileErrorTest, CheckErrorCodesComparableToErrorConditions)
{
    std::error_code ec;

    ec = mb::make_error_code(mb::FileError::ArgumentOutOfRange);
    ASSERT_EQ(ec, mb::FileError::InvalidArgument);
    ASSERT_EQ(mb::FileError::InvalidArgument, ec);
    ec = mb::make_error_code(mb::FileError::CannotConvertEncoding);
    ASSERT_EQ(ec, mb::FileError::InvalidArgument);
    ASSERT_EQ(mb::FileError::InvalidArgument, ec);

    ec = mb::make_error_code(mb::FileError::UnsupportedRead);
    ASSERT_EQ(ec, mb::FileError::Unsupported);
    ASSERT_EQ(mb::FileError::Unsupported, ec);
    ec = mb::make_error_code(mb::FileError::UnsupportedWrite);
    ASSERT_EQ(ec, mb::FileError::Unsupported);
    ASSERT_EQ(mb::FileError::Unsupported, ec);
    ec = mb::make_error_code(mb::FileError::UnsupportedSeek);
    ASSERT_EQ(ec, mb::FileError::Unsupported);
    ASSERT_EQ(mb::FileError::Unsupported, ec);
    ec = mb::make_error_code(mb::FileError::UnsupportedTruncate);
    ASSERT_EQ(ec, mb::FileError::Unsupported);
    ASSERT_EQ(mb::FileError::Unsupported, ec);
}
