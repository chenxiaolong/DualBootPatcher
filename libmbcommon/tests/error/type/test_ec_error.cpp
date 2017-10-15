/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include <gtest/gtest.h>

#ifdef _WIN32
#  include <winerror.h>
#endif

#include "mbcommon/error/error_handler.h"
#include "mbcommon/error/type/ec_error.h"

TEST(ECErrorTest, HandleErrors)
{
    auto error = mb::make_error<mb::ECError>(
            std::make_error_code(std::errc::bad_file_descriptor));

    ASSERT_FALSE(mb::handle_errors(
        std::move(error),
        [](const mb::ECError &err) {
            ASSERT_EQ(err.error_code(), std::errc::bad_file_descriptor);
        }
    ));
}

TEST(ECErrorTest, ConvertFromErrno)
{
    auto error = mb::make_error<mb::ECError>(mb::ECError::from_errno(ENOENT));

    ASSERT_FALSE(mb::handle_errors(
        std::move(error),
        [](const mb::ECError &err) {
            ASSERT_EQ(err.error_code(), std::errc::no_such_file_or_directory);
        }
    ));
}

#ifdef _WIN32
TEST(ECErrorTest, ConvertFromWin32Error)
{
    auto error = mb::make_error<mb::ECError>(mb::ECError::from_win32_error(
            ERROR_FILE_NOT_FOUND));

    ASSERT_FALSE(mb::handle_errors(
        std::move(error),
        [](const mb::ECError &err) {
            ASSERT_EQ(err.error_code(), std::errc::no_such_file_or_directory);
        }
    ));
}
#endif
