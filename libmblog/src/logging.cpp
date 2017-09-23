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
#include <mutex>
#include <string>

#include <cerrno>
#include <cinttypes>
#include <cstdarg>
#include <cstdlib>
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
static std::mutex g_mutex;

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

static bool _local_time(time_t t, std::tm &tm)
{
    // Some systems call tzset() in localtime_r() and others don't. We'll
    // manually call it to ensure the behavior is the same on every platform.
    tzset();

#ifdef _WIN32
    return localtime_s(&tm, &t) == 0;
#else
    return localtime_r(&t, &tm) != nullptr;
#endif
}

static void _local_time_ns(const std::chrono::system_clock::time_point &tp,
                           std::tm &tm, long &nanos, long &gmtoff)
{
    using namespace std::chrono;

    auto tp_seconds = time_point_cast<seconds>(tp);
    auto t = system_clock::to_time_t(tp);

    // * Windows
    // - There will be a race condition if the timezone is changed and
    //   tzset() is called between _local_time() and accessing _timezone or
    //   _dstbias.
    //
    // * Linux with glibc, musl, or bionic
    // * macOS
    // * FreeBSD and OpenBSD
    // - localtime_r locks the tzset mutex so there's no race condition in
    //   setting the tm_gmtoff field.

    if (!_local_time(t, tm)) {
        tm = {};
        tm.tm_mday = 1;
        tm.tm_year = 70;
        tm.tm_wday = 4;
        tm.tm_isdst = 0;
        nanos = 0;
        gmtoff = 0;
    } else {
        nanos = static_cast<long>(
                duration_cast<nanoseconds>(tp - tp_seconds).count());

#ifdef _WIN32
        long dstbias;

        // The default msvcrt that mingw uses doesn't have the _get_timezone()
        // and _get_dstbias() functions
#ifndef __GNUC__
        if (_get_timezone(&gmtoff) || _get_dstbias(&dstbias)) {
            return {};
        }
#else
        gmtoff = _timezone;
        dstbias = _dstbias;
#endif

        if (tm.tm_isdst) {
            gmtoff += dstbias;
        }
        gmtoff = -gmtoff;
#else
        gmtoff = tm.tm_gmtoff;
#endif
    }
}

static std::string _format_iso8601(const std::tm &tm, long nanoseconds,
                                   long gmtoff)
{
    // Sample: 2017-09-17T23:27:00.000000000+00:00
    return mb::format("%04d-%02d-%02dT%02d:%02d:%02d.%09ld%c%02ld:%02ld",
                      tm.tm_year + 1900,
                      tm.tm_mon + 1,
                      tm.tm_mday,
                      tm.tm_hour,
                      tm.tm_min,
                      tm.tm_sec,
                      nanoseconds,
                      gmtoff >= 0 ? '+' : '-',
                      std::abs(gmtoff) / 3600,
                      std::abs(gmtoff / 60) % 60);
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
                    std::tm tm;
                    long nanos;
                    long gmtoff;

                    _local_time_ns(rec.time, tm, nanos, gmtoff);
                    time_buf = _format_iso8601(tm, nanos, gmtoff);
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
    std::lock_guard<std::mutex> guard(g_mutex);

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
