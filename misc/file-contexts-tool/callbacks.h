/*
 * This file describes the callbacks passed to selinux_init() and available
 * for use from the library code.  They all have default implementations.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* callback pointers */
extern int __attribute__ ((format(printf, 1, 2)))
(*selinux_log)(const char *, ...);

#ifdef __cplusplus
}
#endif
