/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include "libmbpio/private/filebase.h"

#include <memory>

namespace io
{
namespace posix
{

class FilePosix : public priv::FileBase
{
public:
    FilePosix();
    virtual ~FilePosix();

    virtual bool open(const char *filename, int mode) override;
    virtual bool open(const std::string &filename, int mode) override;
    virtual bool close() override;
    virtual bool isOpen() override;
    virtual bool read(void *buf, uint64_t size, uint64_t *bytesRead) override;
    virtual bool write(const void *buf, uint64_t size, uint64_t *bytesWritten) override;
    virtual bool tell(uint64_t *pos) override;
    virtual bool seek(int64_t offset, int origin) override;
    virtual bool truncate(uint64_t size) override;
    virtual int error() override;

protected:
    virtual std::string platformErrorString() override;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

}
}
