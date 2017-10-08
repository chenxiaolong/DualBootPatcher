/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include "mbcommon/error/type/string_error.h"

namespace mb
{

char StringError::ID = 0;

StringError::StringError(std::string msg)
    : _msg(std::move(msg))
{
}

std::string StringError::message() const
{
    return _msg;
}

}
