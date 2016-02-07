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

#include <string>
#include <vector>

namespace mb
{
namespace util
{

typedef void (*OutputCb) (const std::string &line, void *data);

int run_shell_command(const std::string &command);
int run_command(const std::vector<std::string> &argv);
int run_command_cb(const std::vector<std::string> &argv,
                   OutputCb cb, void *data);
int run_command_chroot(const std::string &dir,
                       const std::vector<std::string> &argv);
int run_command_chroot_cb(const std::string &dir,
                          const std::vector<std::string> &argv,
                          OutputCb cb, void *data);
int run_command2(const std::vector<std::string> &argv,
                 const std::string &chroot_dir,
                 OutputCb cb, void *data);

}
}