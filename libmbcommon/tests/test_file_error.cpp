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

#define TEST_EQUALITY(A, B) \
    do { \
        ASSERT_EQ((A), (B)); \
        ASSERT_EQ((B), (A)); \
    } while (0)

TEST(FileErrorTest, CheckErrorCodesComparableToErrorConditions)
{
    TEST_EQUALITY(mb::make_error_code(mb::FileError::ArgumentOutOfRange),
                  mb::FileErrorC::InvalidArgument);
    TEST_EQUALITY(mb::make_error_code(mb::FileError::CannotConvertEncoding),
                  mb::FileErrorC::InvalidArgument);

    TEST_EQUALITY(mb::make_error_code(mb::FileError::InvalidState),
                  mb::FileErrorC::InvalidState);

    TEST_EQUALITY(mb::make_error_code(mb::FileError::UnsupportedRead),
                  mb::FileErrorC::Unsupported);
    TEST_EQUALITY(mb::make_error_code(mb::FileError::UnsupportedWrite),
                  mb::FileErrorC::Unsupported);
    TEST_EQUALITY(mb::make_error_code(mb::FileError::UnsupportedSeek),
                  mb::FileErrorC::Unsupported);
    TEST_EQUALITY(mb::make_error_code(mb::FileError::UnsupportedTruncate),
                  mb::FileErrorC::Unsupported);

    TEST_EQUALITY(mb::make_error_code(mb::FileError::IntegerOverflow),
                  mb::FileErrorC::InternalError);
    TEST_EQUALITY(mb::make_error_code(mb::FileError::BadFileFormat),
                  mb::FileErrorC::InternalError);
}
