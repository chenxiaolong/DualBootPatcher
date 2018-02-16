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

#include "mbcommon/common.h"
#include "mbcommon/fmt.h"

#include "mblog/log_level.h"

#define FMT_LOG(EXPR) \
    do { \
        using namespace fmt::literals; \
        EXPR; \
    } while (0)

#define TLOGE(TAG, FMT, ...) \
    FMT_LOG(mb::log::log(mb::log::LogLevel::Error, (TAG), FMT ## _format(__VA_ARGS__)))
#define TLOGW(TAG, FMT, ...) \
    FMT_LOG(mb::log::log(mb::log::LogLevel::Warning, (TAG), FMT ## _format(__VA_ARGS__)))
#define TLOGI(TAG, FMT, ...) \
    FMT_LOG(mb::log::log(mb::log::LogLevel::Info, (TAG), FMT ## _format(__VA_ARGS__)))
#define TLOGD(TAG, FMT, ...) \
    FMT_LOG(mb::log::log(mb::log::LogLevel::Debug, (TAG), FMT ## _format(__VA_ARGS__)))
#define TLOGV(TAG, FMT, ...) \
    FMT_LOG(mb::log::log(mb::log::LogLevel::Verbose, (TAG), FMT ## _format(__VA_ARGS__)))

#define LOGE(...) TLOGE(LOG_TAG, __VA_ARGS__)
#define LOGW(...) TLOGW(LOG_TAG, __VA_ARGS__)
#define LOGI(...) TLOGI(LOG_TAG, __VA_ARGS__)
#define LOGD(...) TLOGD(LOG_TAG, __VA_ARGS__)
#define LOGV(...) TLOGV(LOG_TAG, __VA_ARGS__)

namespace mb::log
{

class BaseLogger;

MB_EXPORT std::shared_ptr<BaseLogger> logger();
MB_EXPORT void set_logger(std::shared_ptr<BaseLogger> logger);

MB_EXPORT void log(LogLevel prio, std::string tag, std::string msg);

MB_EXPORT std::string format();
MB_EXPORT void set_format(std::string fmt);

}
