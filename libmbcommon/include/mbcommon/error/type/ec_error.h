/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#pragma once

#include <system_error>

#include "mbcommon/error/error_info.h"

namespace mb
{

/*!
 * This class wraps a std::error_code in a Error.
 *
 * This is useful if you're writing an interface that returns a Error
 * (or Expected) and you want to call code that still returns
 * std::error_codes.
 */
class MB_EXPORT ECError : public ErrorInfo<ECError>
{
public:
    static char ID;

    ECError(std::error_code ec);

    std::error_code error_code() const;

    std::string message() const override;

protected:
    std::error_code _ec;
};

}
