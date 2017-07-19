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

#include <string>

#include "mbbootimg/reader.h"
#include "mbbootimg/writer.h"

namespace mb
{

bool bi_copy_data_to_fd(MbBiReader *bir, int fd);
bool bi_copy_file_to_data(const std::string &path, MbBiWriter *biw);
bool bi_copy_data_to_file(MbBiReader *bir, const std::string &path);
bool bi_copy_data_to_data(MbBiReader *bir, MbBiWriter *biw);

}
