/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/error_code.h"

#ifdef __MINGW32__
#  include <windows.h>

#  include "mbcommon/finally.h"
#  include "mbcommon/locale.h"
#endif

namespace mb
{

std::error_code ec_from_errno(int errno_value)
{
    return {errno_value, std::generic_category()};
}

std::error_code ec_from_errno()
{
    return ec_from_errno(errno);
}

// Unfortunately, libstdc++'s system_category() only works with errno
// https://github.com/gcc-mirror/gcc/blob/gcc-7_2_0-release/libstdc%2B%2B-v3/src/c%2B%2B11/system_error.cc#L54

#ifdef __MINGW32__

struct Win32ErrorCategory : std::error_category
{
    const char * name() const noexcept override;

    std::string message(int ev) const override;

    std::error_condition
    default_error_condition(int code) const noexcept override;
};

const char * Win32ErrorCategory::name() const noexcept
{
    return "win32";
}

std::string Win32ErrorCategory::message(int ev) const
{
    LPWSTR message_buffer = nullptr;
    size_t size = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_FROM_SYSTEM
            | FORMAT_MESSAGE_IGNORE_INSERTS,            // dwFlags
        nullptr,                                        // lpSource
        static_cast<DWORD>(ev),                         // dwMessageId
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),      // dwLanguageId
        reinterpret_cast<LPWSTR>(&message_buffer),      // lpBuffer
        0,                                              // nSize
        nullptr                                         // Arguments
    );

    auto free_message_buffer = finally([&] {
        LocalFree(message_buffer);
    });

    std::string result;

    if (size > 0) {
        std::wstring message(message_buffer, size);

        auto converted = wcs_to_utf8(message);
        if (converted) {
            result = std::move(converted.value());

            while (!result.empty()
                    && (result.back() == '\n' || result.back() == '\r')) {
                result.pop_back();
            }
            if (!result.empty() && result.back() == '.') {
                result.pop_back();
            }
        } else {
            result = "(Failed to convert message to UTF-8)";
        }
    } else {
        result = "(Unknown error)";
    }

    return result;
}

