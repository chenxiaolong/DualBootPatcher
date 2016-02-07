/*
 * Copyright (C) 2014-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mblog/logging.h"

#include <cerrno>

#include "mblog/stdio_logger.h"

namespace mb
{
namespace log
{

static std::shared_ptr<BaseLogger> logger;

void log_set_logger(std::shared_ptr<BaseLogger> logger_local)
{
    logger = std::move(logger_local);
}

void log(LogLevel prio, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    logv(prio, fmt, ap);

    va_end(ap);
}

void logv(LogLevel prio, const char *fmt, va_list ap)
{
    int saved_errno = errno;

    if (!logger) {
        logger = std::make_shared<StdioLogger>(stdout, false);
    }

    logger->log(prio, fmt, ap);

    errno = saved_errno;
}

}
}
