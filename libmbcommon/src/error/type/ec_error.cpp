/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include "mbcommon/error/type/ec_error.h"

namespace mb
{

char ECError::ID = 0;

ECError::ECError(std::error_code ec) : _ec(ec)
{
}

std::error_code ECError::error_code() const
{
    return _ec;
}

std::string ECError::message() const
{
    return _ec.message();
}

}
