#pragma once

#ifdef __cplusplus
#  include <cstddef>
#else
#  include <stddef.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

void * musl_memmem(const void *haystack, size_t haystacklen,
                   const void *needle, size_t needlelen);

#ifdef __cplusplus
}
#endif
