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
#ifdef USE_ANDROID_LOG
#include <android/log.h>
#endif


// Standard logging
#define LOG_TAG "mbtool"

#ifdef USE_ANDROID_LOG
#  define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,   LOG_TAG, __VA_ARGS__)
#  define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,   LOG_TAG, __VA_ARGS__)
#  define LOGI(...) __android_log_print(ANDROID_LOG_INFO,    LOG_TAG, __VA_ARGS__)
#  define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#  define LOGW(...) __android_log_print(ANDROID_LOG_WARN,    LOG_TAG, __VA_ARGS__)
#else
#  define LOGD(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#  define LOGE(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#  define LOGI(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#  define LOGV(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#  define LOGW(...) fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n")
#endif


// Kernel logging (http://elinux.org/Debugging_by_printing#Log_Levels)
#define KLOG_ERROR(...)   kmsg_write("<3>" LOG_TAG ": " __VA_ARGS__)
#define KLOG_WARNING(...) kmsg_write("<4>" LOG_TAG ": " __VA_ARGS__)
#define KLOG_NOTICE(...)  kmsg_write("<5>" LOG_TAG ": " __VA_ARGS__)
#define KLOG_INFO(...)    kmsg_write("<6>" LOG_TAG ": " __VA_ARGS__)
#define KLOG_DEBUG(...)   kmsg_write("<7>" LOG_TAG ": " __VA_ARGS__)
#define KLOG_DEFAULT(...) kmsg_write("<d>" LOG_TAG ": " __VA_ARGS__)
#define KLOG_CONT(...)    kmsg_write(__VA_ARGS__)

void kmsg_init();
void kmsg_cleanup();
void kmsg_write(const char *fmt, ...);
