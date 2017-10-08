/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include "mbcommon/error/error.h"

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

}
