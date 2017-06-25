/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "gui/monotonic_cond.hpp"

#include <cerrno>
#include <cstring>

#include "mblog/logging.h"

MonotonicCond::MonotonicCond::MonotonicCond() : m_initialized(false)
{
    if (pthread_condattr_init(&m_condattr) < 0) {
        LOGW("Failed to initialize pthread_condattr_t: %s", strerror(errno));
        return;
    }
    if (pthread_condattr_setclock(&m_condattr, CLOCK_MONOTONIC) < 0) {
        LOGW("Failed to set monotonic clock for pthread_condattr_t: %s",
             strerror(errno));
        pthread_condattr_destroy(&m_condattr);
        return;
    }
    if (pthread_cond_init(&m_cond, &m_condattr) < 0) {
        LOGW("Failed to initialize pthread_cond_t: %s", strerror(errno));
        pthread_condattr_destroy(&m_condattr);
        return;
    }
    m_initialized = true;
}

MonotonicCond::~MonotonicCond()
{
    if (m_initialized) {
        pthread_cond_destroy(&m_cond);
        pthread_condattr_destroy(&m_condattr);
    }
}

MonotonicCond::operator pthread_cond_t * ()
{
    return m_initialized ? &m_cond : nullptr;
}
