/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
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

struct partconfig {
    char *id;
    char *kernel_id;
    char *name;
    char *description;
    char *target_system;
    char *target_cache;
    char *target_data;
    char *target_system_partition;
    char *target_cache_partition;
    char *target_data_partition;
};

struct mainconfig {
    struct partconfig *partconfigs;
    int partconfigs_len;
    char *installed;
};

int mainconfig_init(void);
void mainconfig_cleanup(void);
struct mainconfig * get_mainconfig(void);
