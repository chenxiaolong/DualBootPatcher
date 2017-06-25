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
#include "mblog/base_logger.h"
#include "mblog/log_level.h"

#define LOGE(...) mb::log::log(mb::log::LogLevel::Error, __VA_ARGS__)
#define LOGW(...) mb::log::log(mb::log::LogLevel::Warning, __VA_ARGS__)
#define LOGI(...) mb::log::log(mb::log::LogLevel::Info, __VA_ARGS__)
#define LOGD(...) mb::log::log(mb::log::LogLevel::Debug, __VA_ARGS__)
#define LOGV(...) mb::log::log(mb::log::LogLevel::Verbose, __VA_ARGS__)

#define VLOGE(...) mb::log::logv(mb::log::LogLevel::Error, __VA_ARGS__)
#define VLOGW(...) mb::log::logv(mb::log::LogLevel::Warning, __VA_ARGS__)
#define VLOGI(...) mb::log::logv(mb::log::LogLevel::Info, __VA_ARGS__)
#define VLOGD(...) mb::log::logv(mb::log::LogLevel::Debug, __VA_ARGS__)
#define VLOGV(...) mb::log::logv(mb::log::LogLevel::Verbose, __VA_ARGS__)

namespace mb
{
namespace log
{

MB_EXPORT const char * get_log_tag();
MB_EXPORT void set_log_tag(const char *tag);
MB_EXPORT void log_set_logger(std::shared_ptr<BaseLogger> logger);
MB_PRINTF(2, 3)
MB_EXPORT void log(LogLevel prio, const char *fmt, ...);
MB_EXPORT void logv(LogLevel prio, const char *fmt, va_list ap);

}
}
