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

using namespace mb;

TEST(FileErrorTest, CheckErrorCodesComparableToErrorConditions)
{
    TEST_EQUALITY(make_error_code(FileError::ArgumentOutOfRange),
                  FileErrorC::InvalidArgument);
    TEST_EQUALITY(make_error_code(FileError::CannotConvertEncoding),
                  FileErrorC::InvalidArgument);

    TEST_EQUALITY(make_error_code(FileError::InvalidState),
                  FileErrorC::InvalidState);

    TEST_EQUALITY(make_error_code(FileError::UnsupportedRead),
                  FileErrorC::Unsupported);
    TEST_EQUALITY(make_error_code(FileError::UnsupportedWrite),
                  FileErrorC::Unsupported);
    TEST_EQUALITY(make_error_code(FileError::UnsupportedSeek),
                  FileErrorC::Unsupported);
    TEST_EQUALITY(make_error_code(FileError::UnsupportedTruncate),
                  FileErrorC::Unsupported);

    TEST_EQUALITY(make_error_code(FileError::IntegerOverflow),
                  FileErrorC::InternalError);
}
