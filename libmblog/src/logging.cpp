/*
 * Copyright (C) 2014-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mblog/logging.h"

#include <chrono>
#include <string>

#include <cerrno>
#include <cinttypes>
#include <cstdarg>
#include <ctime>

#if defined(_WIN32)
#  include <windows.h>
#else
#  if defined(__APPLE__)
#    include <pthread.h>
#  elif defined(__linux__)
#    include <sys/syscall.h>
#  endif
#  include <unistd.h>
#endif

#include "mbcommon/error.h"
#include "mbcommon/optional.h"
#include "mbcommon/string.h"
#include "mbcommon/type_traits.h"

#include "mblog/log_record.h"
#include "mblog/stdio_logger.h"

namespace mb
{
namespace log
{

#if defined(_WIN32)
using Pid = ReturnType<decltype(GetCurrentProcessId)>::type;
using Tid = ReturnType<decltype(GetCurrentThreadId)>::type;
#elif defined(__APPLE__)
using Pid = ReturnType<decltype(getpid)>::type;
using Tid = std::remove_pointer<ArgN<1, decltype(pthread_threadid_np)>::type>::type;
#elif defined(__linux__)
using Pid = ReturnType<decltype(getpid)>::type;
using Tid = pid_t;
#endif


static std::shared_ptr<BaseLogger> g_logger;

static std::string g_time_format{"%Y/%m/%d %H:%M:%S %Z"};
static std::string g_format{"[%t][%P:%T][%l] %n: %m"};

// %l - Level
// %m - Message
// %n - Tag
// %t - Time
// %P - Process ID
// %T - Thread ID


static Pid _get_pid()
{
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

static Tid _get_tid()
{
#if defined(_WIN32)
    return GetCurrentThreadId();
#elif defined(__APPLE__)
    uint64_t tid;
    if (pthread_threadid_np(nullptr, &tid)) {
        return tid;
    } else {
        return 0;
    }
#elif defined(__BIONIC__)
    return gettid();
#elif defined(__linux__)
    return static_cast<Tid>(syscall(__NR_gettid));
#endif
}

static std::string _format_time(std::string fmt, const std::tm &tm)
{
    // Add a character to allow differentiating empty strings and errors
    fmt += ' ';

    std::string buf;
    buf.resize(fmt.size());

    std::size_t len;
    while ((len = std::strftime(&buf[0], buf.size(), fmt.c_str(), &tm)) == 0) {
        buf.resize(buf.size() * 2);
    }

    buf.resize(len - 1);
    return buf;
}

static std::string _format_time(std::string fmt,
                                const std::chrono::system_clock::time_point &time)
{
    auto t = std::chrono::system_clock::to_time_t(time);
    std::tm tm;

#if defined(_WIN32)
    if (localtime_s(&tm, &t)) {
        return {};
    }
#else
    if (!localtime_r(&t, &tm)) {
        return {};
    }
#endif

    return _format_time(std::move(fmt), tm);
}

static std::string _format_prio(LogLevel prio)
{
    switch (prio) {
    case LogLevel::Error:
        return "error";
    case LogLevel::Warning:
        return "warning";
    case LogLevel::Info:
        return "info";
    case LogLevel::Debug:
        return "debug";
    case LogLevel::Verbose:
        return "verbose";
    default:
        return {};
    }
}

static std::string _format_rec(const LogRecord &rec)
{
    std::string buf;
    optional<std::string> time_buf;

    for (auto it = g_format.begin(); it != g_format.end(); ++it) {
        if (*it == '%') {
            if (it + 1 == g_format.end()) {
                buf += '%';
                break;
            }

            ++it;

            switch (*it) {
            case 'l':
                buf += _format_prio(rec.prio);
                break;

            case 'm':
                buf += rec.msg;
                break;

            case 'n':
                buf += rec.tag;
                break;

            case 't':
                if (!time_buf) {
                    time_buf = _format_time(g_time_format, rec.time);
                }
                buf += *time_buf;
                break;

            case 'P':
                buf += mb::format("%" PRIu64, rec.pid);
                break;

            case 'T':
                buf += mb::format("%" PRIu64, rec.tid);
                break;

            default:
                buf += *it;
                break;
            }
        } else {
            buf += *it;
        }
    }

    return buf;
}

std::shared_ptr<BaseLogger> logger()
{
    return g_logger;
}

void set_logger(std::shared_ptr<BaseLogger> logger)
{
    g_logger = std::move(logger);
}

void log(LogLevel prio, const char *tag, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    log_v(prio, tag, fmt, ap);

    va_end(ap);
}

void log_v(LogLevel prio, const char *tag, const char *fmt, va_list ap)
{
    ErrorRestorer restorer;
    LogRecord rec;

    rec.time = std::chrono::system_clock::now();
    rec.pid = static_cast<uint64_t>(_get_pid());
    rec.tid = static_cast<uint64_t>(_get_tid());
    rec.prio = prio;
    rec.tag = tag;
    rec.msg = format_v(fmt, ap);

    if (!g_logger) {
        g_logger = std::make_shared<StdioLogger>(stdout);
    }

    if (g_logger->formatted()) {
        rec.fmt_msg = _format_rec(rec);
    }

    g_logger->log(rec);
}

std::string time_format()
{
    return g_time_format;
}

void set_time_format(std::string fmt)
{
    g_time_format = std::move(fmt);
}

std::string format()
{
    return g_format;
}

void set_format(std::string fmt)
{
    g_format = std::move(fmt);
}

}
}
