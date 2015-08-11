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

#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>

#include "cwrapper/ctypes.h"

#ifdef __cplusplus
extern "C" {
#endif

enum BootImageType
{
    Android = 1,
    Loki = 2,
    Bump = 3,
    SonyElf = 4
};

CBootImage * mbp_bootimage_create(void);
void mbp_bootimage_destroy(CBootImage *bootImage);

/* enum ErrorCode */ int mbp_bootimage_error(const CBootImage *bootImage);

bool mbp_bootimage_load_data(CBootImage *bootImage,
                             const unsigned char *data, size_t size);
bool mbp_bootimage_load_file(CBootImage *bootImage,
                             const char *filename);

bool mbp_bootimage_create_data(const CBootImage *bootImage,
                               unsigned char **data, size_t *size);
bool mbp_bootimage_create_file(CBootImage *bootImage,
                               const char *filename);

enum BootImageType mbp_bootimage_was_type(const CBootImage *bootImage);
enum BootImageType mbp_bootimage_target_type(const CBootImage *bootImage);
void mbp_bootimage_set_target_type(CBootImage *bootImage, enum BootImageType type);

const char * mbp_bootimage_boardname(const CBootImage *bootImage);
void mbp_bootimage_set_boardname(CBootImage *bootImage,
                                 const char *name);

const char * mbp_bootimage_kernel_cmdline(const CBootImage *bootImage);
void mbp_bootimage_set_kernel_cmdline(CBootImage *bootImage,
                                      const char *cmdline);

uint32_t mbp_bootimage_page_size(const CBootImage *bootImage);
void mbp_bootimage_set_page_size(CBootImage *bootImage,
                                 uint32_t pageSize);

uint32_t mbp_bootimage_kernel_address(const CBootImage *bootImage);
void mbp_bootimage_set_kernel_address(CBootImage *bootImage,
                                      uint32_t address);

uint32_t mbp_bootimage_ramdisk_address(const CBootImage *bootImage);
void mbp_bootimage_set_ramdisk_address(CBootImage *bootImage,
                                       uint32_t address);

uint32_t mbp_bootimage_second_bootloader_address(const CBootImage *bootImage);
void mbp_bootimage_set_second_bootloader_address(CBootImage *bootImage,
                                                 uint32_t address);

uint32_t mbp_bootimage_kernel_tags_address(const CBootImage *bootImage);
void mbp_bootimage_set_kernel_tags_address(CBootImage *bootImage,
                                           uint32_t address);

uint32_t mbp_bootimage_ipl_address(const CBootImage *bootImage);
void mbp_bootimage_set_ipl_address(CBootImage *bootImage,
                                   uint32_t address);

uint32_t mbp_bootimage_rpm_address(const CBootImage *bootImage);
void mbp_bootimage_set_rpm_address(CBootImage *bootImage,
                                   uint32_t address);

uint32_t mbp_bootimage_appsbl_address(const CBootImage *bootImage);
void mbp_bootimage_set_appsbl_address(CBootImage *bootImage,
                                      uint32_t address);

uint32_t mbp_bootimage_entrypoint_address(const CBootImage *bootImage);
void mbp_bootimage_set_entrypoint_address(CBootImage *bootImage,
                                          uint32_t address);

void mbp_bootimage_kernel_image(const CBootImage *bootImage,
                                const unsigned char **data, size_t *size);
void mbp_bootimage_set_kernel_image(CBootImage *bootImage,
                                    const unsigned char *data, size_t size);

void mbp_bootimage_ramdisk_image(const CBootImage *bootImage,
                                 const unsigned char **data, size_t *size);
void mbp_bootimage_set_ramdisk_image(CBootImage *bootImage,
                                     const unsigned char *data, size_t size);

void mbp_bootimage_second_bootloader_image(const CBootImage *bootImage,
                                           const unsigned char **data, size_t *size);
void mbp_bootimage_set_second_bootloader_image(CBootImage *bootImage,
                                               const unsigned char *data, size_t size);

void mbp_bootimage_device_tree_image(const CBootImage *bootImage,
                                     const unsigned char **data, size_t *size);
void mbp_bootimage_set_device_tree_image(CBootImage *bootImage,
                                         const unsigned char *data, size_t size);

void mbp_bootimage_aboot_image(const CBootImage *bootImage,
                               const unsigned char **data, size_t *size);
void mbp_bootimage_set_aboot_image(CBootImage *bootImage,
                                   const unsigned char *data, size_t size);

void mbp_bootimage_ipl_image(const CBootImage *bootImage,
                             const unsigned char **data, size_t *size);
void mbp_bootimage_set_ipl_image(CBootImage *bootImage,
                                 const unsigned char *data, size_t size);

void mbp_bootimage_rpm_image(const CBootImage *bootImage,
                             const unsigned char **data, size_t *size);
void mbp_bootimage_set_rpm_image(CBootImage *bootImage,
                                 const unsigned char *data, size_t size);

void mbp_bootimage_appsbl_image(const CBootImage *bootImage,
                                const unsigned char **data, size_t *size);
void mbp_bootimage_set_appsbl_image(CBootImage *bootImage,
                                   const unsigned char *data, size_t size);

void mbp_bootimage_sin_image(const CBootImage *bootImage,
                             const unsigned char **data, size_t *size);
void mbp_bootimage_set_sin_image(CBootImage *bootImage,
                                 const unsigned char *data, size_t size);

void mbp_bootimage_sin_header(const CBootImage *bootImage,
                              const unsigned char **data, size_t *size);
void mbp_bootimage_set_sin_header(CBootImage *bootImage,
                                  const unsigned char *data, size_t size);

bool mbp_bootimage_equals(CBootImage *lhs, CBootImage *rhs);

#ifdef __cplusplus
}
#endif
