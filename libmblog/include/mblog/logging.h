/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#pragma once

#include <memory>

#include <cstdarg>

#include "mbcommon/common.h"

#include "mblog/log_level.h"

#define TLOGE(TAG, ...) \
    mb::log::log(mb::log::LogLevel::Error, (TAG), __VA_ARGS__)
#define TLOGW(TAG, ...) \
    mb::log::log(mb::log::LogLevel::Warning, (TAG), __VA_ARGS__)
#define TLOGI(TAG, ...) \
    mb::log::log(mb::log::LogLevel::Info, (TAG), __VA_ARGS__)
#define TLOGD(TAG, ...) \
    mb::log::log(mb::log::LogLevel::Debug, (TAG), __VA_ARGS__)
#define TLOGV(TAG, ...) \
    mb::log::log(mb::log::LogLevel::Verbose, (TAG), __VA_ARGS__)

#define LOGE(...) TLOGE(LOG_TAG, __VA_ARGS__)
#define LOGW(...) TLOGW(LOG_TAG, __VA_ARGS__)
#define LOGI(...) TLOGI(LOG_TAG, __VA_ARGS__)
#define LOGD(...) TLOGD(LOG_TAG, __VA_ARGS__)
#define LOGV(...) TLOGV(LOG_TAG, __VA_ARGS__)

#define TVLOGE(TAG, ...) \
    mb::log::log_v(mb::log::LogLevel::Error, (TAG), __VA_ARGS__)
#define TVLOGW(TAG, ...) \
    mb::log::log_v(mb::log::LogLevel::Warning, (TAG), __VA_ARGS__)
#define TVLOGI(TAG, ...) \
    mb::log::log_v(mb::log::LogLevel::Info, (TAG), __VA_ARGS__)
#define TVLOGD(TAG, ...) \
    mb::log::log_v(mb::log::LogLevel::Debug, (TAG), __VA_ARGS__)
#define TVLOGV(TAG, ...) \
    mb::log::log_v(mb::log::LogLevel::Verbose, (TAG), __VA_ARGS__)

#define VLOGE(...) TVLOGE(LOG_TAG, __VA_ARGS__)
#define VLOGW(...) TVLOGW(LOG_TAG, __VA_ARGS__)
#define VLOGI(...) TVLOGI(LOG_TAG, __VA_ARGS__)
#define VLOGD(...) TVLOGD(LOG_TAG, __VA_ARGS__)
#define VLOGV(...) TVLOGV(LOG_TAG, __VA_ARGS__)

namespace mb::log
{

class BaseLogger;

MB_EXPORT std::shared_ptr<BaseLogger> logger();
MB_EXPORT void set_logger(std::shared_ptr<BaseLogger> logger);

MB_PRINTF(3, 4)
MB_EXPORT void log(LogLevel prio, const char *tag, const char *fmt, ...);
MB_EXPORT void log_v(LogLevel prio, const char *tag, const char *fmt, va_list ap);

MB_EXPORT std::string format();
MB_EXPORT void set_format(std::string fmt);

}
