/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#pragma once

#include "mbcommon/error/error_info.h"

namespace mb
{

/*!
 * This class wraps a string in an Error.
 *
 * StringError is useful in cases where the client is not expected to be able
 * to consume the specific error message programmatically (for example, if the
 * error message is to be presented to the user).
 */
class MB_EXPORT StringError : public ErrorInfo<StringError>
{
public:
    static char ID;

    StringError(std::string msg);

    std::string message() const override;

private:
    std::string _msg;
};

}
