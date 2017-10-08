/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include <gtest/gtest.h>

#include "mbcommon/error/error_handler.h"
#include "mbcommon/error/expected.h"

#include "type/custom_error.h"

using namespace mb;

TEST(ExpectedTest, CheckExpectedInSuccessState)
{
    Expected<int> a = 7;
    ASSERT_TRUE(!!a);
    ASSERT_EQ(*a, 7);
}

TEST(ExpectedTest, CheckExpectedInErrorState)
{
    Expected<int> a = make_error<CustomError>(7);
    ASSERT_FALSE(!!a);
    auto error = a.take_error();
    ASSERT_TRUE(!!error);

    ASSERT_FALSE(handle_errors(
        a.take_error(),
        [](const CustomError &err) {
            ASSERT_EQ(err.get_info(), 7);
        }
    ));
}

TEST(ExpectedTest, CheckStoringReferenceType)
{
    int a = 7;
    Expected<int &> b = a;
    ASSERT_TRUE(!!b);
    int &c = *b;
    ASSERT_EQ(&a, &c);
}

TEST(ExpectedTest, CheckStoringVoidType)
{
    Expected<void> a;
    ASSERT_TRUE(!!a);
}

TEST(ExpectedTest, CheckStoringVoidTypeWithError)
{
    Expected<void> a = make_error<CustomError>(7);
    ASSERT_FALSE(!!a);
    auto error = a.take_error();
    ASSERT_TRUE(!!error);

    ASSERT_FALSE(handle_errors(
        a.take_error(),
        [](const CustomError &err) {
            ASSERT_EQ(err.get_info(), 7);
        }
    ));
}

TEST(ExpectedTest, CheckCovariance)
{
    class B {};
    class D : public B {};

    Expected<B *> a1(Expected<D *>(nullptr));
    a1 = Expected<D *>(nullptr);

    Expected<std::unique_ptr<B>> a2(Expected<std::unique_ptr<D>>(nullptr));
    a2 = Expected<std::unique_ptr<D>>(nullptr);
}
