# Copyright (C) 2014  Xiao-Long Chen <chenxiaolong@cxl.epac.to>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import multiboot.autopatchers as autopatchers
import multiboot.config as config
import multiboot.operatingsystem as OS

import imp
import os

always_include = ['Google_Apps', 'Other']
patchinfos = dict()
ramdisks = dict()


def load_patchinfo_dir(pathname):
    modules = list()

    for root, dirs, files in os.walk(os.path.join(OS.patchinfodir, pathname)):
        for f in files:
            if os.path.splitext(f)[1] == '.py':
                fullpath = os.path.join(root, f)
                relpath = os.path.relpath(fullpath, OS.patchinfodir)
                name = relpath.replace('/', '_')

                plugin = imp.load_source(name, fullpath)

                modules.append((relpath, plugin.patchinfo))

    modules.sort()
    return modules


def load_patchinfos():
    global patchinfos

    searchpaths = always_include[:]
    devices = config.list_devices()
    for d in devices:
        if os.path.isdir(os.path.join(OS.patchinfodir, d)):
            searchpaths.append(d)

    for path in searchpaths:
        patchinfos[path] = load_patchinfo_dir(path)


def load_ramdisks():
    global ramdisks

    searchpaths = []
    devices = config.list_devices()
    for d in devices:
        if os.path.isdir(os.path.join(OS.ramdiskdir, d)):
            searchpaths.append(d)

    for path in searchpaths:
        for root, dirs, files in os.walk(os.path.join(OS.ramdiskdir, path)):
            for f in files:
                if os.path.splitext(f)[1] == '.py':
                    fullpath = os.path.join(root, f)
                    relpath = os.path.relpath(fullpath, OS.ramdiskdir)
                    name = relpath.replace('/', '_')

                    plugin = imp.load_source(name, fullpath)

                    ramdisks[relpath.replace('\\', '/')] = plugin


def init():
    load_patchinfos()
    load_ramdisks()