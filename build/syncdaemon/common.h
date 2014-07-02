/*
 * Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COMMON_H
#define COMMON_H

#include <string>

static const std::string BUILD_PROP = "build.prop";

static const std::string APP_DIR = "app.bak";
static const std::string APP_LIB_DIR = "app-lib.bak";
static const std::string DEX_CACHE_DIR = "dalvik-cache";
static const std::string DEX_CACHE_PREFIX = "data@app@";

static const std::string SYSTEM = "/system";
static const std::string CACHE = "/cache";
static const std::string DATA = "/data";
static const std::string RAW_SYSTEM = "/raw-system";
static const std::string RAW_CACHE = "/raw-cache";
static const std::string RAW_DATA = "/raw-data";

static const std::string PRIMARY_ID = "primary";
static const std::string SECONDARY_ID = "dual";
static const std::string MULTI_ID_PREFIX = "multi-slot-";

static const std::string SEP = "/";

// Functions
bool exists_directory(std::string path);
bool exists_file(std::string path);
bool is_same_inode(std::string path1, std::string path2);
int recursively_delete(std::string directory);
std::string dirname2(std::string path);
std::string basename2(std::string path);
std::string search_directory(std::string directory, std::string begin);

#endif //COMMON_H