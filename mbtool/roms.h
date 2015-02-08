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

#pragma once

#include <stddef.h>

#include "packages.h"
#include "util/vector.h"

struct rom {
    char *id;
    char *system_path;
    char *cache_path;
    char *data_path;
    int use_raw_paths;
    struct packages *pkgs;
};

VECTOR(struct rom *, roms);

int mb_roms_init(struct roms *roms);
int mb_roms_cleanup(struct roms *roms);
int mb_roms_get_builtin(struct roms *roms);
int mb_roms_get_installed(struct roms *roms);
struct rom * mb_find_rom_by_id(struct roms *roms, const char *id);
int mb_rom_load_packages(struct rom *rom);
int mb_rom_cleanup_packages(struct rom *rom);