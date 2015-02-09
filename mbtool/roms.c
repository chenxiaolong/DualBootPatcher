/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This file is part of MultiBootPatcher
 *
 * MultiBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * MultiBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with MultiBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "roms.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "util/logging.h"
#include "util/path.h"

#define SYSTEM "/system"
#define CACHE "/cache"
#define DATA "/data"

#define BUILD_PROP "build.prop"


static int roms_add(struct roms *roms, struct rom *rom)
{
    VECTOR_PUSH_BACK(roms, rom, error);
    return 0;

error:
    return -1;
}

static int roms_remove(struct roms *roms, struct rom *rom)
{
    VECTOR_POP_VALUE(roms, rom, error);
    return 0;

error:
    return -1;
}

static void free_rom(struct rom *rom)
{
    if (!rom) {
        return;
    }

    // free(NULL) is valid
    free(rom->id);
    free(rom->system_path);
    free(rom->cache_path);
    free(rom->data_path);

    mb_packages_cleanup(rom->pkgs);
}

int mb_roms_get_builtin(struct roms *roms)
{
    // Primary
    struct rom *r = calloc(sizeof(struct rom), 1);
    if (!r) {
        return -1;
    }

    r->id = strdup("primary");
    r->system_path = strdup(SYSTEM);
    r->cache_path = strdup(CACHE);
    r->data_path = strdup(DATA);

    roms_add(roms, r);

    // Secondary
    r = calloc(sizeof(struct rom), 1);
    if (!r) {
        return -1;
    }

    r->id = strdup("dual");
    r->system_path = mb_path_build(SYSTEM, "multiboot", "dual", "system", NULL);
    r->cache_path = mb_path_build(CACHE, "multiboot", "dual", "cache", NULL);
    r->data_path = mb_path_build(DATA, "multiboot", "dual", "data", NULL);

    roms_add(roms, r);

    // Multislots
    char id[20] = { 0 };
    for (int i = 1; i <= 3; ++i) {
        snprintf(id, 20, "multi-slot-%d", i);

        r = calloc(sizeof(struct rom), 1);
        if (!r) {
            return -1;
        }

        r->id = strdup(id);
        r->system_path = mb_path_build(CACHE, "multiboot", id, "system", NULL);
        r->cache_path = mb_path_build(SYSTEM, "multiboot", id, "cache", NULL);
        r->data_path = mb_path_build(DATA, "multiboot", id, "data", NULL);

        roms_add(roms, r);
    }

    return 0;
}

int mb_roms_get_installed(struct roms *roms)
{
    struct roms all_roms;
    mb_roms_init(&all_roms);
    mb_roms_get_builtin(&all_roms);

    struct stat sb;

    while (all_roms.len > 0) {
        struct rom *r = all_roms.list[0];

        char *raw_bp_path = mb_path_build("/raw", r->system_path, "build.prop", NULL);
        if (!raw_bp_path) {
            goto error;
        }

        char *bp_path = mb_path_build(r->system_path, "build.prop", NULL);
        if (!bp_path) {
            free(raw_bp_path);
            goto error;
        }

        roms_remove(&all_roms, r);

        if (stat(raw_bp_path, &sb) == 0 && S_ISREG(sb.st_mode)) {
            r->use_raw_paths = 1;
            roms_add(roms, r);
        } else if (stat(bp_path, &sb) == 0 && S_ISREG(sb.st_mode)) {
            r->use_raw_paths = 0;
            roms_add(roms, r);
        } else {
            free_rom(r);
        }

        free(raw_bp_path);
        free(bp_path);
    }

    mb_roms_cleanup(&all_roms);
    return 0;

error:
    mb_roms_cleanup(&all_roms);
    return -1;
}

struct rom * mb_find_rom_by_id(struct roms *roms, const char *id)
{
    for (size_t i = 0; i < roms->len; ++i) {
        struct rom *r = roms->list[i];
        if (strcmp(r->id, id) == 0) {
            return r;
        }
    }

    return NULL;
}

int mb_roms_init(struct roms *roms)
{
    roms->list = NULL;
    roms->len = 0;
    roms->capacity = 0;

    return 0;
}

int mb_roms_cleanup(struct roms *roms)
{
    if (!roms) {
        return 0;
    }

    for (size_t i = 0; i < roms->len; ++i) {
        struct rom *r = roms->list[i];
        free_rom(r);
        free(r);
    }

    free(roms->list);
    roms->list = NULL;
    roms->len = 0;
    roms->capacity = 0;

    return 0;
}