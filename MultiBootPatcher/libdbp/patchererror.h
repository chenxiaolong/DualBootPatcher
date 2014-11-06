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

#ifndef PATCHERERROR_H
#define PATCHERERROR_H

#include <memory>

#include "libdbp_global.h"
#include "errors.h"


class LIBDBPSHARED_EXPORT PatcherError
{
public:
    static PatcherError createGenericError(MBP::ErrorCode error);
    static PatcherError createPatcherCreationError(MBP::ErrorCode error, std::string name);
    static PatcherError createIOError(MBP::ErrorCode error, std::string filename);
    static PatcherError createBootImageError(MBP::ErrorCode error);
    static PatcherError createCpioError(MBP::ErrorCode error, std::string filename);
    static PatcherError createArchiveError(MBP::ErrorCode error, std::string filename);
    static PatcherError createXmlError(MBP::ErrorCode error, std::string filename);
    static PatcherError createSupportedFileError(MBP::ErrorCode error, std::string name);
    static PatcherError createCancelledError(MBP::ErrorCode error);
    static PatcherError createPatchingError(MBP::ErrorCode error);

    MBP::ErrorType errorType();
    MBP::ErrorCode errorCode();

    std::string patcherName();
    std::string filename();

    PatcherError();
    PatcherError(const PatcherError &error);
    PatcherError(PatcherError &&error);
    ~PatcherError();
    PatcherError & operator=(const PatcherError &other);
    PatcherError & operator=(PatcherError &&other);


private:
    PatcherError(MBP::ErrorType errorType, MBP::ErrorCode errorCode);

    class Impl;
    std::shared_ptr<Impl> m_impl;
};

#endif // PATCHERERROR_H
