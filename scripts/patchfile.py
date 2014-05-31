#!/usr/bin/env python3

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

# Python 2 compatibility
from __future__ import print_function

import argparse
import os
import re
import shutil
import subprocess
import sys
import textwrap

sys.dont_write_bytecode = True

try:
    import multiboot.operatingsystem as OS
except Exception as e:
    print(str(e))
    sys.exit(1)

import multiboot.autopatcher as autopatcher
import multiboot.config as config
import multiboot.fileinfo as fileinfo
import multiboot.partitionconfigs as partitionconfigs
import multiboot.patcher as patcher
import multiboot.patchinfo as patchinfo
import multiboot.ramdisk as ramdisk


ui = OS.ui

# Valid: 'testsupported', 'patchfile', 'automated'
action = 'patchfile'

filename = None
device = None
partconfig_name = None
partconfig = None
noquestions = False

# For unsupported files
f_preset_name = None
f_hasbootimage = None
f_bootimage = None
f_ramdisk = None
f_autopatcher_name = None
f_patch = None
f_init = None
f_devicecheck = None

# Lists
devices = config.list_devices()
partition_configs = partitionconfigs.get()
presets = None
autopatchers = None
inits = None
ramdisks = None


class SimpleListDialog:
    def __init__(self):
        self.desc = None
        self.choicemsg = None
        self.names = list()
        self.values = list()
        self.invalidnames = list()
        self.invalidvalues = list()
        self.hasvalues = True
        self.allownochoice = True
        self._counter = 0
        self._maxname = 0

    def add_option(self, name, value=None, invalid=False):
        if len(name) > self._maxname:
            self._maxname = len(name)

        if invalid:
            self.invalidnames.append(name)
        else:
            self.names.append(name)

        if self.hasvalues:
            if invalid:
                self.invalidvalues.append(value)
            else:
                self.values.append(value)

        if not invalid:
            self._counter += 1

    def show(self):
        clear_screen()

        if self.desc:
            ui.info(self.desc)
            ui.info('')

        for i in range(0, self._counter):
            n = self.names[i]

            if self.hasvalues:
                v = self.values[i]
                spaces = (' ' * (self._maxname - len(n)))
                ui.info(str(i + 1) + '. ' + n + spaces + ' : ' + v)
            else:
                ui.info(str(i + 1) + '. ' + n)

        for i in range(0, len(self.invalidnames)):
            n = self.invalidnames[i]

            if self.hasvalues:
                v = self.invalidvalues[i]
                spaces = (' ' * (self._maxname - len(n)))
                ui.info('*. ' + n + spaces + ' : ' + v)
            else:
                ui.info('*. ' + n)

        ui.info('')

        if self.choicemsg:
            ui.info(self.choicemsg)
            ui.info('')

    def get_choice(self):
        while True:
            self.show()
            choice = input('Choice: ')

            if not choice and self.allownochoice:
                return 0

            try:
                choice = int(choice)
            except ValueError:
                continue

            if choice <= 0 or choice > self._counter:
                continue

            return choice


def clear_screen():
    if os.name == 'nt':
        ret = subprocess.call('cls', shell=True)
    else:
        sys.stderr.write("\x1b[2J\x1b[H")
        sys.stderr.flush()


