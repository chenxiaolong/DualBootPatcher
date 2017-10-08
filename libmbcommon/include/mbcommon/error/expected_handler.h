/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#pragma once

#include "mbcommon/common.h"
#include "mbcommon/error/error_handler.h"
#include "mbcommon/error/expected.h"

namespace mb
{

/*!
 * Handle any errors (if present) in an Expected<T>, then try a recovery path.
 *
 * If the incoming value is a success value it is returned unmodified. If it
 * is a failure value then it the contained error is passed to handle_errors().
 * If handle_errors() is able to handle the error then the RecoveryPath functor
 * is called to supply the final result. If handle_errors() is not able to
 * handle all errors then the unhandled errors are returned.
 *
 * This utility enables the follow pattern:
 *
 *   @code{.cpp}
 *   enum FooStrategy { Aggressive, Conservative };
 *   Expected<Foo> foo(FooStrategy S);
 *
 *   auto result_or_err = handle_expected(
 *       foo(Aggressive),
 *       []() { return foo(Conservative); },
 *       [](AggressiveStrategyError &) {
 *           // Implicitly conusme this - we'll recover by using a conservative
 *           // strategy.
 *       }
 *   );
 *   @endcode
 */
template <typename T, typename RecoveryFtor, typename... HandlerTs>
MB_EXPORT Expected<T> handle_expected(Expected<T> val_or_err,
                                      RecoveryFtor &&recovery_path,
                                      HandlerTs &&... handlers)
{
    if (val_or_err) {
        return val_or_err;
    }

    if (auto err = handle_errors(val_or_err.take_error(),
                                 std::forward<HandlerTs>(handlers)...)) {
        return std::move(err);
    }

    return recovery_path();
}

}
