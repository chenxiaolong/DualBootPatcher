/*
 * Copyright (C) 2014  Andrew Gunnerson <andrewgunnerson@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

package com.github.chenxiaolong.dualbootpatcher.switcher;

import android.os.Build;

import com.github.chenxiaolong.dualbootpatcher.CommandUtils;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.Device;
import com.github.chenxiaolong.multibootpatcher.nativelib.LibMbp.PatcherConfig;
import com.github.chenxiaolong.multibootpatcher.socket.MbtoolSocket;

public class SwitcherUtils {
    public static final String TAG = SwitcherUtils.class.getSimpleName();

    public static String getBootPartition() {
        String bootBlockDev = null;

        PatcherConfig pc = new PatcherConfig();
        for (Device d : pc.getDevices()) {
            boolean matches = false;

            for (String codename : d.getCodenames()) {
                if (Build.DEVICE.contains(codename)) {
                    matches = true;
                    break;
                }
            }

            if (matches) {
                String[] bootBlockDevs = d.getBootBlockDevs();
                if (bootBlockDevs.length > 0) {
                    bootBlockDev = bootBlockDevs[0];
                }
                break;
            }
        }
        pc.destroy();

        return bootBlockDev;
    }

    public static boolean chooseRom(String id) {
        return MbtoolSocket.getInstance().chooseRom(id);
    }

    public static boolean setKernel(String id) {
        return MbtoolSocket.getInstance().setKernel(id);
    }

    public static void reboot() {
        new Thread() {
            @Override
            public void run() {
                runReboot();
            }
        }.start();
    }

    private static int runReboot() {
        return CommandUtils.runRootCommand("reboot");
    }
}
