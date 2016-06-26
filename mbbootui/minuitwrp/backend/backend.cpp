/*
 * Copyright (C) 2016 Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "backend/backend.h"
#include "backend/backend.gen.h"

#include <cstring>

struct backend
{
    const char *name;
    backend_init_fn fn;
};

#define BACKEND(name) { #name, BACKEND_FUNCTION(name) }

static struct backend backends[] = {
#ifdef ENABLE_OVERLAY_MSM_OLD_BACKEND
    BACKEND(overlay_msm_old),
#endif
#ifdef ENABLE_ADF_BACKEND
    BACKEND(adf),
#endif
#ifdef ENABLE_DRM_BACKEND
    BACKEND(drm),
#endif
#ifdef ENABLE_FBDEV_BACKEND
    BACKEND(fbdev),
#endif
    { nullptr, nullptr }
};

extern "C" {

backend_init_fn get_backend(const char *name)
{
    backend_init_fn fn = nullptr;

    for (const struct backend *iter = backends; iter->name; ++iter) {
        if (strcmp(iter->name, name) == 0) {
            fn = iter->fn;
            break;
        }
    }

    return fn;
}

}
