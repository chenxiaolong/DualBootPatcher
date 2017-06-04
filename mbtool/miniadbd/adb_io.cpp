/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "sysdeps.h"

#include <cstdlib>
#include <cstring>

#include "adb_io.h"
#include "adb_log.h"
#include "adb_utils.h"

#include "mbcommon/string.h"
#include "mbutil/string.h"

bool SendProtocolString(int fd, const std::string& s) {
    int length = s.size();
    if (length > 0xffff) {
        length = 0xffff;
    }

    return WriteFdFmt(fd, "%04x", length) && WriteFdExactly(fd, s);
}

bool SendOkay(int fd) {
    return WriteFdExactly(fd, "OKAY", 4);
}

bool SendFail(int fd, const std::string& reason) {
    return WriteFdExactly(fd, "FAIL", 4) && SendProtocolString(fd, reason);
}

bool ReadFdExactly(int fd, void* buf, size_t len) {
    char* p = reinterpret_cast<char*>(buf);

    size_t len0 = len;

    ADB_LOGD(ADB_LLIO, "readx: fd=%d wanted=%zu", fd, len);
    while (len > 0) {
        int r = adb_read(fd, p, len);
        if (r > 0) {
            len -= r;
            p += r;
        } else if (r == -1) {
            ADB_LOGE(ADB_LLIO, "readx: fd=%d error %d: %s",
                     fd, errno, strerror(errno));
            return false;
        } else {
            ADB_LOGE(ADB_LLIO, "readx: fd=%d disconnected", fd);
            errno = 0;
            return false;
        }
    }

    ADB_LOGD(ADB_LLIO, "readx: fd=%d wanted=%zu got=%zu", fd, len0, len0 - len);
#if 0
    dump_hex(reinterpret_cast<const unsigned char*>(buf), len0);
#endif

    return true;
}

bool WriteFdExactly(int fd, const void* buf, size_t len) {
    const char* p = reinterpret_cast<const char*>(buf);
    int r;

    ADB_LOGD(ADB_LLIO, "writex: fd=%d len=%d: ", fd, (int)len);
#if 0
    dump_hex(reinterpret_cast<const unsigned char*>(buf), len);
#endif

    while (len > 0) {
        r = adb_write(fd, p, len);
        if (r == -1) {
            ADB_LOGE(ADB_LLIO, "writex: fd=%d error %d: %s",
                     fd, errno, strerror(errno));
            if (errno == EAGAIN) {
                adb_sleep_ms(1); // just yield some cpu time
                continue;
            } else if (errno == EPIPE) {
                ADB_LOGE(ADB_LLIO, "writex: fd=%d disconnected", fd);
                errno = 0;
                return false;
            } else {
                return false;
            }
        } else {
            len -= r;
            p += r;
        }
    }
    return true;
}

bool WriteFdExactly(int fd, const char* str) {
    return WriteFdExactly(fd, str, strlen(str));
}

bool WriteFdExactly(int fd, const std::string& str) {
    return WriteFdExactly(fd, str.c_str(), str.size());
}

bool WriteFdFmt(int fd, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char *str = mb_format_v(fmt, ap);
    va_end(ap);

    bool ret = false;

    if (str) {
        ret = WriteFdExactly(fd, str);
        free(str);
    }

    return ret;
}
