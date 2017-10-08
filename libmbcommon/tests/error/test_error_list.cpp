/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include <gtest/gtest.h>

#include "mbcommon/error/error.h"
#include "mbcommon/error/error_handler.h"

#include "type/custom_error.h"

using namespace mb;

TEST(ErrorListTest, CheckAppendingItemToList)
{
    auto error = join_errors(join_errors(make_error<CustomError>(7),
                                         make_error<CustomError>(7)),
                             make_error<CustomError>(7));

    int sum = 0;
    ASSERT_FALSE(handle_errors(
        std::move(error),
        [&](const CustomError &ce) {
            sum += ce.get_info();
        }
    ));

    ASSERT_EQ(sum, 21);
}

TEST(ErrorListTest, CheckPrependingItemToList)
{
    auto error = join_errors(make_error<CustomError>(7),
                             join_errors(make_error<CustomError>(7),
                                         make_error<CustomError>(7)));

    int sum = 0;
    ASSERT_FALSE(handle_errors(
        std::move(error),
        [&](const CustomError &ce) {
            sum += ce.get_info();
        }
    ));

    ASSERT_EQ(sum, 21);
}

TEST(ErrorListTest, CheckJoiningTwoLists)
{
    auto error = join_errors(join_errors(make_error<CustomError>(7),
                                         make_error<CustomError>(7)),
                             join_errors(make_error<CustomError>(7),
                                         make_error<CustomError>(7)));

    int sum = 0;
    ASSERT_FALSE(handle_errors(
        std::move(error),
        [&](const CustomError &ce) {
            sum += ce.get_info();
        }
    ));

    ASSERT_EQ(sum, 28);
}
