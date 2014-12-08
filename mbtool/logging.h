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

#define LOGE(...) logmsg(LOG_ERROR,   __VA_ARGS__)
#define LOGW(...) logmsg(LOG_WARNING, __VA_ARGS__)
#define LOGI(...) logmsg(LOG_INFO,    __VA_ARGS__)
#define LOGD(...) logmsg(LOG_DEBUG,   __VA_ARGS__)
#define LOGV(...) logmsg(LOG_VERBOSE, __VA_ARGS__)

enum loglevels {
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG,
    LOG_VERBOSE
};

void klog_init(void);

void logmsg(int prio, const char *fmt, ...);

void use_default_log_output(void);
void use_standard_log_output(void);
void use_logcat_log_output(void);
void use_kernel_log_output(void);
