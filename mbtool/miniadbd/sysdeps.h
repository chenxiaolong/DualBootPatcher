/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* this file contains system-dependent definitions used by ADB
 * they're related to threads, sockets and file descriptors
 */
#ifndef _ADB_SYSDEPS_H
#define _ADB_SYSDEPS_H

#include <cstdarg>

#include <fcntl.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static __inline__ void close_on_exec(int fd)
{
    fcntl(fd, F_SETFD, FD_CLOEXEC);
}

static __inline__ int unix_open(const char* path, int options, ...)
{
    if ((options & O_CREAT) == 0) {
        return TEMP_FAILURE_RETRY(open(path, options));
    } else {
        int      mode;
        va_list  args;
        va_start(args, options);
        mode = va_arg(args, int);
        va_end(args);
        return TEMP_FAILURE_RETRY(open(path, options, mode));
    }
}

static __inline__ int adb_open_mode(const char* pathname, int options, int mode)
{
    return TEMP_FAILURE_RETRY(open(pathname, options, mode));
}


static __inline__ int adb_open(const char* pathname, int options)
{
    int  fd = TEMP_FAILURE_RETRY(open(pathname, options));
    if (fd < 0)
        return -1;
    close_on_exec( fd );
    return fd;
}
#undef   open
#define  open    ___xxx_open


static __inline__ int adb_read(int fd, void* buf, size_t len)
{
    return TEMP_FAILURE_RETRY(read(fd, buf, len));
}

#undef   read
#define  read  ___xxx_read

static __inline__ int adb_write(int fd, const void* buf, size_t len)
{
    return TEMP_FAILURE_RETRY(write(fd, buf, len));
}
#undef   write
#define  write  ___xxx_write

typedef void* (*adb_thread_func_t)(void* arg);

static __inline__ int adb_thread_create(pthread_t *pthread, adb_thread_func_t start, void* arg)
{
    pthread_attr_t   attr;

    pthread_attr_init (&attr);
    pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_DETACHED);

    return pthread_create( pthread, &attr, start, arg );
}

static __inline__ int unix_socketpair(int d, int type, int protocol, int sv[2])
{
    return socketpair(d, type, protocol, sv);
}

static __inline__ int adb_socketpair(int sv[2])
{
    int rc;

    rc = unix_socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    if (rc < 0)
        return -1;

    close_on_exec(sv[0]);
    close_on_exec(sv[1]);
    return 0;
}

#undef   socketpair
#define  socketpair   ___xxx_socketpair

static __inline__ void adb_sleep_ms(int mseconds)
{
    usleep(mseconds * 1000);
}

static __inline__ char* adb_dirstart(const char* path)
{
    return (char *) strchr(path, '/');
}

#endif /* _ADB_SYSDEPS_H */
