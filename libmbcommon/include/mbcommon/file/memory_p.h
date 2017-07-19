/*
 * Copyright (C) 2017  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "mbcommon/guard_p.h"

#include "mbcommon/file/memory.h"
#include "mbcommon/file_p.h"

/*! \cond INTERNAL */
namespace mb
{

class MemoryFilePrivate : public FilePrivate
{
public:
    MemoryFilePrivate();
    virtual ~MemoryFilePrivate();

    MB_DISABLE_COPY_CONSTRUCT_AND_ASSIGN(MemoryFilePrivate)

    void clear();

    void *data;
    size_t size;

    void **data_ptr;
    size_t *size_ptr;

    size_t pos;

    bool fixed_size;
};

}
/*! \endcond */
