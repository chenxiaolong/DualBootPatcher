/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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
#include <pthread.h>
#endif

namespace io
{

#ifdef __ANDROID__
// The NDK doesn't support TLS
static pthread_once_t gTLSOnce = PTHREAD_ONCE_INIT;
static pthread_key_t gTLSKeyError = 0;
static pthread_key_t gTLSKeyErrorString = 0;
#else
thread_local Error gError;
thread_local std::string gErrorString;
#endif

#ifdef __ANDROID__
void destroyError(void *st)
{
    Error *ptr = static_cast<Error *>(st);
    if (ptr) {
        delete ptr;
    }
}

void destroyErrorString(void *st)
{
    std::string *ptr = static_cast<std::string *>(st);
    if (ptr) {
        delete ptr;
    }
}

void initTLSKeys() {
    pthread_key_create(&gTLSKeyError, destroyError);
    pthread_key_create(&gTLSKeyErrorString, destroyErrorString);
    pthread_setspecific(gTLSKeyError, new int());
    pthread_setspecific(gTLSKeyErrorString, new std::string());
}
#endif

Error lastError()
{
#ifdef __ANDROID__
    if (pthread_once(&gTLSOnce, initTLSKeys) == 0) {
        Error *ptr = static_cast<Error *>(pthread_getspecific(gTLSKeyError));
        if (ptr) {
            return *ptr;
        }
    }
    return Error::NoError;
#else
    return gError;
#endif
}

std::string lastErrorString()
{
#ifdef __ANDROID__
    if (pthread_once(&gTLSOnce, initTLSKeys) == 0) {
        std::string *ptr = static_cast<std::string *>(
                pthread_getspecific(gTLSKeyErrorString));
        if (ptr) {
            return *ptr;
        }
    }
    return std::string();
#else
    return gErrorString;
#endif
}

void setLastError(Error error, std::string errorString)
{
#ifdef __ANDROID__
    if (pthread_once(&gTLSOnce, initTLSKeys) == 0) {
        Error *tlsError = static_cast<Error *>(
                pthread_getspecific(gTLSKeyError));
        if (tlsError) {
            *tlsError = error;
        }
        std::string *tlsErrorString = static_cast<std::string *>(
                pthread_getspecific(gTLSKeyErrorString));
        if (tlsErrorString) {
            *tlsErrorString = std::move(errorString);
        }
    }
#else
    gError = error;
    gErrorString = std::move(errorString);
#endif
}

}
