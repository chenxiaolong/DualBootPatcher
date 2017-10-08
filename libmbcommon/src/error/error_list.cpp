/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include "mbcommon/error/error_list.h"

#include <cassert>

namespace mb
{

char ErrorList::ID = 0;

std::string ErrorList::message() const
{
    std::string buf("Multiple errors:\n");
    for (auto &payload : _payloads) {
        buf += payload->message();
        buf += '\n';
    }
    return buf;
}

ErrorList::ErrorList(std::unique_ptr<ErrorInfoBase> payload1,
                     std::unique_ptr<ErrorInfoBase> payload2)
{
    assert(!payload1->is_a<ErrorList>() && !payload2->is_a<ErrorList>() &&
           "ErrorList constructor payloads should be singleton errors");
    _payloads.push_back(std::move(payload1));
    _payloads.push_back(std::move(payload2));
}

Error ErrorList::join(Error e1, Error e2)
{
    if (!e1) {
        return e2;
    }

    if (!e2) {
        return e1;
    }

    if (e1.is_a<ErrorList>()) {
        auto &e1_list = static_cast<ErrorList &>(*e1._payload);

        if (e2.is_a<ErrorList>()) {
            auto e2_payload = e2.take_payload();
            auto &e2_list = static_cast<ErrorList &>(*e2_payload);
            for (auto &payload : e2_list._payloads) {
                e1_list._payloads.push_back(std::move(payload));
            }
        } else {
            e1_list._payloads.push_back(e2.take_payload());
        }

        return e1;
    }

    if (e2.is_a<ErrorList>()) {
        auto &e2_list = static_cast<ErrorList &>(*e2._payload);
        e2_list._payloads.insert(e2_list._payloads.begin(), e1.take_payload());
        return e2;
    }

    return {std::unique_ptr<ErrorList>(new ErrorList(
            e1.take_payload(), e2.take_payload()))};
}

}
