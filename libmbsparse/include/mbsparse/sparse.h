/*
 * Copyright (C) 2015-2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/file.h"

namespace mb
{
namespace sparse
{

class SparseFilePrivate;
class MB_EXPORT SparseFile : public File
{
    MB_DECLARE_PRIVATE(SparseFile)

public:
    SparseFile();
    SparseFile(File *file);
    virtual ~SparseFile();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(SparseFile)
    MB_DEFAULT_MOVE_CONSTRUCT_AND_ASSIGN(SparseFile)

    // File open
    FileStatus open(File *file);

    // File size
    uint64_t size();

protected:
    /*! \cond INTERNAL */
    SparseFile(SparseFilePrivate *priv);
    SparseFile(SparseFilePrivate *priv, File *file);
    /*! \endcond */

    virtual FileStatus on_open() override;
    virtual FileStatus on_close() override;
    virtual FileStatus on_read(void *buf, size_t size,
                               size_t *bytes_read) override;
    virtual FileStatus on_seek(int64_t offset, int whence,
                               uint64_t *new_offset) override;

private:
    std::unique_ptr<SparseFilePrivate> _priv_ptr;
};

}
}
