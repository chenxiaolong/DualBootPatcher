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

#include "mbsystrace/tracee.h"

#include <string_view>
#include <vector>

#include <cerrno>
#include <climits>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "mbcommon/error_code.h"

#include "mbsystrace/syscalls.h"

namespace mb::systrace
{

// Maximum errno is 4095 on all architectures
// https://elixir.bootlin.com/linux/v4.15.4/source/include/linux/err.h#L18
static constexpr int MAX_ERRNO = 4095;

/*!
 * \brief Get process's page size
 *
 * \return Page size if it is successfully retrieved. Otherwise, an appropriate
 *         error code.
 */
static oc::result<uintptr_t> get_page_size()
{
    const long page_size = sysconf(_SC_PAGESIZE);
    if (page_size < 0) {
        return ec_from_errno();
    }

    return static_cast<uintptr_t>(page_size);
}

/*!
 * \brief Split address range into `iovec`s that don't cross page boundaries
 *
 * \pre \p addr + \p size must not overflow
 *
 * \param addr Starting address
 * \param size Size of memory block
 * \param page_size Page size
 *
 * \return Vector of `iovec` split at page boundaries
 */
static std::vector<iovec>
split_iovs(uintptr_t addr, size_t size, uintptr_t page_size)
{
    std::vector<iovec> iovs;

    uintptr_t cur_addr = addr;
    uintptr_t max_addr = addr + size;

    while (cur_addr < max_addr) {
        auto &iov = iovs.emplace_back();
        iov.iov_base = reinterpret_cast<void *>(cur_addr);
        iov.iov_len = std::min(page_size - cur_addr % page_size,
                               max_addr - cur_addr);

        cur_addr += iov.iov_len;
    }

    return iovs;
}

/*!
 * \brief Read memory from the process
 *
 * \param addr Address to read from
 * \param buf Buffer to read data into
 * \param size Number of bytes to read
 *
 * \pre state() must be a ptrace stop state
 *
 * \return Number of bytes read if successful. This will be fewer than \p size
 *         if the address range extends into an unmapped page. However, if
 *         \p addr is an unmapped address, then `std::errc::bad_address` is
 *         returned instead of 0. On failure, returns an appropriate error code.
 */
oc::result<size_t>
Tracee::read_mem(uintptr_t addr, void *buf, size_t size)
{
    static_assert(sizeof(uintptr_t) >= sizeof(size_t));

    if (addr > std::numeric_limits<uintptr_t>::max() - size) {
        return std::errc::value_too_large;
    }

    OUTCOME_TRY(page_size, get_page_size());

    std::vector<iovec> remote_iovs = split_iovs(addr, size, page_size);
    iovec local_iov{buf, size};

    auto n = process_vm_readv(tid, &local_iov, 1, remote_iovs.data(),
                              remote_iovs.size(), 0);
    if (n < 0) {
        return ec_from_errno();
    }

    return static_cast<size_t>(n);
}

/*!
 * \brief Write memory to the process
 *
 * \param addr Address to write to
 * \param buf Buffer to write data from
 * \param size Number of bytes to write
 *
 * \pre state() must be a ptrace stop state
 *
 * \return Number of bytes written if successful. This will be fewer than
 *         \p size if the address range extends into an unmapped page. However,
 *         if \p addr is an unmapped address, then `std::errc::bad_address` is
 *         returned instead of 0. On failure, returns an appropriate error code.
 */
oc::result<size_t>
Tracee::write_mem(uintptr_t addr, const void *buf, size_t size)
{
    static_assert(sizeof(uintptr_t) >= sizeof(size_t));

    if (addr > std::numeric_limits<uintptr_t>::max() - size) {
        return std::errc::value_too_large;
    }

    OUTCOME_TRY(page_size, get_page_size());

    std::vector<iovec> remote_iovs = split_iovs(addr, size, page_size);
    iovec local_iov{const_cast<void *>(buf), size};

    auto n = process_vm_writev(tid, &local_iov, 1, remote_iovs.data(),
                               remote_iovs.size(), 0);
    if (n < 0) {
        return ec_from_errno();
    }

    return static_cast<size_t>(n);
}

/*!
 * \brief Read a NULL-terminated string from the process
 *
 * \param addr Address to read string from
 *
 * \pre state() must be a ptrace stop state
 *
 * \return The resulting string if successful. Otherwise, returns an error code.
 */
oc::result<std::string> Tracee::read_string(uintptr_t addr)
{
    std::string result;
    result.reserve(64);

    char buf[4096];

    while (true) {
        OUTCOME_TRY(n, read_mem(addr, buf, sizeof(buf)));

        size_t str_n = strnlen(buf, n);
        result += std::string_view(buf, str_n);

        if (str_n < n) {
            break;
        }

        addr += n;
    }

    return result;
}

/*!
 * \brief Execute mmap in the process
 *
 * This function will always invoke the `mmap2` syscall if supported. Otherwise,
 * it will invoke the `mmap` syscall.
 *
 * \pre state() must be a ptrace stop state
 *
 * \param addr Same as with `mmap(2)`
 * \param length Same as with `mmap(2)`
 * \param prot Same as with `mmap(2)`
 * \param flags Same as with `mmap(2)`
 * \param fd Same as with `mmap(2)`
 * \param offset Same as with `mmap(2)`
 *
 * \return If the `mmap2` and `mmap` syscalls are not found, returns
 *         `std::errc::function_not_supported`. If `mmap2` is used
 *         (32 bit system) and \p offset is greater than 2^44, then
 *         `std::errc::invalid_argument` is returned. Otherwise, returns the
 *         result of `mmap(2)` or its error code.
 */
oc::result<uintptr_t>
Tracee::mmap(uintptr_t addr, size_t length, int prot, int flags, int fd,
             off64_t offset)
{
    // 2^12 == 4096
    constexpr int mmap2_shift = 12;

    // Maximum offset allowed by mmap2 is the page size * the maximum value of
    // a 32 bit integer (2^44 on most systems)
    constexpr off64_t mmap2_mask =
            static_cast<off64_t>(-1ull << (mmap2_shift + 32));

    SysCallArgs args = {};
    args[0] = addr;
    args[1] = length;
    args[2] = static_cast<unsigned long>(prot);
    args[3] = static_cast<unsigned long>(flags);
    args[4] = static_cast<unsigned long>(fd);
    args[5] = static_cast<unsigned long>(offset);

    auto mmap_sc = SysCall("mmap2", m_regs.abi());
    if (mmap_sc) {
        if (offset & mmap2_mask) {
            return std::errc::invalid_argument;
        }

        // glibc (but not bionic) silently drops last 12 bits
        args[5] = static_cast<unsigned long>(offset >> mmap2_shift);
    } else {
        mmap_sc = SysCall("mmap", m_regs.abi());
    }

    if (!mmap_sc) {
        return std::errc::function_not_supported;
    }

    OUTCOME_TRY(ret, inject_syscall(mmap_sc.num(), args));

    if (ret < 0 && ret >= -MAX_ERRNO) {
        return ec_from_errno(static_cast<int>(-ret));
    }

    return static_cast<uintptr_t>(ret);
}

/*!
 * \brief Execute munmap in the process
 *
 * \pre state() must be a ptrace stop state
 *
 * \param addr Same as with `munmap(2)`
 * \param length Same as with `munmap(2)`
 *
 * \return If the `munmap` syscall is not found, returns
 *         `std::errc::function_not_supported`. Otherwise, returns nothing if
 *         `munmap(2)` succeeds or the error code if it fails.
 */
oc::result<void> Tracee::munmap(uintptr_t addr, size_t length)
{
    SysCallArgs args = {};
    args[0] = addr;
    args[1] = length;

    auto munmap_sc = SysCall("munmap", m_regs.abi());
    if (!munmap_sc) {
        return std::errc::function_not_supported;
    }

    OUTCOME_TRY(ret, inject_syscall(munmap_sc.num(), args));

    if (ret < 0 && ret >= -MAX_ERRNO) {
        return ec_from_errno(static_cast<int>(-ret));
    }

    return oc::success();
}

}
