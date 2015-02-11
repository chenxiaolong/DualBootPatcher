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

#include "util/logging.h"

#include <unordered_map>

#define LOG_TAG "mbtool"

namespace MB {

static LogTarget log_target;
static std::unordered_map<int, std::string> names;
//static std::array<std::string, 5> targets;

void log_set_target(LogTarget target)
{
    log_target = target;
}

std::shared_ptr<spdlog::logger> logger()
{
    if (names.empty()) {
        names[static_cast<int>(LogTarget::KLOG)] = "klog";
#ifdef USE_ANDROID_LOG
        names[static_cast<int>(LogTarget::LOGCAT)] = "logcat";
#endif
        names[static_cast<int>(LogTarget::STDOUT)] = "stdout";
        names[static_cast<int>(LogTarget::STDERR)] = "stderr";
    }

    LogTarget target = log_target;
    if (target == LogTarget::DEFAULT) {
        target = LogTarget::STDOUT;
    }

    auto logptr = spdlog::get(names[static_cast<int>(target)]);
    if (!logptr) {
        switch (target) {
        case LogTarget::KLOG:
            return spdlog::klog_logger(
                    names[static_cast<int>(LogTarget::KLOG)], LOG_TAG);
#ifdef USE_ANDROID_LOG
        case LogTarget::LOGCAT:
            return spdlog::androidlog_logger(
                    names[static_cast<int>(LogTarget::LOGCAT)], LOG_TAG);
#endif
        case LogTarget::STDOUT:
#if 0
            return spdlog::stdout_logger_st(
                    names[static_cast<int>(LogTarget::STDOUT)]);
#else
            return spdlog::c_stdout_logger(
                    names[static_cast<int>(LogTarget::STDOUT)]);
#endif
        case LogTarget::STDERR:
#if 0
            return spdlog::stderr_logger_st(
                    names[static_cast<int>(LogTarget::STDERR)]);
#else
            return spdlog::c_stderr_logger(
                    names[static_cast<int>(LogTarget::STDERR)]);
#endif
        case LogTarget::DEFAULT:
            // Not reached
            break;
        }
    }
    return logptr;
}

}