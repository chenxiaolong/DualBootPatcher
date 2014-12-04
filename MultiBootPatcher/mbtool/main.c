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

#include <stdio.h>

#include "config.h"
#include "logging.h"
#include "mount_fstab.h"

int main(int argc, char *argv[])
{
    // TODO: This is just a quick hack. This will soon be a multi-call binary
    //       and a daemon.

    if (mainconfig_init() < 0) {
        KLOG_ERROR("Failed to load main configuration file");
        return 1;
    }

    mount_fstab_main(argc, argv);
}
