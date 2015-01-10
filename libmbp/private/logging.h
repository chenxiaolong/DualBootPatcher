/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <cassert>

#include <string>
#include <vector>

#include <boost/format.hpp>


#if defined(ANDROID) && !defined(LIBMBP_MINI)
#  include <android/log.h>
#  define LOG_TAG "libmbp"
#else
#  include <cstdio>
#endif

#define LOGD(...) Log::log(Debug, __VA_ARGS__)
#define LOGV(...) Log::log(Verbose, __VA_ARGS__)
#define LOGI(...) Log::log(Info, __VA_ARGS__)
#define LOGW(...) Log::log(Warning, __VA_ARGS__)
#define LOGE(...) Log::log(Error, __VA_ARGS__)


/*!
    \class Log
    \brief A very basic logger
 */
class Log
{
public:
    enum LogLevel {
        Debug,
        Verbose,
        Info,
        Warning,
        Error
    };

    static void log(LogLevel level, const boost::format &fmt)
    {
#if defined(ANDROID) && !defined(LIBMBP_MINI)
        android_LogPriority priority = ANDROID_LOG_DEBUG;

        switch (level) {
        case Debug:
            priority = ANDROID_LOG_DEBUG;
            break;
        case Verbose:
            priority = ANDROID_LOG_VERBOSE;
            break;
        case Info:
            priority = ANDROID_LOG_INFO;
            break;
        case Warning:
            priority = ANDROID_LOG_WARN;
            break;
        case Error:
            priority = ANDROID_LOG_ERROR;
            break;
        default:
            assert(false);
        }

        __android_log_print(priority, LOG_TAG, "%s", fmt.str().c_str());
#else
        switch (level) {
        case Debug:
            std::fprintf(stderr, "[Debug]: %s\n", fmt.str().c_str());
            break;
        case Verbose:
            std::fprintf(stderr, "[Verbose]: %s\n", fmt.str().c_str());
            break;
        case Info:
            std::fprintf(stderr, "[Info]: %s\n", fmt.str().c_str());
            break;
        case Warning:
            std::fprintf(stderr, "[Warning]: %s\n", fmt.str().c_str());
            break;
        case Error:
            std::fprintf(stderr, "[Error]: %s\n", fmt.str().c_str());
            break;
        default:
            assert(false);
        }
#endif
    }

    static void log(LogLevel level, const std::string &str)
    {
        log(level, boost::format(str));
    }

    // Thanks to jxh at http://stackoverflow.com/questions/18347957/a-convenient-logging-statement-for-c-using-boostformat
    // for the variadic template boost::format wrappers!

    template <typename T, typename... Params>
    static void log(LogLevel level, boost::format &fmt, T arg, Params... params)
    {
        log(level, fmt % arg, params...);
    }

    template <typename... Params>
    static void log(LogLevel level, const std::string &fmt, Params... params)
    {
        boost::format boostFmt(fmt);
        log(level, boostFmt, params...);
    }

private:
    Log() = delete;
    Log(const Log &) = delete;
    Log(Log &&) = delete;
};
