/*
 * Copyright (C) 2015-2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
 * Copyright (C) 2010 The Android Open Source Project
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

#include "../libmbp_global.h"
#include "sparse_header.h"

#include <cstdio>

/*
 * Following a Raw or Fill or CRC32 chunk is data.
 * - For a Raw chunk, it's the data in chunk_sz * blk_sz.
 * - For a Fill chunk, it's 4 bytes of the fill data.
 * - For a CRC32 chunk, it's 4 bytes of CRC32
 */

typedef bool (*SparseOpenCb)(void *userData);
typedef bool (*SparseCloseCb)(void *userData);
typedef bool (*SparseReadCb)(void *buf, uint64_t size, uint64_t *bytesRead,
                             void *userData);
typedef bool (*SparseSeekCb)(int64_t offset, int whence, void *userData);
typedef bool (*SparseSkipCb)(uint64_t offset, void *userData);

struct SparseCtx;

MBP_EXPORT SparseCtx * sparseCtxNew();
MBP_EXPORT bool sparseCtxFree(SparseCtx *ctx);

MBP_EXPORT bool sparseOpen(SparseCtx *ctx, SparseOpenCb openCb,
                           SparseCloseCb closeCb, SparseReadCb readCb,
                           SparseSeekCb seekCb, SparseSkipCb skipCb,
                           void *userData);
MBP_EXPORT bool sparseClose(SparseCtx *ctx);
MBP_EXPORT bool sparseRead(SparseCtx *ctx, void *buf, uint64_t size,
                           uint64_t *bytesRead);
MBP_EXPORT bool sparseSeek(SparseCtx *ctx, int64_t offset, int whence);
MBP_EXPORT bool sparseTell(SparseCtx *ctx, uint64_t *offset);
MBP_EXPORT bool sparseSize(SparseCtx *ctx, uint64_t *size);