/*
 * Copyright (C) 2018  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mbsystrace/registers_p.h"

#include <linux/elf.h>
#include <sys/ptrace.h>

#include "mbcommon/error_code.h"

namespace mb::systrace::detail
{

oc::result<void> read_raw_regs(pid_t tid, iovec &iov)
{
    if (ptrace(PTRACE_GETREGSET, tid, NT_PRSTATUS, &iov) != 0) {
        return ec_from_errno();
    }

    return oc::success();
}

oc::result<void> write_raw_regs(pid_t tid, iovec &iov)
{
    if (ptrace(PTRACE_SETREGSET, tid, NT_PRSTATUS, &iov) != 0) {
        return ec_from_errno();
    }

    return oc::success();
}

}
