/*
 * This is based on LLVM code.
 * See licenses/llvm/LICENSE.TXT for details.
 */

#include "mbcommon/error/error_info_base.h"

namespace mb
{

char ErrorInfoBase::ID = 0;

const void * ErrorInfoBase::class_id()
{
    return &ID;
}

bool ErrorInfoBase::is_a(const void *const id) const
{
    return id == class_id();
}

}
