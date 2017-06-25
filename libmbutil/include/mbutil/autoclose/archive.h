/*
 * Copyright (C) 2015  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#include <memory>

#include <archive.h>
#include <archive_entry.h>

namespace mb
{
namespace autoclose
{

typedef std::unique_ptr<::archive, int (*)(::archive *)> archive;
typedef std::unique_ptr<::archive_entry, void (*)(::archive_entry *)> archive_entry;
typedef std::unique_ptr<::archive_entry_linkresolver, void (*)(::archive_entry_linkresolver *)> archive_entry_linkresolver;

}
}