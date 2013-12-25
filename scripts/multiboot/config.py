#!/usr/bin/env python3

import multiboot.operatingsystem as OS

import configparser
import os

config = configparser.ConfigParser()
config.read(os.path.join(OS.rootdir, 'defaults.conf'))

def get_device():
    return config['Defaults']['device']

def get_ramdisk_offset():
    if 'ramdisk_offset' in config[get_device()]:
        return config[get_device()]['ramdisk_offset']
    else:
        return None

def get_tags_offset():
    if 'tags_offset' in config[get_device()]:
        return config[get_device()]['tags_offset']
    else:
        return None

def get_selinux():
    if 'selinux' in config[get_device()]:
        return config[get_device()]['selinux']
    else:
        return None
