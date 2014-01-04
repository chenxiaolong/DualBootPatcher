import multiboot.operatingsystem as OS

import os
import re
import sys

READ = 'r'
WRITE = 'wb'


def write(f, line):
    if sys.hexversion < 0x03000000:
        f.write(line)
    else:
        f.write(line.encode("UTF-8"))


def open_file(f, flags, directory=""):
    return open(os.path.join(directory, f), flags)


def first_line(f, directory=""):
    f = open_file(f, READ, directory=directory)
    line = f.readline()
    f.close()
    return line.rstrip('\n')


def all_lines(f, directory=""):
    f = open_file(f, READ, directory=directory)
    lines = f.readlines()
    f.close()
    return lines


def write_lines(f, lines, directory=""):
    f = open_file(f, WRITE, directory=directory)

    for i in lines:
        write(f, i)

    f.close()


def whitespace(line):
    m = re.search(r"^(\s+).*$", line)
    if m and m.group(1):
        return m.group(1)
    else:
        return ""


def unix_path(path):
    # Windows sucks
    if OS.is_windows():
        return path.replace('\\', '/')
    else:
        return path
