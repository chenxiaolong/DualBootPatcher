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

#include <cerrno>
#include <cstdlib>

#include <sys/stat.h>
#include <unistd.h>

#if __ANDROID_API__ >= 21
#include <sys/statvfs.h>
#endif

#include <jni.h>

#include "mblog/logging.h"

#define CLASS_METHOD(method) \
    Java_com_github_chenxiaolong_dualbootpatcher_nativelib_libmiscstuff_Constants_ ## method

extern "C" {

static jboolean set_int_constant(JNIEnv *env, jclass clazz,
                                 const char *field_name, jint value)
{
    jfieldID field = env->GetStaticFieldID(clazz, field_name, "I");
    if (!field) {
        // Java will throw exception
        return JNI_FALSE;
    }

    env->SetStaticIntField(clazz, field, value);
    return JNI_TRUE;
}

JNIEXPORT void JNICALL CLASS_METHOD(initConstants)(JNIEnv *env, jclass clazz)
{
#define SET_CONSTANT(item) \
    LOGE("SETTING %s to %d", #item, item); \
    if (!set_int_constant(env, clazz, #item, item)) { \
        return; \
    }

#if EWOULDBLOCK != EAGAIN
#error EWOULDBLOCK != EAGAIN
#endif

    // BEGIN: errno.h
    SET_CONSTANT(EPERM);            /* 1 */
    SET_CONSTANT(ENOENT);           /* 2 */
    SET_CONSTANT(ESRCH);            /* 3 */
    SET_CONSTANT(EINTR);            /* 4 */
    SET_CONSTANT(EIO);              /* 5 */
    SET_CONSTANT(ENXIO);            /* 6 */
    SET_CONSTANT(E2BIG);            /* 7 */
    SET_CONSTANT(ENOEXEC);          /* 8 */
    SET_CONSTANT(EBADF);            /* 9 */
    SET_CONSTANT(ECHILD);           /* 10 */
    SET_CONSTANT(EAGAIN);           /* 11 */
    SET_CONSTANT(ENOMEM);           /* 12 */
    SET_CONSTANT(EACCES);           /* 13 */
    SET_CONSTANT(EFAULT);           /* 14 */
    SET_CONSTANT(ENOTBLK);          /* 15 */
    SET_CONSTANT(EBUSY);            /* 16 */
    SET_CONSTANT(EEXIST);           /* 17 */
    SET_CONSTANT(EXDEV);            /* 18 */
    SET_CONSTANT(ENODEV);           /* 19 */
    SET_CONSTANT(ENOTDIR);          /* 20 */
    SET_CONSTANT(EISDIR);           /* 21 */
    SET_CONSTANT(EINVAL);           /* 22 */
    SET_CONSTANT(ENFILE);           /* 23 */
    SET_CONSTANT(EMFILE);           /* 24 */
    SET_CONSTANT(ENOTTY);           /* 25 */
    SET_CONSTANT(ETXTBSY);          /* 26 */
    SET_CONSTANT(EFBIG);            /* 27 */
    SET_CONSTANT(ENOSPC);           /* 28 */
    SET_CONSTANT(ESPIPE);           /* 29 */
    SET_CONSTANT(EROFS);            /* 30 */
    SET_CONSTANT(EMLINK);           /* 31 */
    SET_CONSTANT(EPIPE);            /* 32 */
    SET_CONSTANT(EDOM);             /* 33 */
    SET_CONSTANT(ERANGE);           /* 34 */
    SET_CONSTANT(EDEADLK);          /* 35 */
    SET_CONSTANT(ENAMETOOLONG);     /* 36 */
    SET_CONSTANT(ENOLCK);           /* 37 */
    SET_CONSTANT(ENOSYS);           /* 38 */
    SET_CONSTANT(ENOTEMPTY);        /* 39 */
    SET_CONSTANT(ELOOP);            /* 40 */
    SET_CONSTANT(ENOMSG);           /* 42 */
    SET_CONSTANT(EIDRM);            /* 43 */
    SET_CONSTANT(ECHRNG);           /* 44 */
    SET_CONSTANT(EL2NSYNC);         /* 45 */
    SET_CONSTANT(EL3HLT);           /* 46 */
    SET_CONSTANT(EL3RST);           /* 47 */
    SET_CONSTANT(ELNRNG);           /* 48 */
    SET_CONSTANT(EUNATCH);          /* 49 */
    SET_CONSTANT(ENOCSI);           /* 50 */
    SET_CONSTANT(EL2HLT);           /* 51 */
    SET_CONSTANT(EBADE);            /* 52 */
    SET_CONSTANT(EBADR);            /* 53 */
    SET_CONSTANT(EXFULL);           /* 54 */
    SET_CONSTANT(ENOANO);           /* 55 */
    SET_CONSTANT(EBADRQC);          /* 56 */
    SET_CONSTANT(EBADSLT);          /* 57 */
    SET_CONSTANT(EBFONT);           /* 59 */
    SET_CONSTANT(ENOSTR);           /* 60 */
    SET_CONSTANT(ENODATA);          /* 61 */
    SET_CONSTANT(ETIME);            /* 62 */
    SET_CONSTANT(ENOSR);            /* 63 */
    SET_CONSTANT(ENONET);           /* 64 */
    SET_CONSTANT(ENOPKG);           /* 65 */
    SET_CONSTANT(EREMOTE);          /* 66 */
    SET_CONSTANT(ENOLINK);          /* 67 */
    SET_CONSTANT(EADV);             /* 68 */
    SET_CONSTANT(ESRMNT);           /* 69 */
    SET_CONSTANT(ECOMM);            /* 70 */
    SET_CONSTANT(EPROTO);           /* 71 */
    SET_CONSTANT(EMULTIHOP);        /* 72 */
    SET_CONSTANT(EDOTDOT);          /* 73 */
    SET_CONSTANT(EBADMSG);          /* 74 */
    SET_CONSTANT(EOVERFLOW);        /* 75 */
    SET_CONSTANT(ENOTUNIQ);         /* 76 */
    SET_CONSTANT(EBADFD);           /* 77 */
    SET_CONSTANT(EREMCHG);          /* 78 */
    SET_CONSTANT(ELIBACC);          /* 79 */
    SET_CONSTANT(ELIBBAD);          /* 80 */
    SET_CONSTANT(ELIBSCN);          /* 81 */
    SET_CONSTANT(ELIBMAX);          /* 82 */
    SET_CONSTANT(ELIBEXEC);         /* 83 */
    SET_CONSTANT(EILSEQ);           /* 84 */
    SET_CONSTANT(ERESTART);         /* 85 */
    SET_CONSTANT(ESTRPIPE);         /* 86 */
    SET_CONSTANT(EUSERS);           /* 87 */
    SET_CONSTANT(ENOTSOCK);         /* 88 */
    SET_CONSTANT(EDESTADDRREQ);     /* 89 */
    SET_CONSTANT(EMSGSIZE);         /* 90 */
    SET_CONSTANT(EPROTOTYPE);       /* 91 */
    SET_CONSTANT(ENOPROTOOPT);      /* 92 */
    SET_CONSTANT(EPROTONOSUPPORT);  /* 93 */
    SET_CONSTANT(ESOCKTNOSUPPORT);  /* 94 */
    SET_CONSTANT(EOPNOTSUPP);       /* 95 */
    SET_CONSTANT(EPFNOSUPPORT);     /* 96 */
    SET_CONSTANT(EAFNOSUPPORT);     /* 97 */
    SET_CONSTANT(EADDRINUSE);       /* 98 */
    SET_CONSTANT(EADDRNOTAVAIL);    /* 99 */
    SET_CONSTANT(ENETDOWN);         /* 100 */
    SET_CONSTANT(ENETUNREACH);      /* 101 */
    SET_CONSTANT(ENETRESET);        /* 102 */
    SET_CONSTANT(ECONNABORTED);     /* 103 */
    SET_CONSTANT(ECONNRESET);       /* 104 */
    SET_CONSTANT(ENOBUFS);          /* 105 */
    SET_CONSTANT(EISCONN);          /* 106 */
    SET_CONSTANT(ENOTCONN);         /* 107 */
    SET_CONSTANT(ESHUTDOWN);        /* 108 */
    SET_CONSTANT(ETOOMANYREFS);     /* 109 */
    SET_CONSTANT(ETIMEDOUT);        /* 110 */
    SET_CONSTANT(ECONNREFUSED);     /* 111 */
    SET_CONSTANT(EHOSTDOWN);        /* 112 */
    SET_CONSTANT(EHOSTUNREACH);     /* 113 */
    SET_CONSTANT(EALREADY);         /* 114 */
    SET_CONSTANT(EINPROGRESS);      /* 115 */
    SET_CONSTANT(ESTALE);           /* 116 */
    SET_CONSTANT(EUCLEAN);          /* 117 */
    SET_CONSTANT(ENOTNAM);          /* 118 */
    SET_CONSTANT(ENAVAIL);          /* 119 */
    SET_CONSTANT(EISNAM);           /* 120 */
    SET_CONSTANT(EREMOTEIO);        /* 121 */
    SET_CONSTANT(EDQUOT);           /* 122 */
    SET_CONSTANT(ENOMEDIUM);        /* 123 */
    SET_CONSTANT(EMEDIUMTYPE);      /* 124 */
    SET_CONSTANT(ECANCELED);        /* 125 */
    SET_CONSTANT(ENOKEY);           /* 126 */
    SET_CONSTANT(EKEYEXPIRED);      /* 127 */
    SET_CONSTANT(EKEYREVOKED);      /* 128 */
    SET_CONSTANT(EKEYREJECTED);     /* 129 */
    SET_CONSTANT(EOWNERDEAD);       /* 130 */
    SET_CONSTANT(ENOTRECOVERABLE);  /* 131 */
    // END: errno.h

    // BEGIN: linux/stat.h
    SET_CONSTANT(S_IFMT);           /* 00170000 */
    SET_CONSTANT(S_IFSOCK);         /* 0140000 */
    SET_CONSTANT(S_IFLNK);          /* 0120000 */
    SET_CONSTANT(S_IFREG);          /* 0100000 */
    SET_CONSTANT(S_IFBLK);          /* 0060000 */
    SET_CONSTANT(S_IFDIR);          /* 0040000 */
    SET_CONSTANT(S_IFCHR);          /* 0020000 */
    SET_CONSTANT(S_IFIFO);          /* 0010000 */
    SET_CONSTANT(S_ISUID);          /* 0004000 */
    SET_CONSTANT(S_ISGID);          /* 0002000 */
    SET_CONSTANT(S_ISVTX);          /* 0001000 */
    SET_CONSTANT(S_IRWXU);          /* 00700 */
    SET_CONSTANT(S_IRUSR);          /* 00400 */
    SET_CONSTANT(S_IWUSR);          /* 00200 */
    SET_CONSTANT(S_IXUSR);          /* 00100 */
    SET_CONSTANT(S_IRWXG);          /* 00070 */
    SET_CONSTANT(S_IRGRP);          /* 00040 */
    SET_CONSTANT(S_IWGRP);          /* 00020 */
    SET_CONSTANT(S_IXGRP);          /* 00010 */
    SET_CONSTANT(S_IRWXO);          /* 00007 */
    SET_CONSTANT(S_IROTH);          /* 00004 */
    SET_CONSTANT(S_IWOTH);          /* 00002 */
    SET_CONSTANT(S_IXOTH);          /* 00001 */
    // END: linux/stat.h

    // BEGIN: sys/statvfs.h
#if __ANDROID_API__ >= 21
    SET_CONSTANT(ST_RDONLY);        /* 0x0001 */
    SET_CONSTANT(ST_NOSUID);        /* 0x0002 */
    SET_CONSTANT(ST_NODEV);         /* 0x0004 */
    SET_CONSTANT(ST_NOEXEC);        /* 0x0008 */
    SET_CONSTANT(ST_SYNCHRONOUS);   /* 0x0010 */
    SET_CONSTANT(ST_MANDLOCK);      /* 0x0040 */
    SET_CONSTANT(ST_NOATIME);       /* 0x0400 */
    SET_CONSTANT(ST_NODIRATIME);    /* 0x0800 */
    SET_CONSTANT(ST_RELATIME);      /* 0x1000 */
#endif
    // END: sys/statvfs.h

    // BEGIN: unistd.h
    SET_CONSTANT(STDIN_FILENO);     /* 0 */
    SET_CONSTANT(STDOUT_FILENO);    /* 1 */
    SET_CONSTANT(STDERR_FILENO);    /* 2 */
    // END: unistd.h
}

}
