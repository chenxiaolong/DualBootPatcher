/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#pragma once

#include <memory>

#include "mbcommon/common.h"
#include "mbcommon/error/error_info_base.h"

namespace mb
{

class ErrorSuccess;

/*!
 * Lightweight error class with error context and mandatory checking.
 *
 * Instances of this class wrap a ErrorInfoBase pointer. Failure states
 * are represented by setting the pointer to a ErrorInfoBase subclass
 * instance containing information describing the failure. Success is
 * represented by a null pointer value.
 *
 * Instances of Error also contains a 'checked' flag, which must be set
 * before the destructor is called, otherwise the destructor will trigger a
 * runtime error. This enforces at runtime the requirement that all Error
 * instances be checked or returned to the caller.
 *
 * There are two ways to set the checked flag, depending on what state the
 * Error instance is in. For Error instances indicating success, it
 * is sufficient to invoke the boolean conversion operator. E.g.:
 *
 *   @code{.cpp}
 *   Error foo(<...>);
 *
 *   if (auto e = foo(<...>))
 *       return e; // <- Return e if it is in the error state.
 *   // We have verified that e was in the success state. It can now be safely
 *   // destroyed.
 *   @endcode
 *
 * A success value *can not* be dropped. For example, just calling 'foo(<...>)'
 * without testing the return value will raise a runtime error, even if foo
 * returns success.
 *
 * For Error instances representing failure, you must use the handle_errors()
 * function with a typed handler. E.g.:
 *
 *   @code{.cpp}
 *   class MyErrorInfo : public ErrorInfo<MyErrorInfo> {
 *       // Custom error info.
 *   };
 *
 *   Error foo(<...>) { return make_error<MyErrorInfo>(...); }
 *
 *   auto e = foo(<...>); // <- foo returns failure with MyErrorInfo.
 *   auto new_e = handle_errors(
 *       e,
 *       [](const MyErrorInfo &m) {
 *           // Deal with the error.
 *       },
 *       [](std::unique_ptr<OtherError> m) -> Error {
 *           if (can_handle(*m)) {
 *               // handle error.
 *               return Error::success();
 *           }
 *           // Couldn't handle this error instance. Pass it up the stack.
 *           return Error(std::move(m));
 *       }
 *   );
 *   // Note - we must check or return new_e in case any of the handlers
 *   // returned a new error.
 *   @endcode
 *
 * *All* Error instances must be checked before destruction, even if
 * they're moved-assigned or constructed from Success values that have already
 * been checked. This enforces checking through all levels of the call stack.
 */
class MB_EXPORT MB_NO_DISCARD Error
{
    // ErrorList needs to be able to yank ErrorInfoBase pointers out of this
    // class to add to the error list.
    friend class ErrorList;

    // handle_errors() needs to be able to set the checked flag.
    template <typename... HandlerTs>
    friend Error handle_errors(Error e, HandlerTs &&... handlers);

    // Expected<T> needs to be able to steal the payload when constructed from
    // an error.
    template <typename T>
    friend class Expected;

protected:
    /*!
     * Create a success value. Prefer using 'Error::success()' for readability
     */
    Error();

public:
    /*!
     * Create a success value.
     */
    static ErrorSuccess success();

    // Errors are not copy-constructable.
    Error(const Error &Other) = delete;

    /*!
     * Move-construct an error value. The newly constructed error is considered
     * unchecked, even if the source error had been checked. The original error
     * becomes a checked Success value, regardless of its original state.
     */
    Error(Error &&other);

    /*!
     * Create an error value. Prefer using the 'make_error' function, but
     * this constructor can be useful when "re-throwing" errors from handlers.
     */
    Error(std::unique_ptr<ErrorInfoBase> payload);

    // Errors are not copy-assignable.
    Error & operator=(const Error &other) = delete;

    /*!
     * Move-assign an error value. The current error must represent success, you
     * you cannot overwrite an unhandled error. The current error is then
     * considered unchecked. The source error becomes a checked success value,
     * regardless of its original state.
     */
    Error & operator=(Error &&other);

    /*!
     * Destroy an Error. Fails with a call to abort() if the error is
     * unchecked.
     */
    ~Error();

    /*!
     * Bool conversion. Returns true if this Error is in a failure state,
     * and false if it is in an accept state. If the error is in a Success state
     * it will be considered checked.
     */
    explicit operator bool();

    /*!
     * Check whether one error is a subclass of another.
     */
    template <typename ErrT>
    bool is_a() const
    {
        return _payload && _payload->is_a(ErrT::class_id());
    }

    /*!
     * Returns the dynamic class id of this error, or null if this is a success
     * value.
     */
    const void * dynamic_class_id() const;

private:
    std::unique_ptr<ErrorInfoBase> take_payload();

    std::unique_ptr<ErrorInfoBase> _payload;
};

/*!
 * Subclass of Error for the sole purpose of identifying the success path in
 * the type system. This allows to catch invalid conversion to Expected<T> at
 * compile time.
 */
class MB_EXPORT ErrorSuccess : public Error
{
};

inline ErrorSuccess Error::success()
{
    return ErrorSuccess();
}

/*!
 * Make an Error instance representing failure using the given error info type.
 */
template <typename ErrT, typename... ArgTs>
MB_EXPORT Error make_error(ArgTs &&... args)
{
    return {std::make_unique<ErrT>(std::forward<ArgTs>(args)...)};
}

}
