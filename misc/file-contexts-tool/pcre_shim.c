#include "pcre_shim.h"

#include <string.h>

#include <dlfcn.h>

#include "callbacks.h"

static int load_pcre1(struct pcre1_shim *p1, void *handle)
{
    // pcre_free() is actually a function pointer, not a function
    void **pcre_free = dlsym(handle, "pcre_free");
    if (!(p1->pcre_free = *pcre_free)) {
        return -1;
    }

    if (!(p1->pcre_compile = dlsym(handle, "pcre_compile"))) {
        return -1;
    }

    if (!(p1->pcre_fullinfo = dlsym(handle, "pcre_fullinfo"))) {
        return -1;
    }

    if (!(p1->pcre_study = dlsym(handle, "pcre_study"))) {
        return -1;
    }

    // pcre_free_study does not exist in every version of PCRE. Older versions
    // use just pcre_free()
    if (!(p1->pcre_free_study = dlsym(handle, "pcre_free_study"))) {
        selinux_log("pcre_free_study does not exist; using pcre_free\n");
        p1->pcre_free_study = *pcre_free;
    }

    if (!(p1->pcre_version = dlsym(handle, "pcre_version"))) {
        return -1;
    }

    return 0;
}

static int load_pcre2(struct pcre2_shim *p2, void *handle)
{
    if (!(p2->pcre2_config = dlsym(handle, "pcre2_config_8"))) {
        return -1;
    }

    if (!(p2->pcre2_compile = dlsym(handle, "pcre2_compile_8"))) {
        return -1;
    }

    if (!(p2->pcre2_code_free = dlsym(handle, "pcre2_code_free_8"))) {
        return -1;
    }

    if (!(p2->pcre2_match_data_create_from_pattern =
            dlsym(handle, "pcre2_match_data_create_from_pattern_8"))) {
        return -1;
    }

    if (!(p2->pcre2_match_data_free =
            dlsym(handle, "pcre2_match_data_free_8"))) {
        return -1;
    }

    if (!(p2->pcre2_serialize_encode =
            dlsym(handle, "pcre2_serialize_encode_8"))) {
        return -1;
    }

    if (!(p2->pcre2_serialize_free = dlsym(handle, "pcre2_serialize_free_8"))) {
        return -1;
    }

    if (!(p2->pcre2_get_error_message =
            dlsym(handle, "pcre2_get_error_message_8"))) {
        return -1;
    }

    return 0;
}

int pcre_shim_load(struct pcre_shim *shim, const char *path)
{
    memset(shim, 0, sizeof(*shim));

    shim->handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!shim->handle) {
        selinux_log("%s: Failed to dlopen: %s\n", path, dlerror());
        goto error;
    }

    // Perform simple check to determine if we're using PCRE1 or PCRE2
    if (dlsym(shim->handle, "pcre_version")) {
        shim->use_pcre2 = false;

        if (load_pcre1(&shim->p1, shim->handle) < 0) {
            goto error;
        }
    } else if (dlsym(shim->handle, "pcre2_config_8")) {
        shim->use_pcre2 = true;

        if (load_pcre2(&shim->p2, shim->handle) < 0) {
            goto error;
        }
    } else {
        selinux_log("%s: Library is neither PCRE nor PCRE2\n", path);
        goto error;
    }

    return 0;

error:
    if (shim->handle) {
        dlclose(shim->handle);
    }
    return -1;
}

int pcre_shim_unload(struct pcre_shim *shim)
{
    if (shim && shim->handle) {
        if (dlclose(shim->handle) < 0) {
            selinux_log("Failed to unload library: %s\n", dlerror());
            return -1;
        }
    }

    return 0;
}
