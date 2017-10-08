/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#pragma once

#include <vector>

#include "mbcommon/error/error.h"
#include "mbcommon/error/error_info.h"

namespace mb
{

/*!
 * Special ErrorInfo subclass representing a list of ErrorInfos.
 * Instances of this class are constructed by joinError.
 */
class MB_EXPORT ErrorList final : public ErrorInfo<ErrorList>
{
    // handle_errors needs to be able to iterate the payload list of an
    // ErrorList.
    template <typename... HandlerTs>
    friend Error handle_errors(Error e, HandlerTs &&... handlers);

    // join_errors is implemented in terms of join.
    friend Error join_errors(Error, Error);

public:
    static char ID;

    std::string message() const override;

private:
    ErrorList(std::unique_ptr<ErrorInfoBase> payload1,
              std::unique_ptr<ErrorInfoBase> payload2);

    static Error join(Error e1, Error e2);

    std::vector<std::unique_ptr<ErrorInfoBase>> _payloads;
};

/*!
 * Concatenate errors. The resulting Error is unchecked, and contains the
 * ErrorInfo(s), if any, contained in e1, followed by the ErrorInfo(s), if any,
 * contained in e2.
 */
inline MB_EXPORT Error join_errors(Error e1, Error e2)
{
    return ErrorList::join(std::move(e1), std::move(e2));
}

}
