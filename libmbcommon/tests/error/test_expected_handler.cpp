/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include <gtest/gtest.h>

#include "mbcommon/error/expected_handler.h"

#include "type/custom_error.h"

using namespace mb;

TEST(ExpectedHandlerTest, HandleExpectedSuccess)
{
    auto val_or_err = handle_expected(
        Expected<int>(42),
        []() {
            return Expected<int>(43);
        }
    );

    ASSERT_TRUE(!!val_or_err);
    ASSERT_EQ(*val_or_err, 42);
}

enum class FooStrategy
{
    Aggressive,
    Conservative
};

static Expected<int> foo(FooStrategy S)
{
    if (S == FooStrategy::Aggressive) {
        return make_error<CustomError>(7);
    }
    return 42;
}

TEST(ExpectedHandlerTest, HandleExpectedUnhandledError)
{
    auto val_or_err = handle_expected(
        foo(FooStrategy::Aggressive),
        []() {
            return foo(FooStrategy::Conservative);
        }
    );

    ASSERT_FALSE(!!val_or_err);
    auto err = val_or_err.take_error();
    ASSERT_TRUE(err.is_a<CustomError>());
}

TEST(ExpectedHandlerTest, HandleExpectedHandledError)
{
    auto val_or_err = handle_expected(
        foo(FooStrategy::Aggressive),
        []() {
            return foo(FooStrategy::Conservative);
        },
        [](const CustomError &) {
            // Ignore
        }
    );

    ASSERT_TRUE(!!val_or_err);
    ASSERT_EQ(*val_or_err, 42);
}
