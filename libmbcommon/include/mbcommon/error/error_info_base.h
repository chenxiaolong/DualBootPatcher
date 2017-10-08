/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#pragma once

#include <string>

#include "mbcommon/common.h"

namespace mb
{

/*!
 * Base class for error info classes. Do not extend this directly: Extend
 * the ErrorInfo template subclass instead.
 */
class MB_EXPORT ErrorInfoBase
{
public:
    virtual ~ErrorInfoBase() = default;

    /*!
     * Return the error message as a string.
     */
    virtual std::string message() const = 0;

    /*!
     * Returns the class ID for this type.
     */
    static const void * class_id();

    /*!
     * Returns the class ID for the dynamic type of this ErrorInfoBase instance.
     */
    virtual const void * dynamic_class_id() const = 0;

    /*!
     * Check whether this instance is a subclass of the class identified by id.
     */
    virtual bool is_a(const void *const id) const;

    // Check whether this instance is a subclass of ErrorInfoT.
    template <typename ErrorInfoT>
    bool is_a() const
    {
        return is_a(ErrorInfoT::class_id());
    }

private:
    static char ID;
};

}
