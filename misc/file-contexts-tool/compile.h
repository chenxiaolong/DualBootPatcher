#pragma once

#include "pcre_shim.h"

#ifdef __cplusplus
extern "C" {
#endif

int compile(struct pcre_shim *shim,
            const char *source_file, const char *target_file);

#ifdef __cplusplus
}
#endif
