/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include <gtest/gtest.h>

#include "mbcommon/error/error.h"

#include "type/custom_error.h"

using namespace mb;

namespace
{

TEST(ErrorTest, CheckSuccessState)
{
    Error e = Error::success();
    ASSERT_FALSE(!!e);
}

TEST(ErrorTest, CheckErrorState)
{
    Error e = make_error<CustomError>(7);
    ASSERT_TRUE(!!e);
}

TEST(ErrorTest, IsAHandling)
{
    Error e = make_error<CustomError>(1);
    Error f = make_error<CustomSubError>(1, 2);
    Error g = Error::success();

    ASSERT_TRUE(e.is_a<CustomError>());
    ASSERT_FALSE(e.is_a<CustomSubError>());
    ASSERT_TRUE(f.is_a<CustomError>());
    ASSERT_TRUE(f.is_a<CustomSubError>());
    ASSERT_FALSE(g.is_a<CustomError>());
}

}
