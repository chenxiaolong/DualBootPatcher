/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include <gtest/gtest.h>

#include "mbcommon/error/error_handler.h"
#include "mbcommon/error/type/ec_error.h"

TEST(ECErrorTest, HandleErrors)
{
    auto error = mb::make_error<mb::ECError>(
            std::make_error_code(std::errc::bad_file_descriptor));

    mb::Error remnant = mb::handle_errors(
        std::move(error),
        [](const mb::ECError &err) {
            ASSERT_EQ(err.error_code(), std::errc::bad_file_descriptor);
        }
    );

    ASSERT_FALSE(remnant);
}
