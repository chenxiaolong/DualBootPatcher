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

#include <stdio.h>

#define LOGE(...) mb_logmsg(MB_LOG_ERROR,   __VA_ARGS__)
#define LOGW(...) mb_logmsg(MB_LOG_WARNING, __VA_ARGS__)
#define LOGI(...) mb_logmsg(MB_LOG_INFO,    __VA_ARGS__)
#define LOGD(...) mb_logmsg(MB_LOG_DEBUG,   __VA_ARGS__)
#define LOGV(...) mb_logmsg(MB_LOG_VERBOSE, __VA_ARGS__)

enum loglevels {
    MB_LOG_ERROR,
    MB_LOG_WARNING,
    MB_LOG_INFO,
    MB_LOG_DEBUG,
    MB_LOG_VERBOSE
};

void mb_klog_init(void);

__attribute__((format(printf, 2, 3)))
void mb_logmsg(int prio, const char *fmt, ...);

void mb_log_use_default_output(void);
void mb_log_use_standard_output(void);
void mb_log_use_logcat_output(void);
void mb_log_use_kernel_output(void);
void mb_log_set_standard_stream(int prio, FILE *stream);
void mb_log_set_default_standard_stream(int prio);
void mb_log_set_standard_stream_all(FILE *stream);
void mb_log_set_default_standard_stream_all(void);
