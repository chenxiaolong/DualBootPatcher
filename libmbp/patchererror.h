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

#include <memory>
#include <string>

#include "libmbp_global.h"
#include "errors.h"


namespace mbp
{

class MBP_EXPORT PatcherError
{
public:
    static PatcherError createGenericError(ErrorCode error);
    static PatcherError createPatcherCreationError(ErrorCode error, std::string name);
    static PatcherError createIOError(ErrorCode error, std::string filename);
    static PatcherError createBootImageError(ErrorCode error);
    static PatcherError createCpioError(ErrorCode error, std::string filename);
    static PatcherError createArchiveError(ErrorCode error, std::string filename);
    static PatcherError createXmlError(ErrorCode error, std::string filename);
    static PatcherError createSupportedFileError(ErrorCode error, std::string name);
    static PatcherError createCancelledError(ErrorCode error);
    static PatcherError createPatchingError(ErrorCode error);

    ErrorType errorType() const;
    ErrorCode errorCode() const;

    std::string patcherId() const;
    std::string filename() const;

    PatcherError();
    PatcherError(const PatcherError &error);
    PatcherError(PatcherError &&error);
    ~PatcherError();
    PatcherError & operator=(const PatcherError &other);
    PatcherError & operator=(PatcherError &&other);

    explicit operator bool() const;


private:
    PatcherError(ErrorType errorType, ErrorCode errorCode);

    class Impl;
    std::shared_ptr<Impl> m_impl;
};

}