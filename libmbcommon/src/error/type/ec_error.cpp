/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include "mbcommon/error/type/ec_error.h"

#ifdef _WIN32
#  include "mbcommon/error/win32_error.h"
#endif

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

ECError ECError::from_errno(int error)
{
    return {std::error_code{error, std::generic_category()}};
}

#ifdef _WIN32
ECError ECError::from_win32_error(DWORD error)
{
    return {std::error_code{static_cast<int>(error), win32_error_category()}};
}
#endif

}
