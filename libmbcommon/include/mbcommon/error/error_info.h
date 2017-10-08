/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#pragma once

#include "mbcommon/common.h"
#include "mbcommon/error/error_info_base.h"

namespace mb
{

/*!
 * Base class for user error types. Users should declare their error types like:
 *
 *   @code{.cpp}
 *   class MyError : public ErrorInfo<MyError>
 *   {
 *       ...
 *   };
 *   @endcode
 *
 * This class provides an implementation of the ErrorInfoBase::class_id()
 * method, which is used by the Error RTTI system.
 */
template <typename ThisErrT, typename ParentErrT = ErrorInfoBase>
class MB_EXPORT ErrorInfo : public ParentErrT
{
public:
    static const void * class_id()
    {
        return &ThisErrT::ID;
    }

    const void * dynamic_class_id() const override
    {
        return &ThisErrT::ID;
    }

    bool is_a(const void *const id) const override
    {
        return id == class_id() || ParentErrT::is_a(id);
    }
};

}
