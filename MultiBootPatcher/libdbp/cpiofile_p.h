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

#ifndef CPIOFILE_P_H
#define CPIOFILE_P_H

#include "cpiofile.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>

// libarchive
#include <archive_entry.h>

class CpioFilePrivate
{
public:
    ~CpioFilePrivate();

    QList<QPair<archive_entry *, QByteArray>> files;

    PatcherError::Error errorCode;
    QString errorString;
};

#endif // CPIOFILE_P_H
