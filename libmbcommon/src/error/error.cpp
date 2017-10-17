/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include "mbcommon/error/error.h"

#include "mbcommon/error/error_handler.h"

namespace mb
{

Error::Error() = default;

Error::Error(Error &&other)
{
    *this = std::move(other);
}

Error::Error(std::unique_ptr<ErrorInfoBase> payload)
{
    _payload = std::move(payload);
}

Error & Error::operator=(Error &&other)
{
    _payload = std::move(other._payload);
    return *this;
}

Error::~Error() = default;

Error::operator bool()
{
    return _payload.operator bool();
}

const void * Error::dynamic_class_id() const
{
    if (!_payload) {
        return nullptr;
    }
    return _payload->dynamic_class_id();
}

std::unique_ptr<ErrorInfoBase> Error::take_payload()
{
    return std::move(_payload);
}

std::string to_string(Error e)
{
    return to_string(std::move(e), "; ");
}

std::string to_string(Error e, const std::string &delimiter)
{
    std::string buf;
    bool first = true;

    (void) handle_errors(
        std::move(e),
        [&](const ErrorInfoBase &ei) {
            if (first) {
                first = false;
            } else {
                buf += delimiter;
            }
            buf += ei.message();
        }
    );

    return buf;
}

}
