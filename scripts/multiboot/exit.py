import shutil
import sys

remove_dirs = []


def remove_on_exit(directory):
    global remove_dirs
    remove_dirs.append(directory)


def exit(exit_code):
    global remove_dirs
    for d in remove_dirs:
        shutil.rmtree(d)
    sys.exit(exit_code)
