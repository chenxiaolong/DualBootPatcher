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

#ifndef C_PATCHINFO_H
#define C_PATCHINFO_H

#ifdef __cplusplus
extern "C" {
#endif

const char * mbp_patchinfo_default();
const char * mbp_patchinfo_notmatched();

struct CPatchInfo;
typedef struct CPatchInfo CPatchInfo;

CPatchInfo * mbp_patchinfo_create();
void mbp_patchinfo_destroy(CPatchInfo *info);

char * mbp_patchinfo_id(const CPatchInfo *info);
void mbp_patchinfo_set_id(CPatchInfo *info, const char *id);

char * mbp_patchinfo_name(const CPatchInfo *info);
void mbp_patchinfo_set_name(CPatchInfo *info, const char *name);

char * mbp_patchinfo_key_from_filename(const CPatchInfo *info,
                                       const char *fileName);

char ** mbp_patchinfo_regexes(const CPatchInfo *info);
void mbp_patchinfo_set_regexes(CPatchInfo *info, const char **regexes);

char ** mbp_patchinfo_exclude_regexes(const CPatchInfo *info);
void mbp_patchinfo_set_exclude_regexes(CPatchInfo *info, const char **regexes);

char ** mbp_patchinfo_cond_regexes(const CPatchInfo *info);
void mbp_patchinfo_set_cond_regexes(CPatchInfo *info, const char **regexes);

int mbp_patchinfo_has_not_matched(const CPatchInfo *info);
void mbp_patchinfo_set_has_not_matched(CPatchInfo *info, int hasElem);

//AutoPatcherItems autoPatchers(const std::string &key) const;
//void setAutoPatchers(const std::string &key, AutoPatcherItems autoPatchers);

int mbp_patchinfo_has_boot_image(const CPatchInfo *info, const char *key);
void mbp_patchinfo_set_has_boot_image(CPatchInfo *info,
                                      const char *key, int hasBootImage);

int mbp_patchinfo_autodetect_boot_images(const CPatchInfo *info,
                                         const char *key);
void mbp_patchinfo_set_autodetect_boot_images(CPatchInfo *info,
                                              const char *key, int autoDetect);

char ** mbp_patchinfo_boot_images(const CPatchInfo *info, const char *key);
void mbp_patchinfo_set_boot_images(CPatchInfo *info,
                                   const char *key, const char **bootImages);

char * mbp_patchinfo_ramdisk(const CPatchInfo *info, const char *key);
void mbp_patchinfo_set_ramdisk(CPatchInfo *info,
                               const char *key, const char *ramdisk);

char * mbp_patchinfo_patched_init(const CPatchInfo *info, const char *key);
void mbp_patchinfo_set_patched_init(CPatchInfo *info,
                                    const char *key, const char *init);

int mbp_patchinfo_device_check(const CPatchInfo *info, const char *key);
void mbp_patchinfo_set_device_check(CPatchInfo *info,
                                    const char *key, int deviceCheck);

char ** mbp_patchinfo_supported_configs(const CPatchInfo *info,
                                        const char *key);
void mbp_patchinfo_set_supported_configs(CPatchInfo *info,
                                         const char *key, char ** configs);

#ifdef __cplusplus
}
#endif

#endif // C_FILEINFO_H