def parse_args():
    global action, filename, device, partconfig_name, noquestions
    global f_preset_name, f_hasbootimage, f_bootimage, f_ramdisk
    global f_autopatcher_name, f_patch, f_init, f_devicecheck

    parser = argparse.ArgumentParser()
    parser.formatter_class = argparse.RawDescriptionHelpFormatter
    parser.description = textwrap.dedent('''
    Dual Boot Patcher
    -----------------
    ''')

    parser.add_argument('file',
                        help='The file to patch',
                        action='store')

    parser.add_argument('-d', '--device',
                        help='Device the file is being patched for',
                        action='store')
    parser.add_argument('-p', '--partconfig',
                        help='Partition configuration to use in the patched file',
                        action='store')

    parser.add_argument('--noquestions',
                        help=textwrap.dedent('''
                            Do not ask for device or partition configuration.
                            The patcher will fail and exit if the file is
                            unsupported
                        '''),
                        action='store_true')

    group = parser.add_argument_group('Unsupported files',
                                      description=textwrap.dedent('''
    The parameters below are for patching unsupported files. If they are used,
    the '--unsupported' parameter must be passed. If none of the parameters
    below are specified and a file is unsupported, the patcher will prompt you
    to select these options. Generally, you won't need to touch these unless
    you want the patching process to be automated.
    '''))

    supported_group = group.add_mutually_exclusive_group()
    supported_group.add_argument('--is-supported',
                                 help='Test if a file is supported',
                                 action='store_true')
    supported_group.add_argument('--unsupported',
                                 help='Must be passed if file is unsupported',
                                 action='store_true')

    group.add_argument('--preset',
                       help='Use one of the preset patcher presets',
                       action='store')

    bootimage_group = group.add_mutually_exclusive_group()
    bootimage_group.add_argument('--hasbootimage',
                                 help='File contains a boot image (default)',
                                 action='store_true')
    bootimage_group.add_argument('--nobootimage',
                                 help='File does not contain a boot image',
                                 action='store_false',
                                 dest='hasbootimage')

    group.add_argument('--bootimage',
                       help=textwrap.dedent('''
                           Path to boot image in zip. If there are multiple
                           boot images, separate them with commas.
                           (Default: boot.img)
                       '''),
                       action='store')

    group.add_argument('--ramdisk',
                       help='The type of ramdisk in the boot image(s)',
                       action='store')

    autopatch_group = group.add_mutually_exclusive_group()
    autopatch_group.add_argument('--autopatcher',
                                 help='Use an autopatcher (default)',
                                 action='store')
    autopatch_group.add_argument('--patch',
                                 help='Use a patch/diff file instead of an autopatcher',
                                 action='store')

    group.add_argument('--patchedinit',
                       help='Use a patched/prebuilt init binary',
                       action='store')

    group.add_argument('--nodevicecheck',
                       help=textwrap.dedent('''
                           Attempt to remove device model checks. Many AOSP-
                           based ROMs, for example, have assert() statements
                           to cancel installation if the model is incorrect.
                           This will attempt to remove those statements.
                       '''),
                       action='store_false',
                       dest='devicecheck')

    parser.set_defaults(hasbootimage=True, bootimage='boot.img')
    args = parser.parse_args()

    # Validate arguments
    if args.unsupported:
        if not args.device:
            raise Exception('The --device parameter must be passed')

        if not args.partconfig:
            raise Exception('The --partconfig parameter must be passed')

        if not args.preset:
            if args.hasbootimage:
                if not args.ramdisk:
                    raise Exception('The --ramdisk parameter must be passed')

            if not args.autopatcher and not args.patch:
                raise Exception('The --autopatcher or --patch parameters must be passed')

        f_preset_name = args.preset
        f_hasbootimage = args.hasbootimage
        f_bootimage = args.bootimage

        f_ramdisk = args.ramdisk
        if f_ramdisk and not f_ramdisk.endswith('.def'):
            f_ramdisk += '.def'

        f_autopatcher_name = args.autopatcher
        f_patch = args.patch
        f_init = args.patchedinit
        f_devicecheck = args.devicecheck


    filename = args.file
    device = args.device
    partconfig_name = args.partconfig

    noquestions = args.noquestions

    if args.is_supported:
        action='testsupported'
    elif args.unsupported:
        action='automated'
    else:
        action='patchfile'


