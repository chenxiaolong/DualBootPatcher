#pragma once

#ifdef __cplusplus
#  include <cstdio>
#else
#  include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

ssize_t gnulib_getdelim(char **__restrict lineptr, size_t *__restrict n,
                        int delim, FILE *__restrict stream);

#ifdef __cplusplus
}
#endif
