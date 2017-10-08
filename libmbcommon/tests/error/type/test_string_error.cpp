/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include <gtest/gtest.h>

#include "mbcommon/error/error_handler.h"
#include "mbcommon/error/type/string_error.h"

TEST(StringErrorTest, HandleErrors)
{
    auto error = mb::make_error<mb::StringError>("StringErrorMessage");

    mb::Error remnant = mb::handle_errors(
        std::move(error),
        [](const mb::StringError &err) {
            ASSERT_EQ("StringErrorMessage", err.message());
        }
    );

    ASSERT_FALSE(remnant);
}