std::error_condition
Win32ErrorCategory::default_error_condition(int code) const noexcept
{
#define MAP_ERROR(WIN32, ERRC) \
    case WIN32: return std::make_error_condition(std::errc::ERRC)

    // Mapping taken from Boost 1.65.1:
    // http://www.boost.org/doc/libs/1_65_1/boost/system/detail/error_code.ipp
    switch (code) {
    MAP_ERROR(ERROR_ACCESS_DENIED, permission_denied);
    MAP_ERROR(ERROR_ALREADY_EXISTS, file_exists);
    MAP_ERROR(ERROR_BAD_UNIT, no_such_device);
    MAP_ERROR(ERROR_BUFFER_OVERFLOW, filename_too_long);
    MAP_ERROR(ERROR_BUSY, device_or_resource_busy);
    MAP_ERROR(ERROR_BUSY_DRIVE, device_or_resource_busy);
    MAP_ERROR(ERROR_CANNOT_MAKE, permission_denied);
    MAP_ERROR(ERROR_CANTOPEN, io_error);
    MAP_ERROR(ERROR_CANTREAD, io_error);
    MAP_ERROR(ERROR_CANTWRITE, io_error);
    MAP_ERROR(ERROR_CURRENT_DIRECTORY, permission_denied);
    MAP_ERROR(ERROR_DEV_NOT_EXIST, no_such_device);
    MAP_ERROR(ERROR_DEVICE_IN_USE, device_or_resource_busy);
    MAP_ERROR(ERROR_DIR_NOT_EMPTY, directory_not_empty);
    MAP_ERROR(ERROR_DIRECTORY, invalid_argument);
    // WinError.h: "The directory name is invalid"
    MAP_ERROR(ERROR_DISK_FULL, no_space_on_device);
    MAP_ERROR(ERROR_FILE_EXISTS, file_exists);
    MAP_ERROR(ERROR_FILE_NOT_FOUND, no_such_file_or_directory);
    MAP_ERROR(ERROR_HANDLE_DISK_FULL, no_space_on_device);
    MAP_ERROR(ERROR_INVALID_ACCESS, permission_denied);
    MAP_ERROR(ERROR_INVALID_DRIVE, no_such_device);
    MAP_ERROR(ERROR_INVALID_FUNCTION, function_not_supported);
    MAP_ERROR(ERROR_INVALID_HANDLE, invalid_argument);
    MAP_ERROR(ERROR_INVALID_NAME, invalid_argument);
    MAP_ERROR(ERROR_LOCK_VIOLATION, no_lock_available);
    MAP_ERROR(ERROR_LOCKED, no_lock_available);
    MAP_ERROR(ERROR_NEGATIVE_SEEK, invalid_argument);
    MAP_ERROR(ERROR_NOACCESS, permission_denied);
    MAP_ERROR(ERROR_NOT_ENOUGH_MEMORY, not_enough_memory);
    MAP_ERROR(ERROR_NOT_READY, resource_unavailable_try_again);
    MAP_ERROR(ERROR_NOT_SAME_DEVICE, cross_device_link);
    MAP_ERROR(ERROR_OPEN_FAILED, io_error);
    MAP_ERROR(ERROR_OPEN_FILES, device_or_resource_busy);
    MAP_ERROR(ERROR_OPERATION_ABORTED, operation_canceled);
    MAP_ERROR(ERROR_OUTOFMEMORY, not_enough_memory);
    MAP_ERROR(ERROR_PATH_NOT_FOUND, no_such_file_or_directory);
    MAP_ERROR(ERROR_READ_FAULT, io_error);
    MAP_ERROR(ERROR_RETRY, resource_unavailable_try_again);
    MAP_ERROR(ERROR_SEEK, io_error);
    MAP_ERROR(ERROR_SHARING_VIOLATION, permission_denied);
    MAP_ERROR(ERROR_TOO_MANY_OPEN_FILES, too_many_files_open);
    MAP_ERROR(ERROR_WRITE_FAULT, io_error);
    MAP_ERROR(ERROR_WRITE_PROTECT, permission_denied);
    MAP_ERROR(WSAEACCES, permission_denied);
    MAP_ERROR(WSAEADDRINUSE, address_in_use);
    MAP_ERROR(WSAEADDRNOTAVAIL, address_not_available);
    MAP_ERROR(WSAEAFNOSUPPORT, address_family_not_supported);
    MAP_ERROR(WSAEALREADY, connection_already_in_progress);
    MAP_ERROR(WSAEBADF, bad_file_descriptor);
    MAP_ERROR(WSAECONNABORTED, connection_aborted);
    MAP_ERROR(WSAECONNREFUSED, connection_refused);
    MAP_ERROR(WSAECONNRESET, connection_reset);
    MAP_ERROR(WSAEDESTADDRREQ, destination_address_required);
    MAP_ERROR(WSAEFAULT, bad_address);
    MAP_ERROR(WSAEHOSTUNREACH, host_unreachable);
    MAP_ERROR(WSAEINPROGRESS, operation_in_progress);
    MAP_ERROR(WSAEINTR, interrupted);
    MAP_ERROR(WSAEINVAL, invalid_argument);
    MAP_ERROR(WSAEISCONN, already_connected);
    MAP_ERROR(WSAEMFILE, too_many_files_open);
    MAP_ERROR(WSAEMSGSIZE, message_size);
    MAP_ERROR(WSAENAMETOOLONG, filename_too_long);
    MAP_ERROR(WSAENETDOWN, network_down);
    MAP_ERROR(WSAENETRESET, network_reset);
    MAP_ERROR(WSAENETUNREACH, network_unreachable);
    MAP_ERROR(WSAENOBUFS, no_buffer_space);
    MAP_ERROR(WSAENOPROTOOPT, no_protocol_option);
    MAP_ERROR(WSAENOTCONN, not_connected);
    MAP_ERROR(WSAENOTSOCK, not_a_socket);
    MAP_ERROR(WSAEOPNOTSUPP, operation_not_supported);
    MAP_ERROR(WSAEPROTONOSUPPORT, protocol_not_supported);
    MAP_ERROR(WSAEPROTOTYPE, wrong_protocol_type);
    MAP_ERROR(WSAETIMEDOUT, timed_out);
    MAP_ERROR(WSAEWOULDBLOCK, operation_would_block);
    default:
        return {code, std::system_category()};
    }

#undef MAP_ERROR
}

#endif

#ifdef _WIN32
const std::error_category & win32_error_category()
{
#ifdef __MINGW32__
    static Win32ErrorCategory c;
    return c;
#else
    return std::system_category();
#endif
}

std::error_code ec_from_win32(DWORD error_value)
{
    return {static_cast<int>(error_value), win32_error_category()};
}

std::error_code ec_from_win32()
{
    return ec_from_win32(GetLastError());
}
#endif

}
