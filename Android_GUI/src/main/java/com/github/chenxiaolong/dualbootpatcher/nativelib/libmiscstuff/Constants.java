/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.github.chenxiaolong.dualbootpatcher.nativelib.libmiscstuff;

@SuppressWarnings({"unused", "WeakerAccess", "JniMissingFunction"})
public final class Constants {
    private Constants() {
    }

    private static int noInline() {
        return -1;
    }

    private static native void initConstants();

    public static boolean S_ISLNK(int mode) {
        return (mode & S_IFMT) == S_IFLNK;
    }

    public static boolean S_ISREG(int mode) {
        return (mode & S_IFMT) == S_IFREG;
    }

    public static boolean S_ISDIR(int mode) {
        return (mode & S_IFMT) == S_IFDIR;
    }

    public static boolean S_ISCHR(int mode) {
        return (mode & S_IFMT) == S_IFCHR;
    }

    public static boolean S_ISBLK(int mode) {
        return (mode & S_IFMT) == S_IFBLK;
    }

    public static boolean S_ISFIFO(int mode) {
        return (mode & S_IFMT) == S_IFIFO;
    }

    public static boolean S_ISSOCK(int mode) {
        return (mode & S_IFMT) == S_IFSOCK;
    }

    // BEGIN: errno.h
    public static final int EPERM = noInline();
    public static final int ENOENT = noInline();
    public static final int ESRCH = noInline();
    public static final int EINTR = noInline();
    public static final int EIO = noInline();
    public static final int ENXIO = noInline();
    public static final int E2BIG = noInline();
    public static final int ENOEXEC = noInline();
    public static final int EBADF = noInline();
    public static final int ECHILD = noInline();
    public static final int EAGAIN = noInline();
    public static final int ENOMEM = noInline();
    public static final int EACCES = noInline();
    public static final int EFAULT = noInline();
    public static final int ENOTBLK = noInline();
    public static final int EBUSY = noInline();
    public static final int EEXIST = noInline();
    public static final int EXDEV = noInline();
    public static final int ENODEV = noInline();
    public static final int ENOTDIR = noInline();
    public static final int EISDIR = noInline();
    public static final int EINVAL = noInline();
    public static final int ENFILE = noInline();
    public static final int EMFILE = noInline();
    public static final int ENOTTY = noInline();
    public static final int ETXTBSY = noInline();
    public static final int EFBIG = noInline();
    public static final int ENOSPC = noInline();
    public static final int ESPIPE = noInline();
    public static final int EROFS = noInline();
    public static final int EMLINK = noInline();
    public static final int EPIPE = noInline();
    public static final int EDOM = noInline();
    public static final int ERANGE = noInline();
    public static final int EDEADLK = noInline();
    public static final int ENAMETOOLONG = noInline();
    public static final int ENOLCK = noInline();
    public static final int ENOSYS = noInline();
    public static final int ENOTEMPTY = noInline();
    public static final int ELOOP = noInline();
    public static final int ENOMSG = noInline();
    public static final int EIDRM = noInline();
    public static final int ECHRNG = noInline();
    public static final int EL2NSYNC = noInline();
    public static final int EL3HLT = noInline();
    public static final int EL3RST = noInline();
    public static final int ELNRNG = noInline();
    public static final int EUNATCH = noInline();
    public static final int ENOCSI = noInline();
    public static final int EL2HLT = noInline();
    public static final int EBADE = noInline();
    public static final int EBADR = noInline();
    public static final int EXFULL = noInline();
    public static final int ENOANO = noInline();
    public static final int EBADRQC = noInline();
    public static final int EBADSLT = noInline();
    public static final int EBFONT = noInline();
    public static final int ENOSTR = noInline();
    public static final int ENODATA = noInline();
    public static final int ETIME = noInline();
    public static final int ENOSR = noInline();
    public static final int ENONET = noInline();
    public static final int ENOPKG = noInline();
    public static final int EREMOTE = noInline();
    public static final int ENOLINK = noInline();
    public static final int EADV = noInline();
    public static final int ESRMNT = noInline();
    public static final int ECOMM = noInline();
    public static final int EPROTO = noInline();
    public static final int EMULTIHOP = noInline();
    public static final int EDOTDOT = noInline();
    public static final int EBADMSG = noInline();
    public static final int EOVERFLOW = noInline();
    public static final int ENOTUNIQ = noInline();
    public static final int EBADFD = noInline();
    public static final int EREMCHG = noInline();
    public static final int ELIBACC = noInline();
    public static final int ELIBBAD = noInline();
    public static final int ELIBSCN = noInline();
    public static final int ELIBMAX = noInline();
    public static final int ELIBEXEC = noInline();
    public static final int EILSEQ = noInline();
    public static final int ERESTART = noInline();
    public static final int ESTRPIPE = noInline();
    public static final int EUSERS = noInline();
    public static final int ENOTSOCK = noInline();
    public static final int EDESTADDRREQ = noInline();
    public static final int EMSGSIZE = noInline();
    public static final int EPROTOTYPE = noInline();
    public static final int ENOPROTOOPT = noInline();
    public static final int EPROTONOSUPPORT = noInline();
    public static final int ESOCKTNOSUPPORT = noInline();
    public static final int EOPNOTSUPP = noInline();
    public static final int EPFNOSUPPORT = noInline();
    public static final int EAFNOSUPPORT = noInline();
    public static final int EADDRINUSE = noInline();
    public static final int EADDRNOTAVAIL = noInline();
    public static final int ENETDOWN = noInline();
    public static final int ENETUNREACH = noInline();
    public static final int ENETRESET = noInline();
    public static final int ECONNABORTED = noInline();
    public static final int ECONNRESET = noInline();
    public static final int ENOBUFS = noInline();
    public static final int EISCONN = noInline();
    public static final int ENOTCONN = noInline();
    public static final int ESHUTDOWN = noInline();
    public static final int ETOOMANYREFS = noInline();
    public static final int ETIMEDOUT = noInline();
    public static final int ECONNREFUSED = noInline();
    public static final int EHOSTDOWN = noInline();
    public static final int EHOSTUNREACH = noInline();
    public static final int EALREADY = noInline();
    public static final int EINPROGRESS = noInline();
    public static final int ESTALE = noInline();
    public static final int EUCLEAN = noInline();
    public static final int ENOTNAM = noInline();
    public static final int ENAVAIL = noInline();
    public static final int EISNAM = noInline();
    public static final int EREMOTEIO = noInline();
    public static final int EDQUOT = noInline();
    public static final int ENOMEDIUM = noInline();
    public static final int EMEDIUMTYPE = noInline();
    public static final int ECANCELED = noInline();
    public static final int ENOKEY = noInline();
    public static final int EKEYEXPIRED = noInline();
    public static final int EKEYREVOKED = noInline();
    public static final int EKEYREJECTED = noInline();
    public static final int EOWNERDEAD = noInline();
    public static final int ENOTRECOVERABLE = noInline();
    // END: errno.h

