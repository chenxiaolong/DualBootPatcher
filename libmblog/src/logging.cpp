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
#include <optional>
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
#include "mbcommon/string.h"
#include "mbcommon/type_traits.h"

#include "mblog/log_record.h"
#include "mblog/stdio_logger.h"

namespace mb::log
{

#if defined(_WIN32)
using Pid = ReturnType<decltype(GetCurrentProcessId)>::type;
using Tid = ReturnType<decltype(GetCurrentThreadId)>::type;
#elif defined(__APPLE__)
using Pid = ReturnType<decltype(getpid)>::type;
using Tid = std::remove_pointer_t<ArgN<1, decltype(pthread_threadid_np)>::type>;
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

static std::tm _tm_epoch()
{
    std::tm tm = {};
    tm.tm_mday = 1;
    tm.tm_year = 70;
    tm.tm_wday = 4;
    tm.tm_isdst = 0;
    return tm;
}

// Based on wine's msvcrt code and mingw's winpthread code
#ifdef _WIN32
// Number of 100ns-seconds between the beginning of the Windows epoch
// (Jan. 1, 1601) and the Unix epoch (Jan. 1, 1970)
static constexpr int64_t DELTA_EPOCH_IN_100NS = INT64_C(116444736000000000);
static constexpr uint32_t POW10_7 = 10000000;

union FileTimeU64
{
    uint64_t u64;
    FILETIME ft;
};

static constexpr int g_month_lengths[2][12] = {
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
};

static inline bool _is_leap_year(int year)
{
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

static bool _is_dst(const TIME_ZONE_INFORMATION &tzi, const SYSTEMTIME &st)
{
    // No DST if timezone doesn't have daylight savings rules
    if (!tzi.DaylightDate.wMonth) {
        return false;
    }

    // Remove UTC bias and standard bias, leaving only the daylight savings bias
    // so that when converting to local time, the SYSTEMTIME structure only
    // changes if daylight savings is in effect.
    // Add bias only for daylight time
    TIME_ZONE_INFORMATION tmp = tzi;
    tmp.Bias = 0;
    tmp.StandardBias = 0;

    SYSTEMTIME out;

    if (!SystemTimeToTzSpecificLocalTime(&tmp, &st, &out)) {
        return false;
    }

    return memcmp(&st, &out, sizeof(SYSTEMTIME));
  }

static void _time_point_to_file_time(const std::chrono::system_clock::time_point &tp,
                                     FILETIME &ft)
{
    using namespace std::chrono;

    FileTimeU64 ftu64;

    auto t = system_clock::to_time_t(tp);
    auto nanos = duration_cast<nanoseconds>(
            tp - time_point_cast<seconds>(tp)).count();

    ftu64.u64 = static_cast<uint64_t>(t) * POW10_7 + DELTA_EPOCH_IN_100NS
            + static_cast<uint64_t>(nanos) / 100;

    ft = ftu64.ft;
}

static bool _file_time_to_ns(const FILETIME &ft,
                             const TIME_ZONE_INFORMATION &tzi,
                             std::tm &tm, long &nanos)
{
    SYSTEMTIME st;

    if (!FileTimeToSystemTime(&ft, &st)) {
        return false;
    }

    tm.tm_sec = st.wSecond;
    tm.tm_min = st.wMinute;
    tm.tm_hour = st.wHour;
    tm.tm_mday = st.wDay;
    tm.tm_year = st.wYear - 1900;
    tm.tm_mon = st.wMonth - 1;
    tm.tm_wday = st.wDayOfWeek;
    for (auto i = tm.tm_yday = 0; i < st.wMonth - 1; i++) {
        tm.tm_yday += g_month_lengths[_is_leap_year(st.wYear)][i];
    }
    tm.tm_yday += st.wDay - 1;
    tm.tm_isdst = _is_dst(tzi, st);

    FileTimeU64 ftu64;
    ftu64.ft = ft;

    nanos = static_cast<long>(
            (ftu64.u64 - DELTA_EPOCH_IN_100NS) % POW10_7 * 100);

    return true;
}
#endif

static bool _local_time_ns(const std::chrono::system_clock::time_point &tp,
                           std::tm &tm, long &nanos, long &gmtoff)
{
    using namespace std::chrono;

    // * Windows
    // - If localtime_s() is used, there will be a race condition if the
    //   timezone is changed and tzset() is called between localtime_s() and
    //   accessing _timezone or _dstbias. Instead, we'll get the timezone
    //   beforehand, and convert to local time manually.
    //
    // * Linux with glibc, musl, or bionic
    // * macOS
    // * FreeBSD and OpenBSD
    // - localtime_r locks the tzset mutex so there's no race condition in
    //   setting the tm_gmtoff field.

#ifdef _WIN32
    TIME_ZONE_INFORMATION tzi;
    FILETIME ft_utc;
    FILETIME ft_local;

    if (GetTimeZoneInformation(&tzi) == TIME_ZONE_ID_INVALID) {
        return false;
    }

    _time_point_to_file_time(tp, ft_utc);

    if (!FileTimeToLocalFileTime(&ft_utc, &ft_local)) {
        return false;
    }

    if (!_file_time_to_ns(ft_local, tzi, tm, nanos)) {
        return false;
    }

    gmtoff = -tzi.Bias * 60;

    if (tzi.DaylightDate.wMonth && tm.tm_isdst) {
        gmtoff -= tzi.DaylightBias * 60;
    } else if (tzi.StandardDate.wMonth) {
        gmtoff -= tzi.StandardBias * 60;
    }
#else
    auto t = system_clock::to_time_t(tp);

    // Some systems call tzset() in localtime_r() and others don't. We'll
    // manually call it to ensure the behavior is the same on every platform.
    tzset();

    if (!localtime_r(&t, &tm)) {
        return false;
    }

    nanos = static_cast<long>(duration_cast<nanoseconds>(
            tp - time_point_cast<seconds>(tp)).count());
    gmtoff = tm.tm_gmtoff;
#endif

    return true;
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

static char _format_prio(LogLevel prio)
{
    switch (prio) {
    case LogLevel::Error:
        return 'E';
    case LogLevel::Warning:
        return 'W';
    case LogLevel::Info:
        return 'I';
    case LogLevel::Debug:
        return 'D';
    case LogLevel::Verbose:
        return 'V';
    default:
        MB_UNREACHABLE("Invalid log level: %d", static_cast<int>(prio));
    }
}

static std::string _format_rec(const LogRecord &rec)
{
    std::string buf;
    std::optional<std::string> time_buf;

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

                    if (!_local_time_ns(rec.time, tm, nanos, gmtoff)) {
                        tm = _tm_epoch();
                        nanos = 0;
                        gmtoff = 0;
                    }

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
