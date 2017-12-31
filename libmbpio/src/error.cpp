/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbpio/error.h"

#ifdef __ANDROID__
#  include <pthread.h>
#endif

namespace mb::io
{

#ifdef __ANDROID__
// The NDK doesn't support TLS
static pthread_once_t g_tls_once = PTHREAD_ONCE_INIT;
static pthread_key_t g_tls_key_error = 0;
static pthread_key_t g_tls_key_error_string = 0;
#else
thread_local Error g_error;
thread_local std::string g_error_string;
#endif

#ifdef __ANDROID__
static void _destroy_error(void *st)
{
    delete static_cast<Error *>(st);
}

static void _destroy_error_string(void *st)
{
    delete static_cast<std::string *>(st);
}

static void _init_tls_keys()
{
    pthread_key_create(&g_tls_key_error, _destroy_error);
    pthread_key_create(&g_tls_key_error_string, _destroy_error_string);
    pthread_setspecific(g_tls_key_error, new int());
    pthread_setspecific(g_tls_key_error_string, new std::string());
}
#endif

Error last_error()
{
#ifdef __ANDROID__
    if (pthread_once(&g_tls_once, _init_tls_keys) == 0) {
        Error *ptr = static_cast<Error *>(pthread_getspecific(g_tls_key_error));
        if (ptr) {
            return *ptr;
        }
    }
    return Error::NoError;
#else
    return g_error;
#endif
}

std::string last_error_string()
{
#ifdef __ANDROID__
    if (pthread_once(&g_tls_once, _init_tls_keys) == 0) {
        std::string *ptr = static_cast<std::string *>(
                pthread_getspecific(g_tls_key_error_string));
        if (ptr) {
            return *ptr;
        }
    }
    return {};
#else
    return g_error_string;
#endif
}

void set_last_error(Error error, std::string error_string)
{
#ifdef __ANDROID__
    if (pthread_once(&g_tls_once, _init_tls_keys) == 0) {
        Error *tls_error = static_cast<Error *>(
                pthread_getspecific(g_tls_key_error));
        if (tls_error) {
            *tls_error = error;
        }
        std::string *tls_error_string = static_cast<std::string *>(
                pthread_getspecific(g_tls_key_error_string));
        if (tls_error_string) {
            *tls_error_string = std::move(error_string);
        }
    }
#else
    g_error = error;
    g_error_string = std::move(error_string);
#endif
}

}