    // BEGIN: linux/stat.h
    public static final int S_IFMT = noInline();
    public static final int S_IFSOCK = noInline();
    public static final int S_IFLNK = noInline();
    public static final int S_IFREG = noInline();
    public static final int S_IFBLK = noInline();
    public static final int S_IFDIR = noInline();
    public static final int S_IFCHR = noInline();
    public static final int S_IFIFO = noInline();
    public static final int S_ISUID = noInline();
    public static final int S_ISGID = noInline();
    public static final int S_ISVTX = noInline();
    public static final int S_IRWXU = noInline();
    public static final int S_IRUSR = noInline();
    public static final int S_IWUSR = noInline();
    public static final int S_IXUSR = noInline();
    public static final int S_IRWXG = noInline();
    public static final int S_IRGRP = noInline();
    public static final int S_IWGRP = noInline();
    public static final int S_IXGRP = noInline();
    public static final int S_IRWXO = noInline();
    public static final int S_IROTH = noInline();
    public static final int S_IWOTH = noInline();
    public static final int S_IXOTH = noInline();
    // END: linux/stat.h

    // BEGIN: sys/statvfs.h
    public static final int ST_RDONLY = noInline();
    public static final int ST_NOSUID = noInline();
    public static final int ST_NODEV = noInline();
    public static final int ST_NOEXEC = noInline();
    public static final int ST_SYNCHRONOUS = noInline();
    public static final int ST_MANDLOCK = noInline();
    public static final int ST_NOATIME = noInline();
    public static final int ST_NODIRATIME = noInline();
    public static final int ST_RELATIME = noInline();
    // END: sys/statvfs.h

    // BEGIN: unistd.h
    public static final int STDIN_FILENO = noInline();
    public static final int STDOUT_FILENO = noInline();
    public static final int STDERR_FILENO = noInline();
    // END: unistd.h

    static {
        System.loadLibrary("miscstuff-jni");
        initConstants();
    }
}