def check_parameters():
    global filename, partconfig
    global partition_configs, presets, autopatchers, inits, ramdisks

    if not os.path.exists(filename):
        raise Exception('File %s does not exist' % filename)
        sys.exit(1)

    filename = os.path.abspath(filename)

    # Make sure the specified parameters are supported

    for c in partition_configs:
        if c.id == partconfig_name:
            partconfig = c

    if not partconfig:
        raise Exception('Partition configuration %s is not supported' %
                        partconfig_name)

    presets = patchinfo.get_all_patchinfos(device)
    autopatchers = autopatcher.get_auto_patchers()
    inits = ramdisk.get_all_inits()
    ramdisks = ramdisk.get_all_ramdisks(device)

    if action == 'automated':
        if f_preset_name:
            found = False
            for p in presets:
                if f_preset_name == p[0][:-3]:
                    found = True

            if not found:
                raise Exception('Preset %s was not found' % f_preset_name)

        else:
            if f_autopatcher_name:
                found = False
                for a in autopatchers:
                    if f_autopatcher_name == a.name:
                        found = True

                if not found:
                    raise Exception('Autopatcher %s was not found' %
                                    f_autopatcher_name)

            if f_init:
                if f_init not in inits:
                    raise Exception('Patched init %s was not found' % f_init)

            if f_ramdisk:
                if f_ramdisk not in ramdisks:
                    raise Exception('Ramdisk %s was not found' % f_ramdisk)


def ask_device():
    global device

    sld = SimpleListDialog()
    #sld.hasvalues = False
    sld.allownochoice = False
    sld.desc = 'Choose a device:'

    for d in devices:
        sld.add_option(d, config.get(d, 'name'))

    choice = sld.get_choice()

    device = devices[choice - 1]


def ask_partconfig(file_info):
    global partconfig_name

    unsupported_configs = []
    supported_configs = []

    if file_info.patchinfo:
        for c in partition_configs:
            if file_info.patchinfo.is_partconfig_supported(c):
                supported_configs.append(c)
            else:
                unsupported_configs.append(c)
    else:
        supported_configs.extend(partition_configs)

    sld = SimpleListDialog()
    #sld.hasvalues = False
    sld.allownochoice = False
    sld.desc = 'Choose a partition configuration:'

    for c in supported_configs:
        sld.add_option(c.name, c.description)

    for c in unsupported_configs:
        sld.add_option(c.name + ' (unsupported)', c.description, invalid=True)

    choice = sld.get_choice()

    partconfig_name = supported_configs[choice - 1].id


def patch_supported(file_info):
    ui.info('Detected ' + file_info.patchinfo.name)

    file_info.partconfig = partconfig

    newfile = patcher.patch_file(file_info)

    if not newfile:
        sys.exit(1)

    file_info.move_to_target(newfile)

    if not OS.is_android():
        ui.info("Path: " + file_info.get_new_filename())


