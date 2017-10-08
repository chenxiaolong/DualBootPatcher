/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include "custom_error.h"

#include "mbcommon/string.h"

char CustomError::ID = 0;

CustomError::CustomError(int info)
    : _info(info)
{
}

int CustomError::get_info() const
{
    return _info;
}

std::string CustomError::message() const
{
    return mb::format("CustomError { %d }", get_info());
}

char CustomSubError::ID = 0;

CustomSubError::CustomSubError(int info, int extra_info)
    : _extra_info(extra_info)
{
    _info = info;
}

int CustomSubError::get_extra_info() const
{
    return _extra_info;
}

std::string CustomSubError::message() const
{
    return mb::format("CustomSubError { %d, %d }",
                      get_info(), get_extra_info());
}
