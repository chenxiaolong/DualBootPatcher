/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
 * Copyright (C) 2010 The Android Open Source Project
 *
 * This file is part of DualBootPatcher
 *
 * DualBootPatcher is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DualBootPatcher is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DualBootPatcher.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "mbcommon/common.h"
#include "mbsparse/sparse_header.h"

#ifdef __cplusplus
#include <cstdio>
#else
#include <stdbool.h>
#include <stdio.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef bool (*SparseOpenCb)(void *userData);
typedef bool (*SparseCloseCb)(void *userData);
typedef bool (*SparseReadCb)(void *buf, uint64_t size, uint64_t *bytesRead,
                             void *userData);
typedef bool (*SparseSeekCb)(int64_t offset, int whence, void *userData);
typedef bool (*SparseSkipCb)(uint64_t offset, void *userData);

struct SparseCtx;

MB_EXPORT struct SparseCtx * sparseCtxNew();
MB_EXPORT bool sparseCtxFree(struct SparseCtx *ctx);

MB_EXPORT bool sparseOpen(struct SparseCtx *ctx, SparseOpenCb openCb,
                          SparseCloseCb closeCb, SparseReadCb readCb,
                          SparseSeekCb seekCb, SparseSkipCb skipCb,
                          void *userData);
MB_EXPORT bool sparseClose(struct SparseCtx *ctx);
MB_EXPORT bool sparseRead(struct SparseCtx *ctx, void *buf, uint64_t size,
                          uint64_t *bytesRead);
MB_EXPORT bool sparseSeek(struct SparseCtx *ctx, int64_t offset, int whence);
MB_EXPORT bool sparseTell(struct SparseCtx *ctx, uint64_t *offset);
MB_EXPORT bool sparseSize(struct SparseCtx *ctx, uint64_t *size);

#ifdef __cplusplus
}
#endif