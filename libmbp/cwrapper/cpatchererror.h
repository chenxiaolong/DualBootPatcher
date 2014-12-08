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

#include "cwrapper/ctypes.h"
#include "errors.h"

#ifdef __cplusplus
using namespace MBP;

extern "C" {
#endif

void mbp_error_destroy(CPatcherError *error);

ErrorType mbp_error_error_type(const CPatcherError *error);
ErrorCode mbp_error_error_code(const CPatcherError *error);

char * mbp_error_patcher_name(const CPatcherError *error);
char * mbp_error_filename(const CPatcherError *error);

#ifdef __cplusplus
}
#endif
