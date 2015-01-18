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

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef USE_ANDROID_LOG
#include <android/log.h>
#endif

#define LOG_TAG "mbtool"

// Standard outputs
#define STDLOG_LEVEL_ERROR   "[Error]"
#define STDLOG_LEVEL_WARNING "[Warning]"
#define STDLOG_LEVEL_INFO    "[Info]"
#define STDLOG_LEVEL_DEBUG   "[Debug]"
#define STDLOG_LEVEL_VERBOSE "[Verbose]"

// Kernel logging (http://elinux.org/Debugging_by_printing#Log_Levels)
#define KMSG_LEVEL_ERROR   "<3>"
#define KMSG_LEVEL_WARNING "<4>"
#define KMSG_LEVEL_NOTICE  "<5>"
#define KMSG_LEVEL_INFO    "<6>"
#define KMSG_LEVEL_DEBUG   "<7>"
#define KMSG_LEVEL_DEFAULT "<d>"


enum log_outputs {
    STANDARD,
    LOGCAT,
    KERNEL
};

#define DEFAULT_LOG_OUTPUT STANDARD

static int log_output = STANDARD;


static FILE *std_stream_error = stderr;
static FILE *std_stream_warning = stderr;
static FILE *std_stream_info = stdout;
static FILE *std_stream_debug = stdout;
static FILE *std_stream_verbose = stdout;


#define KMSG_BUF_SIZE 512

static int kmsg_fd = -1;

void mb_klog_init(void)
{
    static int open_mode = O_WRONLY | O_NOCTTY | O_CLOEXEC;
    static const char *kmsg = "/dev/kmsg";
    static const char *kmsg2 = "/dev/kmsg.mbtool";

    if (kmsg_fd < 0) {
        kmsg_fd = open(kmsg, open_mode);
    }
    if (kmsg_fd < 0) {
        // If /dev/kmsg hasn't been created yet, then create our own
        // Character device mode: S_IFCHR | 0600
        //
        if (mknod(kmsg2, S_IFCHR | 0600, makedev(1, 11)) == 0) {
            kmsg_fd = open(kmsg2, open_mode);
            if (kmsg_fd >= 0) {
                unlink(kmsg2);
            }
        }
    }
}

static void kmsg_write(const char *fmt, va_list ap)
{
    if (kmsg_fd < 0) {
        mb_klog_init();
    }
    if (kmsg_fd < 0) {
        // error...
        return;
    }

    char buf[KMSG_BUF_SIZE];
    vsnprintf(buf, KMSG_BUF_SIZE, fmt, ap);
    buf[KMSG_BUF_SIZE - 1] = '\0';
    write(kmsg_fd, buf, strlen(buf));
}

void mb_logmsg(int prio, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    if (log_output == KERNEL) {
        const char *kmsg_prio = NULL;
        char newfmt[100];

        switch (prio) {
        case MB_LOG_ERROR:   kmsg_prio = KMSG_LEVEL_ERROR;   break;
        case MB_LOG_WARNING: kmsg_prio = KMSG_LEVEL_WARNING; break;
        case MB_LOG_INFO:    kmsg_prio = KMSG_LEVEL_INFO;    break;
        case MB_LOG_DEBUG:   kmsg_prio = KMSG_LEVEL_DEBUG;   break;
        case MB_LOG_VERBOSE: kmsg_prio = KMSG_LEVEL_DEFAULT; break;
        }

        if (!kmsg_prio) {
            kmsg_prio = KMSG_LEVEL_DEFAULT;
        }

        snprintf(newfmt, 100, "%s" LOG_TAG ": %s", kmsg_prio, fmt);
        kmsg_write(newfmt, ap);
#ifdef USE_ANDROID_LOG
    } else if (log_output == LOGCAT) {
        int logcat_prio = -1;

        switch (prio) {
        case MB_LOG_ERROR:   logcat_prio = ANDROID_LOG_ERROR;   break;
        case MB_LOG_WARNING: logcat_prio = ANDROID_LOG_WARN;    break;
        case MB_LOG_INFO:    logcat_prio = ANDROID_LOG_INFO;    break;
        case MB_LOG_DEBUG:   logcat_prio = ANDROID_LOG_DEBUG;   break;
        case MB_LOG_VERBOSE: logcat_prio = ANDROID_LOG_VERBOSE; break;
        }

        if (logcat_prio < 0) {
            logcat_prio = ANDROID_LOG_VERBOSE;
        }

        __android_log_vprint(logcat_prio, LOG_TAG, fmt, va);
#endif
    } else if (log_output == STANDARD) {
        FILE *stream = NULL;
        const char *stdlog_prio = NULL;
        const char newfmt[100];

        switch (prio) {
        case MB_LOG_ERROR:   stdlog_prio = STDLOG_LEVEL_ERROR;   stream = std_stream_error;   break;
        case MB_LOG_WARNING: stdlog_prio = STDLOG_LEVEL_WARNING; stream = std_stream_warning; break;
        case MB_LOG_INFO:    stdlog_prio = STDLOG_LEVEL_INFO;    stream = std_stream_info;    break;
        case MB_LOG_DEBUG:   stdlog_prio = STDLOG_LEVEL_DEBUG;   stream = std_stream_debug;   break;
        case MB_LOG_VERBOSE: stdlog_prio = STDLOG_LEVEL_VERBOSE; stream = std_stream_verbose; break;
        }

        if (!stdlog_prio) {
            stdlog_prio = STDLOG_LEVEL_VERBOSE;
            stream = stdout;
        }

        snprintf((char *) newfmt, 100, "%s %s\n", stdlog_prio, fmt);
        vfprintf(stream, newfmt, ap);
    }

    va_end(ap);
}

void mb_log_use_default_output(void)
{
    log_output = DEFAULT_LOG_OUTPUT;
}

void mb_log_use_standard_output(void)
{
    log_output = STANDARD;
}

void mb_log_use_logcat_output(void)
{
#ifdef USE_ANDROID_LOG
    log_output = LOGCAT;
#else
    log_output = STANDARD;
#endif
}

void mb_log_use_kernel_output(void)
{
    log_output = KERNEL;
}

void mb_log_set_standard_stream(int prio, FILE *stream)
{
    switch (prio) {
    case MB_LOG_ERROR:   std_stream_error = stream;   break;
    case MB_LOG_WARNING: std_stream_warning = stream; break;
    case MB_LOG_INFO:    std_stream_info = stream;    break;
    case MB_LOG_DEBUG:   std_stream_debug = stream;   break;
    case MB_LOG_VERBOSE: std_stream_verbose = stream; break;
    }
}

void mb_log_set_default_standard_stream(int prio)
{
    switch (prio) {
    case MB_LOG_ERROR:   std_stream_error = stderr;   break;
    case MB_LOG_WARNING: std_stream_warning = stderr; break;
    case MB_LOG_INFO:    std_stream_info = stdout;    break;
    case MB_LOG_DEBUG:   std_stream_debug = stdout;   break;
    case MB_LOG_VERBOSE: std_stream_verbose = stdout; break;
    }
}

void mb_log_set_standard_stream_all(FILE *stream)
{
    std_stream_error = stream;
    std_stream_warning = stream;
    std_stream_info = stream;
    std_stream_debug = stream;
    std_stream_verbose = stream;
}

void mb_log_set_default_standard_stream_all(void)
{
    std_stream_error = stderr;
    std_stream_warning = stderr;
    std_stream_info = stdout;
    std_stream_debug = stdout;
    std_stream_verbose = stdout;
}
