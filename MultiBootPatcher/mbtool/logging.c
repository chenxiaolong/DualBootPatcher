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

#include "logging.h"

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
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


#define KMSG_BUF_SIZE 512

static int kmsg_fd = -1;

void klog_init(void)
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

void kmsg_write(const char *fmt, va_list ap)
{
    if (kmsg_fd < 0) {
        klog_init();
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

void logmsg(int prio, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    if (log_output == KERNEL) {
        const char *kmsg_prio = NULL;
        char newfmt[100];

        switch (prio) {
        case LOG_ERROR:   kmsg_prio = KMSG_LEVEL_ERROR;   break;
        case LOG_WARNING: kmsg_prio = KMSG_LEVEL_WARNING; break;
        case LOG_INFO:    kmsg_prio = KMSG_LEVEL_INFO;    break;
        case LOG_DEBUG:   kmsg_prio = KMSG_LEVEL_DEBUG;   break;
        case LOG_VERBOSE: kmsg_prio = KMSG_LEVEL_DEFAULT; break;
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
        case LOG_ERROR: logcat_prio = ANDROID_LOG_ERROR;     break;
        case LOG_WARNING: logcat_prio = ANDROID_LOG_WARN;    break;
        case LOG_INFO:    logcat_prio = ANDROID_LOG_INFO;    break;
        case LOG_DEBUG:   logcat_prio = ANDROID_LOG_DEBUG;   break;
        case LOG_VERBOSE: logcat_prio = ANDROID_LOG_VERBOSE; break;
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
        case LOG_ERROR:   stdlog_prio = STDLOG_LEVEL_ERROR;   stream = stderr; break;
        case LOG_WARNING: stdlog_prio = STDLOG_LEVEL_WARNING; stream = stderr; break;
        case LOG_INFO:    stdlog_prio = STDLOG_LEVEL_INFO;    stream = stdout; break;
        case LOG_DEBUG:   stdlog_prio = STDLOG_LEVEL_DEBUG;   stream = stdout; break;
        case LOG_VERBOSE: stdlog_prio = STDLOG_LEVEL_VERBOSE; stream = stdout; break;
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

void use_default_log_output(void)
{
    log_output = DEFAULT_LOG_OUTPUT;
}

void use_standard_log_output(void)
{
    log_output = STANDARD;
}

void use_logcat_log_output(void)
{
#ifdef USE_ANDROID_LOG
    log_output = LOGCAT;
#else
    log_output = STANDARD;
#endif
}

void use_kernel_log_output(void)
{
    log_output = KERNEL;
}