def patch_unsupported(file_info):
    f_preset = None
    f_preset_name = 'Custom'
    f_autopatcher = autopatchers[0]
    f_autopatcher_name = f_autopatcher.name
    f_patch = None
    f_devicecheck = True
    f_hasbootimage = True
    f_bootimage = None
    f_ramdisk = ramdisks[0][:-4]
    f_init = None

    while True:
        desc = 'File: ' + filename + '\n\n'
        desc += 'The file you have selected is not supported. You can attempt to\n'
        desc += 'patch the file anyway using the options below.'

        choicemsg = 'Choose an option to change it or press Enter to start patching'

        sld = SimpleListDialog()
        sld.desc = desc
        sld.choicemsg = choicemsg

        sld.add_option('Preset', f_preset_name)
        if not f_preset:
            sld.add_option('Autopatcher', f_autopatcher_name)
            sld.add_option('Patch', str(f_patch))
            sld.add_option('Remove device checks', str(not f_devicecheck))
            sld.add_option('File has boot image', str(f_hasbootimage))
            if f_hasbootimage:
                if not f_bootimage:
                    sld.add_option('Boot image(s)', 'Autodetect')
                else:
                    sld.add_option('Boot image(s)', str(f_bootimage))
                sld.add_option('Ramdisk', str(f_ramdisk))
                sld.add_option('Patched init', str(f_init))

        choice = sld.get_choice()

        if choice == 0:
            break

        if choice == 1:
            sld = SimpleListDialog()
            sld.allownochoice = False
            sld.desc = 'Choose a preset:'

            for p in presets:
                sld.add_option(p[0][:-3], p[1].name)

            sld.add_option('Custom', 'Specify options manually')

            choice = sld.get_choice()

            # Custom
            if choice == len(presets) + 1:
                f_preset = None
                f_preset_name = 'Custom'
            else:
                f_preset = presets[choice - 1]
                f_preset_name = f_preset[0][:-3]

        elif choice == 2:
            sld = SimpleListDialog()
            sld.hasvalues = False
            sld.allownochoice = False
            sld.desc = 'Choose an autopatcher:'

            for a in autopatchers:
                sld.add_option(a.name)

            choice = sld.get_choice()

            f_autopatcher = autopatchers[choice - 1]
            f_autopatcher_name = f_autopatcher.name
            f_patch = None

        elif choice == 3:
            cancel = False

            clear_screen()
            ui.info('Enter the path to the patch/diff file ' +
                    '(leave blank to cancel)')
            ui.info('')

            while True:
                text = input('Path: ')

                if not text:
                    cancel = True
                    break

                if os.path.exists(text) and os.path.isfile(text):
                    break
                else:
                    ui.info('Invalid path')
                    continue

            if cancel:
                continue

            f_autopatcher = None
            f_autopatcher_name = 'None'
            f_patch = text

        elif choice == 4:
            sld = SimpleListDialog()
            sld.hasvalues = False
            sld.allownochoice = False
            sld.desc = 'Remove device checks from file:'

            sld.add_option('True')
            sld.add_option('False')

            choice = sld.get_choice()

            f_devicecheck = choice == 2

        elif choice == 5:
            sld = SimpleListDialog()
            sld.hasvalues = False
            sld.allownochoice = False
            sld.desc = 'The file contains a boot image (or multiple boot images):'

            sld.add_option('True')
            sld.add_option('False')

            choice = sld.get_choice()

            f_hasbootimage = choice == 1

        elif choice == 6:
            clear_screen()
            ui.info('Enter the path of the boot image. If there are multiple, ' +
                    'separate them with\ncommas. (Leave blank to autodetect)')
            ui.info('')

            text = input('Boot images: ')

            if text and text.strip():
                f_bootimage = text
            else:
                f_bootimage = None

        elif choice == 7:
            sld = SimpleListDialog()
            sld.hasvalues = False
            sld.allownochoice = False
            sld.desc = 'Choose a ramdisk:'

            for r in ramdisks:
                sld.add_option(r[:-4])

            choice = sld.get_choice()

            f_ramdisk = ramdisks[choice - 1][:-4]

        elif choice == 8:
            sld = SimpleListDialog()
            sld.hasvalues = False
            sld.allownochoice = False
            sld.desc = 'Choose a patched init binary:'

            for i in inits:
                sld.add_option(i)

            sld.add_option('None')

            choice = sld.get_choice()

            # Custom
            if choice == len(inits) + 1:
                f_init = None
            else:
                f_init = inits[choice - 1]

    if f_preset_name == 'Custom':
        patch_info = patchinfo.PatchInfo()

        patch_info.name = 'Unsupported file (with custom patcher options)'

        if f_autopatcher:
            patch_info.patch = f_autopatcher.patcher
            patch_info.extract = f_autopatcher.extractor
        elif f_patch:
            patch_info.patch = f_patch

        patch_info.has_boot_image = f_hasbootimage
        if patch_info.has_boot_image:
            patch_info.ramdisk = f_ramdisk + '.def'
            if f_bootimage and f_bootimage.strip():
                patch_info.bootimg = f_bootimage.split(',')
            patch_info.patched_init = f_init

        patch_info.device_check = f_devicecheck

        file_info.patchinfo = patch_info

    else:
        orig_patch_info = f_preset[1]
        patch_info = patchinfo.PatchInfo()

        patch_info.name = 'Unsupported file (manually set to: %s)' % \
                orig_patch_info.name
        patch_info.patch = orig_patch_info.patch
        patch_info.extract = orig_patch_info.extract
        patch_info.ramdisk = orig_patch_info.ramdisk
        patch_info.bootimg = orig_patch_info.bootimg
        patch_info.has_boot_image = orig_patch_info.has_boot_image
        patch_info.patched_init = orig_patch_info.patched_init
        patch_info.device_check = orig_patch_info.device_check
        patch_info.configs = orig_patch_info.configs
        # Possibly override?
        #patch_info.configs = ['all']

        file_info.patchinfo = patch_info

    patch_supported(file_info)


