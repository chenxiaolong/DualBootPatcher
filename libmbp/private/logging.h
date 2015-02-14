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

#include <memory>

#include <cppformat/format.h>

#include "../logging.h"


#define LOGE(str) log(MBP::LogLevel::Error, str)
#define LOGW(str) log(MBP::LogLevel::Warning, str)
#define LOGI(str) log(MBP::LogLevel::Info, str)
#define LOGD(str) log(MBP::LogLevel::Debug, str)
#define LOGV(str) log(MBP::LogLevel::Verbose, str)
#define FLOGE(...) log(MBP::LogLevel::Error, fmt::format(__VA_ARGS__))
#define FLOGW(...) log(MBP::LogLevel::Warning, fmt::format(__VA_ARGS__))
#define FLOGI(...) log(MBP::LogLevel::Info, fmt::format(__VA_ARGS__))
#define FLOGD(...) log(MBP::LogLevel::Debug, fmt::format(__VA_ARGS__))
#define FLOGV(...) log(MBP::LogLevel::Verbose, fmt::format(__VA_ARGS__))

void log(MBP::LogLevel level, const std::string &str);
