/*
 * User-supplied callbacks and default implementations.
 * Class and permission mappings.
 */

#include <stdarg.h>
#include <stdio.h>

#include "callbacks.h"

/* default implementations */
static int __attribute__ ((format(printf, 1, 2)))
default_selinux_log(const char *fmt, ...)
{
    int rc;
    va_list ap;
    va_start(ap, fmt);
    rc = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return rc;
}

/* callback pointers */
int __attribute__ ((format(printf, 1, 2)))
(*selinux_log)(const char *, ...) = default_selinux_log;