def load_automated(file_info):
    global f_ramdisk

    f_preset = None
    if f_preset_name:
        for p in presets:
            if p[0][:-3] == f_preset_name:
                f_preset = p

    f_autopatcher = None
    if f_autopatcher_name:
        for a in autopatchers:
            if a.name == f_autopatcher_name:
                f_autopatcher = a

    if f_ramdisk and f_ramdisk.endswith('.def'):
        f_ramdisk = f_ramdisk[:-4]

    if not f_preset:
        patch_info = patchinfo.PatchInfo()

        patch_info.name = 'Unsupported file (with custom parameters)'

        if f_autopatcher:
            patch_info.patch = f_autopatcher.patcher
            patch_info.extract = f_autopatcher.extractor
        elif f_patch:
            patch_info.patch = f_patch

        patch_info.has_boot_image = f_hasbootimage
        if patch_info.has_boot_image:
            patch_info.ramdisk = f_ramdisk + '.def'
            patch_info.bootimg = f_bootimage.split(',')
            patch_info.patched_init = f_init

        patch_info.device_check = f_devicecheck

        file_info.patchinfo = patch_info

    else:
        orig_patch_info = f_preset[1]
        patch_info = patchinfo.PatchInfo()

        patch_info.name = 'Unsupported file (manually set to: %s)' % \
                orig_patch_info.name
        patch_info.patch = orig_patch_info.patch
        patch_info.extract = orig_patch_info.extract
        patch_info.ramdisk = orig_patch_info.ramdisk
        patch_info.bootimg = orig_patch_info.bootimg
        patch_info.has_boot_image = orig_patch_info.has_boot_image
        patch_info.patched_init = orig_patch_info.patched_init
        patch_info.device_check = orig_patch_info.device_check
        patch_info.configs = orig_patch_info.configs
        # Possibly override?
        #patch_info.configs = ['all']

        file_info.patchinfo = patch_info


try:
    parse_args()

    file_info = fileinfo.FileInfo()
    file_info.filename = filename

    if not file_info.is_filetype_supported():
        ui.failed('Unsupported file type')
        sys.exit(1)

    if not device:
        ask_device()

    if device not in devices:
        raise Exception('Device %s is not supported' % device)

    file_info.device = device

    supported = file_info.find_and_merge_patchinfo()

    if not partconfig_name:
        ask_partconfig(file_info)

    check_parameters()

    if action == 'automated':
        load_automated(file_info)

    # Needed for the case where partconfig is passed via command-line
    if action != 'testsupported' and file_info.patchinfo and \
            not file_info.patchinfo.is_partconfig_supported(partconfig):
        raise Exception("The %s partition configuration is not supported for this file"
                % partconfig.id)
except Exception as e:
    ui.failed(str(e))
    sys.exit(1)

if action == 'testsupported':
    if supported:
        print('supported')
    else:
        print('unsupported')

elif action == 'patchfile':
    if supported:
        patch_supported(file_info)
    elif noquestions:
        ui.failed('The file you have selected is not supported.')
        sys.exit(1)
    else:
        patch_unsupported(file_info)

elif action == 'automated':
    if supported:
        ui.info('File is already supported')
        sys.exit(1)
    else:
        patch_supported(file_info)

sys.exit(0)
