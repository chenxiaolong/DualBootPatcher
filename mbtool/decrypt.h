/*
 * Copyright (C) 2016  Andrew Gunnerson <andrewgunnerson@gmail.com>
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

#define CRYPTFS_PW_TYPE_DEFAULT         "default"
#define CRYPTFS_PW_TYPE_PASSWORD        "password"
#define CRYPTFS_PW_TYPE_PATTERN         "pattern"
#define CRYPTFS_PW_TYPE_PIN             "pin"

namespace mb
{

bool decrypt_init();
bool decrypt_cleanup();
const char * decrypt_get_pw_type();
bool decrypt_userdata(const char *password);

int decrypt_main(int argc, char *argv[]);

}
