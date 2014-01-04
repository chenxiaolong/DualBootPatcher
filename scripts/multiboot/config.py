#!/usr/bin/env python3

import multiboot.operatingsystem as OS

import os
import sys

if sys.hexversion < 0x03000000:
    import ConfigParser
    config = ConfigParser.ConfigParser()
    py3 = False
else:
    import configparser
    config = configparser.ConfigParser()
    py3 = True

config.read(os.path.join(OS.rootdir, 'defaults.conf'))


def get(section, option):
    if py3:
        return config[section][option]
    else:
        return config.get(section, option)


def has(section, option):
    if py3:
        return option in config[section]
    else:
        return config.has_option(section, option)


def get_device():
    return get('Defaults', 'device')


def get_selinux():
    if has(get_device(), 'selinux'):
        return get(get_device(), 'selinux')
    else:
        return None


def get_ramdisk_offset():
    if has(get_device(), 'ramdisk_offset'):
        return get(get_device(), 'ramdisk_offset')
    else:
        return None
