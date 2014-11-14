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

#ifndef C_STRINGMAP_H
#define C_STRINGMAP_H

#ifdef __cplusplus
extern "C" {
#endif

struct CStringMap;
typedef struct CStringMap CStringMap;

CStringMap * mbp_stringmap_create();
void mbp_stringmap_destroy(CStringMap *map);

char ** mbp_stringmap_keys(const CStringMap *map);
char * mbp_stringmap_get(const CStringMap *map, const char *key);
void mbp_stringmap_set(CStringMap *map, const char *key, const char *value);
void mbp_stringmap_remove(CStringMap *map, const char *key);

#ifdef __cplusplus
}
#endif

#endif // C_STRINGMAP_H
