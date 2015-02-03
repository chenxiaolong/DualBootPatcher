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

int64_t mb_socket_read(int fd, void *buf, int64_t size);
int64_t mb_socket_write(int fd, void *buf, int64_t size);
int mb_socket_read_uint16(int fd, uint16_t *result);
int mb_socket_write_uint16(int fd, uint16_t n);
int mb_socket_read_uint32(int fd, uint32_t *result);
int mb_socket_write_uint32(int fd, uint32_t n);
int mb_socket_read_uint64(int fd, uint64_t *result);
int mb_socket_write_uint64(int fd, uint64_t n);
int mb_socket_read_int16(int fd, int16_t *result);
int mb_socket_write_int16(int fd, int16_t n);
int mb_socket_read_int32(int fd, int32_t *result);
int mb_socket_write_int32(int fd, int32_t n);
int mb_socket_read_int64(int fd, int64_t *result);
int mb_socket_write_int64(int fd, int64_t n);
int mb_socket_read_string(int fd, char **result);
int mb_socket_write_string(int fd, char *str);
