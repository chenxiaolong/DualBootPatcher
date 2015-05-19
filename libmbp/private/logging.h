/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "../logging.h"
#include "private/stringutils.h"


#define LOGE(str) log(mbp::LogLevel::Error, str)
#define LOGW(str) log(mbp::LogLevel::Warning, str)
#define LOGI(str) log(mbp::LogLevel::Info, str)
#define LOGD(str) log(mbp::LogLevel::Debug, str)
#define LOGV(str) log(mbp::LogLevel::Verbose, str)
#define FLOGE(...) log(mbp::LogLevel::Error, StringUtils::format(__VA_ARGS__))
#define FLOGW(...) log(mbp::LogLevel::Warning, StringUtils::format(__VA_ARGS__))
#define FLOGI(...) log(mbp::LogLevel::Info, StringUtils::format(__VA_ARGS__))
#define FLOGD(...) log(mbp::LogLevel::Debug, StringUtils::format(__VA_ARGS__))
#define FLOGV(...) log(mbp::LogLevel::Verbose, StringUtils::format(__VA_ARGS__))

void log(mbp::LogLevel level, const std::string &str);
