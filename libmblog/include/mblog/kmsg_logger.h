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

#include "mblog/base_logger.h"

#define KMSG_BUF_SIZE 512

namespace mb
{
namespace log
{

class MB_EXPORT KmsgLogger : public BaseLogger
{
public:
    KmsgLogger(bool force_error_prio);

    virtual ~KmsgLogger();

    virtual void log(LogLevel prio, const char *fmt, va_list ap) override;

private:
    int _fd;
    char _buf[KMSG_BUF_SIZE];
    bool _force_error_prio;
};

}
}
