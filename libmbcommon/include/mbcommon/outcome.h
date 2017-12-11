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

#pragma GCC diagnostic push

#ifdef __clang__
#  pragma GCC diagnostic ignored "-Wc++1z-extensions"
#  pragma GCC diagnostic ignored "-Wdocumentation"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include "outcome.hpp"

#pragma GCC diagnostic pop


namespace mb
{

namespace oc = OUTCOME_V2_NAMESPACE;

}
