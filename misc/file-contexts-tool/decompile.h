#pragma once

#include "pcre_shim.h"

#ifdef __cplusplus
extern "C" {
#endif

int decompile(struct pcre_shim *shim,
              const char *source_file, const char *target_file);

#ifdef __cplusplus
}
#endif
