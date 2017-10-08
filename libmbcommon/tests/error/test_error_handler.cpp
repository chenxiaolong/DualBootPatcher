/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include <gtest/gtest.h>

#include "mbcommon/error/error.h"
#include "mbcommon/error/error_handler.h"

#include "type/custom_error.h"

using namespace mb;

TEST(ErrorHandlerTest, HandleCustomError)
{
    int caught_error_info = 0;
    ASSERT_FALSE(handle_errors(
        make_error<CustomError>(42),
        [&](const CustomError &ce) {
            caught_error_info = ce.get_info();
        }
    ));

    ASSERT_EQ(caught_error_info, 42);
}

static void handle_custom_error_void(const CustomError &ce)
{
    (void) ce;
}

static Error handle_custom_error(const CustomError &ce)
{
    (void) ce;
    return Error::success();
}

static void handle_custom_error_up_void(std::unique_ptr<CustomError> ce)
{
    (void) ce;
}

static Error handle_custom_error_up(std::unique_ptr<CustomError> ce)
{
    (void) ce;
    return Error::success();
}

TEST(ErrorHandlerTest, HandlerTypeDeduction)
{
    ASSERT_FALSE(handle_errors(
        make_error<CustomError>(42),
        [](const CustomError &ce) {
            (void) ce;
        }
    ));

    ASSERT_FALSE(handle_errors(
        make_error<CustomError>(42),
        [](const CustomError &ce) mutable {
            (void) ce;
        }
    ));

    ASSERT_FALSE(handle_errors(
        make_error<CustomError>(42),
        [](const CustomError &ce) -> Error {
            (void) ce;
            return Error::success();
        }
    ));

    ASSERT_FALSE(handle_errors(
        make_error<CustomError>(42),
        [](const CustomError &ce) mutable -> Error {
            (void) ce;
            return Error::success();
        }
    ));

    ASSERT_FALSE(handle_errors(
        make_error<CustomError>(42),
        [](CustomError &ce) {
            (void) ce;
        }
    ));

    ASSERT_FALSE(handle_errors(
        make_error<CustomError>(42),
        [](CustomError &ce) mutable {
            (void) ce;
        }
    ));

    ASSERT_FALSE(handle_errors(
        make_error<CustomError>(42),
        [](CustomError &ce) -> Error {
            (void) ce;
            return Error::success();
        }
    ));

    ASSERT_FALSE(handle_errors(
        make_error<CustomError>(42),
        [](CustomError &ce) mutable -> Error {
            (void) ce;
            return Error::success();
        }
    ));

    ASSERT_FALSE(handle_errors(
        make_error<CustomError>(42),
        [](std::unique_ptr<CustomError> ce) {
            (void) ce;
        }
    ));

    ASSERT_FALSE(handle_errors(
        make_error<CustomError>(42),
        [](std::unique_ptr<CustomError> ce) mutable {
            (void) ce;
        }
    ));

    ASSERT_FALSE(handle_errors(
        make_error<CustomError>(42),
        [](std::unique_ptr<CustomError> ce) -> Error {
            (void) ce;
            return Error::success();
        }
    ));

    ASSERT_FALSE(handle_errors(
        make_error<CustomError>(42),
        [](std::unique_ptr<CustomError> ce) mutable -> Error {
            (void) ce;
            return Error::success();
        }
    ));

    ASSERT_FALSE(handle_errors(make_error<CustomError>(42),
                               handle_custom_error_void));

    ASSERT_FALSE(handle_errors(make_error<CustomError>(42),
                               handle_custom_error));

    ASSERT_FALSE(handle_errors(make_error<CustomError>(42),
                               handle_custom_error_up_void));

    ASSERT_FALSE(handle_errors(make_error<CustomError>(42),
                               handle_custom_error_up));
}

TEST(ErrorHandlerTest, HandleCustomErrorWithCustomBaseClass)
{
    int caught_error_info = 0;
    int caught_error_extra_info = 0;
    ASSERT_FALSE(handle_errors(
        make_error<CustomSubError>(42, 7),
        [&](const CustomSubError &se) {
            caught_error_info = se.get_info();
            caught_error_extra_info = se.get_extra_info();
        }
    ));

    ASSERT_EQ(caught_error_info, 42);
    ASSERT_EQ(caught_error_extra_info, 7);
}

TEST(ErrorHandlerTest, FirstHandlerOnly)
{
    int dummy_info = 0;
    int caught_error_info = 0;
    int caught_error_extra_info = 0;

    ASSERT_FALSE(handle_errors(
        make_error<CustomSubError>(42, 7),
        [&](const CustomSubError &se) {
            caught_error_info = se.get_info();
            caught_error_extra_info = se.get_extra_info();
        },
        [&](const CustomError &ce) {
            dummy_info = ce.get_info();
        }
    ));

    ASSERT_EQ(caught_error_info, 42);
    ASSERT_EQ(caught_error_extra_info, 7);
    ASSERT_EQ(dummy_info, 0);
}

TEST(ErrorHandlerTest, HandlerShadowing)
{
    int caught_error_info = 0;
    int dummy_info = 0;
    int dummy_extra_info = 0;

    ASSERT_FALSE(handle_errors(
        make_error<CustomSubError>(42, 7),
        [&](const CustomError &ce) {
            caught_error_info = ce.get_info();
        },
        [&](const CustomSubError &se) {
            dummy_info = se.get_info();
            dummy_extra_info = se.get_extra_info();
        }
    ));

    ASSERT_EQ(caught_error_info, 42);
    ASSERT_EQ(dummy_info, 0);
    ASSERT_EQ(dummy_extra_info, 0);
}

TEST(ErrorHandlerTest, CheckHandlingJoinedErrorsPreservesOrdering)
{
    int custom_error_info1 = 0;
    int custom_error_info2 = 0;
    int custom_error_extra_info = 0;
    Error error = join_errors(make_error<CustomError>(7),
                              make_error<CustomSubError>(42, 7));

    ASSERT_FALSE(handle_errors(
        std::move(error),
        [&](const CustomSubError &se) {
            custom_error_info2 = se.get_info();
            custom_error_extra_info = se.get_extra_info();
        },
        [&](const CustomError &ce) {
            // Ensure order is preserved
            ASSERT_EQ(custom_error_info2, 0)
                    << "Failed to preserve ordering";
            custom_error_info1 = ce.get_info();
        }
    ));

    ASSERT_EQ(custom_error_info1, 7);
    ASSERT_EQ(custom_error_info2, 42);
    ASSERT_EQ(custom_error_extra_info, 7);
}

TEST(ErrorHandlerTest, CatchErrorFromHandler)
{
    int error_info = 0;

    Error e = handle_errors(
        make_error<CustomError>(7),
        [&](std::unique_ptr<CustomError> ce) {
            return Error(std::move(ce));
        }
    );

    ASSERT_FALSE(handle_errors(
        std::move(e),
        [&](const CustomError &ce) {
            error_info = ce.get_info();
        }
    ));

    ASSERT_EQ(error_info, 7);
}
